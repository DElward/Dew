/*
**  rcall.c -- Remote call functions
*/
/***************************************************************/
/*
*/
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>

#include "HttpGet.h"
#include "osc.h"
#include "wsdl.h"
#include "xmlh.h"
#include "dbtreeh.h"
#include "snprfh.h"
#include "wsproc.h"
#include "rcall.h"

/***************************************************************/
/* Begin rcall functions                                       */
/***************************************************************/
struct rcall * new_rcall(int binding_type) {

    struct rcall * rc;

    rc = New(struct rcall, 1);
    rc->rc_binding_type       = binding_type;
    rc->rc_location     = NULL;
    rc->rc_soap_action  = NULL;
    rc->rc_host         = NULL;
    rc->rc_location     = NULL;
    rc->rc_response     = NULL;
    rc->rc_resp_len     = 0;
    rc->rc_resp_max     = 0;

    return (rc);
}
/***************************************************************/
void rcall_set_location(struct rcall * rc, const char * location) {

    if (rc->rc_location) Free(rc->rc_location);

    if (location) rc->rc_location = Strdup(location);
    else rc->rc_location = NULL;
}
/***************************************************************/
void rcall_set_soap_action(struct rcall * rc, const char * soap_action) {

    if (rc->rc_soap_action) Free(rc->rc_soap_action);

    if (!soap_action) {
        rc->rc_soap_action = NULL;
    } else {
        rc->rc_soap_action = New(char, 16 + IStrlen(soap_action));
        strcpy(rc->rc_soap_action, "SOAPAction: ");
        strcpy(rc->rc_soap_action + IStrlen(rc->rc_soap_action), soap_action);
    }
}
/***************************************************************/
void rcall_set_host(struct rcall * rc, const char * host) {

    if (rc->rc_host) Free(rc->rc_host);

    if (!host) {
        rc->rc_host = NULL;
    } else {
        rc->rc_host = New(char, 16 + IStrlen(host));
        strcpy(rc->rc_host, "Host: ");
        strcpy(rc->rc_host + IStrlen(rc->rc_host), host);
    }
}
/***************************************************************/
void free_rcall(struct rcall * rc) {

    Free(rc->rc_location);
    Free(rc->rc_soap_action);
    Free(rc->rc_host);
    Free(rc->rc_response);

    if (rc->rc_content_xt) free_xmltree(rc->rc_content_xt);

    Free(rc);
}
/***************************************************************/
static void tgt_raw_string(char ** tgt_buf,
                int  *  tgt_len,
                int  *  tgt_max,
                const void * tbuf,
                int          tlen)
{
    if ((*tgt_len) + tlen >= (*tgt_max)) {
        if (!(*tgt_max)) (*tgt_max) = 64;
        else              (*tgt_max) *= 2;
        /* check tgt_max again, in case tlen is giant */
        if ((*tgt_len) + tlen >= (*tgt_max))
            (*tgt_max) = (*tgt_len) + tlen + 64;
        (*tgt_buf) = Realloc((*tgt_buf), char, (*tgt_max));
    }
    memcpy((*tgt_buf) + (*tgt_len), tbuf, tlen);
    (*tgt_len) += tlen;
    (*tgt_buf)[*tgt_len] = '\0';
}
/***************************************************************/
static size_t rcall_write_callback(void *contents, size_t size, size_t nmemb, void * vrp)
{
  size_t realsize = size * nmemb;
  struct rcall * rc = (struct rcall *)vrp;

  tgt_raw_string(&(rc->rc_response), &(rc->rc_resp_len), &(rc->rc_resp_max),
    contents, realsize);

  return realsize;
}
/***************************************************************/
int rcall_call(struct wsglob * wsg,
            struct dynl_info * dynl,
            struct rcall * rc,
            struct xmltree ** p_soap_result)
{
/*
********
** From: http://stackoverflow.com/questions/4150710/
**              soap-request-and-response-read-from-and-to-file-using-libcurl-c
**  Presuming that your SOAP server expects a HTTP POST, you need to:
**      Set the CURLOPT_POST option;
**      Set the Content-Type header
**          Content-Type: application/soap+xml using CURLOPT_HTTPHEADER;
**      Set the size of your XML request using the CURLOPT_POSTFIELDSIZE option.
**
********
**          POST /stockquote.asmx HTTP/1.1
**          Content-Type: text/xml
**          Content-Encoding: UTF-8
**          SOAPAction: http://www.webserviceX.NET/GetQuote
**          Host: www.webservicex.com
**          Content-Length: 224
**          
**          <s11:Envelope xmlns:s11='http://schemas.xmlsoap.org/soap/envelope/'>
**            <s11:Body>
**              <ns1:GetQuote xmlns:ns1='http://www.webserviceX.NET/'>
**                <ns1:symbol>PG</ns1:symbol>
**              </ns1:GetQuote>
**            </s11:Body>
**          </s11:Envelope>
*/

    int wstat = 0;
    int xstat;
    CURL *curl_handle;
    CURLcode res;
    char * xstr;
    char * xresp;
    struct xmltree * resp_xtree;
    struct curl_slist *http_hdr = NULL;
    char errmsg[GLB_ERRMSG_SIZE];

    http_hdr = dynl->dcf->dcf_curl_slist_append(http_hdr, "Content-Type: text/xml; charset=utf-8");
    /* http_hdr = dynl->dcf->dcf_curl_slist_append(http_hdr, "Content-Encoding: UTF-8"); */

    if (rc->rc_soap_action) {
        http_hdr = dynl->dcf->dcf_curl_slist_append(http_hdr, rc->rc_soap_action);
    }

    if (rc->rc_host) {
        http_hdr = dynl->dcf->dcf_curl_slist_append(http_hdr, rc->rc_host);
    }

    xstr = xmltree_to_string(rc->rc_content_xt, 0);
    /* fprintf(stdout, "xstr: %s\n", xstr); */

    dynl->dcf->dcf_curl_global_init(CURL_GLOBAL_ALL);

    /* init the curl session */
    curl_handle = dynl->dcf->dcf_curl_easy_init();
    /* ask libcurl to show us the verbose output */
    /* dynl->dcf->dcf_curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 1L); */

    dynl->dcf->dcf_curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, FALSE);

    dynl->dcf->dcf_curl_easy_setopt(curl_handle, CURLOPT_URL, rc->rc_location);

    /* send all data to this function  */
    dynl->dcf->dcf_curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, rcall_write_callback);

    /* we pass our 'chunk' struct to the callback function */
    dynl->dcf->dcf_curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)rc);

    /* some servers don't like requests that are made without a user-agent
       field, so we provide one */
    dynl->dcf->dcf_curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

    dynl->dcf->dcf_curl_easy_setopt(curl_handle, CURLOPT_POST, 1);

    dynl->dcf->dcf_curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, http_hdr);

    dynl->dcf->dcf_curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, xstr);

    /* if we don't provide POSTFIELDSIZE, libcurl will strlen() by
       itself */
    dynl->dcf->dcf_curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDSIZE, (long)strlen(xstr));

    /* Perform the request, res will get the return code */
    res = dynl->dcf->dcf_curl_easy_perform(curl_handle);

    /* Check for errors */
    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n",
              dynl->dcf->dcf_curl_easy_strerror(res));
    } else {
        /* fprintf(stdout, "Answer: %s\n", rc->rc_response); */
        resp_xtree = NULL;
        xstat = xmlp_get_xmltree_from_string(rc->rc_response, &resp_xtree, errmsg, sizeof(errmsg));
        if (xstat) {
            wstat = ws_set_error(wsg, WSE_XMLRESPONSE,
                        "%s", errmsg);
        } else {
            xresp = xmltree_to_string(resp_xtree, XTTS_INDENT | XTTS_NL);
            fprintf(stdout, "XML Answer: %s\n", xresp);
            Free(xresp);
            free_xmltree(resp_xtree);
        }
    }

    dynl->dcf->dcf_curl_slist_free_all(http_hdr);
    Free(xstr);

    return (wstat);
}
/***************************************************************/
/* End of rcall functions                                      */
/***************************************************************/
