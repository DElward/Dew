/*
**  wsdl.c -- wsdl functions
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
#include "xmlh.h"
#include "dbtreeh.h"
#include "snprfh.h"
#include "wsproc.h"
#include "wsdl.h"

#define TEST_NAME_SPACES 0

/*
** Documentation:
**  Specifications:
**      WSDL        : http://www.w3.org/TR/wsdl
**      XML Schema  : http://www.w3.org/TR/xmlschema11-1/
**      Datatypes   : http://www.w3.org/TR/xmlschema-2/
**      SOAP 1.1    : http://www.w3.org/TR/SOAP
**      SOAP 1.2    : http://www.w3.org/TR/soap12/
**      SOAP binding:
**          1.1 https://docs.oracle.com/cd/E19182-01/821-0015/ggeij/index.html
**          1.2 https://docs.oracle.com/cd/E19182-01/821-0015/ggeij2/index.html
**
**      Other:
**          WSDL        : http://www.w3schools.com/xml/xml_wsdl.asp
**          WSDL        : http://www.tutorialspoint.com/wsdl/index.htm
**          WSDL        : http://oak.cs.ucla.edu/cs144/reference/WSDL/
**          WSDL 2.0    : http://tutorials.jenkov.com/wsdl/index.html (OAuth 2.0)
**          XML Schema  : http://www.w3schools.com/schema/default.asp
**          XML Schema  : http://www.tutorialspoint.com/xsd/index.htm
**          SOAP        : http://www.w3schools.com/xml/xml_soap.asp
**          Web services:
**              http://www.xml.com/pub/a/ws/2001/04/04/webservices/index.html
**
**                        http://article.gmane.org/gmane.text.xml.axis.devel/
**                              6085/match=matching+portType
**
** Testing
**      Directory of public SOAP Web Services
**          http://www.service-repository.com/
**          Stock quote: http://www.webservicex.com/new/Home/Index
**              WSDL: http://www.webservicex.net/stockquote.asmx?WSDL
**      http://soapclient.com/soaptest.html
**
** Example from Membrane (XML response edited for readability):
**      WSDL    : http://www.webservicex.net/stockquote.asmx?WSDL
**      Endpoint: http://www.webservicex.com/stockquote.asmx
**      Request:
**
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
**          
**      Response:
**
**          HTTP/1.1 200 OK
**          Cache-Control: private, max-age=0
**          Content-Type: text/xml; charset=utf-8
**          Content-Encoding: UTF-8
**          Server: Microsoft-IIS/7.0
**          X-AspNet-Version: 4.0.30319
**          X-Powered-By: ASP.NET
**          Date: Tue, 03 Nov 2015 20:23:34 GMT
**          Content-Length: 1017
**          
**          <?xml version = "1.0" encoding = "UTF-8" ?>
**          <soap:Envelope
**              xmlns:soap="http://schemas.xmlsoap.org/soap/envelope/"
**              xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
**              xmlns:xsd="http://www.w3.org/2001/XMLSchema">
**            <soap:Body>
**              <GetQuoteResponse xmlns="http://www.webserviceX.NET/">
**                <GetQuoteResult>
**                  <StockQuotes>
**                    <Stock>
**                      <Symbol>PG</Symbol>
**                      <Last>77.265</Last>
**                      <Date>11/3/2015</Date>
**                      <Time>3:08pm</Time>
**                      <Change>+0.665</Change>
**                      <Open>76.300</Open>
**                      <High>77.420</High>
**                      <Low>75.960</Low>
**                      <Volume>5795203</Volume>
**                      <MktCap>210.20B</MktCap>
**                      <PreviousClose>76.600</PreviousClose>
**                      <PercentageChange>+0.868%</PercentageChange>
**                      <AnnRange>65.020 - 93.890</AnnRange>
**                      <Earns>2.657</Earns>
**                      <P-E>29.080</P-E>
**                      <Name>Procter & Gamble Company (The) </Name>
**                    </Stock>
**                  </StockQuotes>
**                </GetQuoteResult>
**              </GetQuoteResponse>
**            </soap:Body>
**          </soap:Envelope>
**
**      SOAP 1.2 Request:
**
**          POST /stockquote.asmx HTTP/1.1
**          Content-Type: application/soap+xml
**          Content-Encoding: UTF-8
**          SOAPAction: http://www.webserviceX.NET/GetQuote
**          Host: www.webservicex.com
**          Content-Length: 223
**          
**          <s12:Envelope xmlns:s12='http://www.w3.org/2003/05/soap-envelope'>
**            <s12:Body>
**              <ns1:GetQuote xmlns:ns1='http://www.webserviceX.NET/'>
**                <ns1:symbol>JNJ</ns1:symbol>
**              </ns1:GetQuote>
**            </s12:Body>
**          </s12:Envelope>
**
**      SOAP 1.2 Response:
**
**          HTTP/1.1 200 OK
**          Cache-Control: private, max-age=0
**          Content-Type: application/soap+xml; charset=utf-8
**          Content-Encoding: UTF-8
**          Server: Microsoft-IIS/7.0
**          X-AspNet-Version: 4.0.30319
**          X-Powered-By: ASP.NET
**          Date: Thu, 05 Nov 2015 21:11:48 GMT
**          Content-Length: 1032
**          
**          <?xml version = "1.0" encoding = "UTF-8" ?>
**          <soap:Envelope xmlns:soap="http://www.w3.org/2003/05/soap-envelope"
**                         xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
**                         xmlns:xsd="http://www.w3.org/2001/XMLSchema">
**            <soap:Body>
**              <GetQuoteResponse xmlns="http://www.webserviceX.NET/">
**                <GetQuoteResult>
**                  <StockQuotes>
**                    <Stock>
**                      <Symbol>JNJ</Symbol>
**                      <Last>102.4003</Last>
**                      <Date>11/5/2015</Date>
**                      <Time>3:56pm</Time>
**                      <Change>+0.4603</Change>
**                      <Open>102.2500</Open>
**                      <High>102.4900</High>
**                      <Low>101.4200</Low>
**                      <Volume>5609407</Volume>
**                      <MktCap>283.34B</MktCap>
**                      <PreviousClose>101.9400</PreviousClose>
**                      <PercentageChange>+0.4515%</PercentageChange>
**                      <AnnRange>81.7900 - 109.4900</AnnRange>
**                      <Earns>5.2110</Earns>
**                      <P-E>19.6508</P-E>
**                      <Name>Johnson & Johnson Common Stock</Name>
**                    </Stock>
**                  </StockQuotes>
**                </GetQuoteResult>
**              </GetQuoteResponse>
**            </soap:Body>
**          </soap:Envelope>
**
********
** From: http://stackoverflow.com/questions/4150710/
**              soap-request-and-response-read-from-and-to-file-using-libcurl-c
**  Presuming that your SOAP server expects a HTTP POST, you need to:
**      Set the CURLOPT_POST option;
**      Set the Content-Type header
**          Content-Type: application/soap+xml using CURLOPT_HTTPHEADER;
**      Set the size of your XML request using the CURLOPT_POSTFIELDSIZE option.
**
*/

/* forward functions */
void free_wssoplist(struct wssoplist * wssopl);

/***************************************************************/
char * xml_find_attr(struct xmltree * xtree, const char * attr_name) {

    int atix;
    char * attr_val;

    attr_val = NULL;
    atix = 0;
    while (!attr_val && atix < xtree->nattrs) {
        if (!strcmp(xtree->attrnam[atix], attr_name)) {
            attr_val = xtree->attrval[atix];
        } else {
            atix++;
        }
    }

    return (attr_val);
}
/***************************************************************/
char * xml_get_name(const char * element_name) {

    char * colon;

    colon = strchr(element_name, ':');
    if (colon) {
        colon += 1;
    }

    return (colon);
}
/***************************************************************/
void xml_get_name_space(const char * element_name,
                        char * name_space,
                        int name_space_sz) {

    int nslen;
    char * colon;

    colon = strchr(element_name, ':');
    if (colon) {
        nslen = colon - element_name;
        if (nslen >= name_space_sz) nslen = name_space_sz - 1;
        memcpy(name_space, element_name, nslen);
        name_space[nslen] = '\0';
    } else {
        name_space[0] = '\0';
    }
}
/***************************************************************/
int xml_item_name_equals(const char * element_name, const char * item_name) {

    int is_eq;
    char * colon;

    is_eq = 0;
    colon = strchr(element_name, ':');
    if (colon) {
        if (!ELSTRCMP(colon + 1, item_name)) is_eq = 1;
    } else {
        if (!ELSTRCMP(element_name, item_name)) is_eq = 1;
    }

    return (is_eq);
}
/***************************************************************/
int xml_name_space_equals(const char * element_name, const char * name_space) {

    int is_eq;
    int nslen;
    char * colon;

    is_eq = 0;
    colon = strchr(element_name, ':');
    if (colon) {
        nslen = colon - element_name;
        if (!memcmp(element_name, name_space, nslen) &&
            !name_space[nslen]) is_eq = 1;
    } else {
        if (!name_space[0]) is_eq = 1;
    }

    return (is_eq);
}
/***************************************************************/
int xml_full_name_equals(const char * element_name,
                    const char * name_space,
                    const char * item_name) {

    int is_eq;
    int nslen;
    char * colon;

    is_eq = 0;
    colon = strchr(element_name, ':');
    if (colon) {
        nslen = colon - element_name;
        if (!memcmp(element_name, name_space, nslen) &&
            !name_space[nslen] &&
            !ELSTRCMP(colon + 1, item_name)) is_eq = 1;
    } else {
        if (!name_space[0] &&
            !ELSTRCMP(element_name, item_name)) is_eq = 1;
    }

    return (is_eq);
}
/***************************************************************/
struct xmltree * parse_wsdl_find_item(int nsubel,
            struct xmltree ** subel,
            const char * item_name) {

    struct xmltree * xtree_itm;
    int ii;

    xtree_itm = NULL;
    ii = 0;
    while (ii < nsubel && !xtree_itm) {
        if (xml_item_name_equals(subel[ii]->elname, item_name)) {
            xtree_itm = subel[ii];
        } else {
            ii++;
        }
    }

    return (xtree_itm);
}
/***************************************************************/
struct xmltree * parse_wsdl_find_definitions(struct xmltree * xtree) {
/*
** Element name = Full element name in XML file
** Namespace    = Part of element name in XML before ':', or blank if none
** Item name    = Part of element name in XML after ':', or element name if none
*/
    int estat = 0;
    struct xmltree * xtree_wsd;

    if (xml_item_name_equals(xtree->elname, WSDL_DOCUMENT)) {
        xtree_wsd = parse_wsdl_find_item(xtree->nsubel, xtree->subel,
                        WSDL_DEFINITIONS);
    } else if (xml_item_name_equals(xtree->elname, WSDL_DEFINITIONS)) {
        xtree_wsd = xtree;
    } else {
        xtree_wsd = NULL;
    }

    return (xtree_wsd);
}
/***************************************************************/
/* Name space functions                                        */
/***************************************************************/
struct nslist {  /* nsl_    name space list */
    int                     nsl_mname_space;
    int                     nsl_nname_space;
    char **                 nsl_aname_space;
    char **                 nsl_aname_space_uri;
    int *                   nsl_aname_space_uri_sorted;
    struct nslist *         nsl_parent_nslist;
};
/***************************************************************/
int nsl_find_name_space_index(struct nslist * nsl, const char * name_space) {


    int lo;
    int hi;
    int mid;

    lo = 0;
    hi = nsl->nsl_nname_space - 1;
    while (lo <= hi) {
        mid = (lo + hi) / 2;
        if (strcmp(name_space, nsl->nsl_aname_space[mid]) <= 0)
                                             hi = mid - 1;
        else                                 lo = mid + 1;
    }

    return (lo);
}
/***************************************************************/
char * nsl_find_name_space(struct nslist * nsl,
                           const char * name_space,
                           int check_parent) {

    int ix;
    char * nsuri;

    nsuri = NULL;
    ix = nsl_find_name_space_index(nsl, name_space);

    if (ix >= nsl->nsl_nname_space ||
        strcmp(name_space, nsl->nsl_aname_space[ix])) {
        if (check_parent > 0 && nsl->nsl_parent_nslist) {
            nsuri = nsl_find_name_space(
                nsl->nsl_parent_nslist, name_space, check_parent - 1);
        }
    } else {
        nsuri = nsl->nsl_aname_space_uri[ix];
    }

    return (nsuri);
}
/***************************************************************/
int nsl_find_name_space_uri_index(struct nslist * nsl, const char * name_space_uri) {


    int lo;
    int hi;
    int mid;

    lo = 0;
    hi = nsl->nsl_nname_space - 1;
    while (lo <= hi) {
        mid = (lo + hi) / 2;
        if (strcmp(name_space_uri,
            nsl->nsl_aname_space_uri[nsl->nsl_aname_space_uri_sorted[mid]]) <= 0)
                                             hi = mid - 1;
        else                                 lo = mid + 1;
    }

    return (lo);
}
/***************************************************************/
char * nsl_find_name_space_uri(struct nslist * nsl,
                            const char * name_space_uri,
                            int check_parent) {

    int ix;
    char * ns;

    ns = NULL;
    ix = nsl_find_name_space_uri_index(nsl, name_space_uri);

    if (ix >= nsl->nsl_nname_space ||
        strcmp(name_space_uri,
               nsl->nsl_aname_space_uri[nsl->nsl_aname_space_uri_sorted[ix]])) {
        if (check_parent > 0 && nsl->nsl_parent_nslist) {
            ns = nsl_find_name_space_uri(
                nsl->nsl_parent_nslist, name_space_uri, check_parent - 1);
        }
    } else {
        /* find first one */
        while (ix > 0 &&
            !strcmp(nsl->nsl_aname_space[nsl->nsl_aname_space_uri_sorted[ix]],
                    nsl->nsl_aname_space[nsl->nsl_aname_space_uri_sorted[ix-1]])) {
            ix--;
        }
        ns = nsl->nsl_aname_space[nsl->nsl_aname_space_uri_sorted[ix]];
    }

    return (ns);
}
/***************************************************************/
int nsl_insert_name_space(struct nslist * nsl,
                    const char * name_space,
                    const char * name_space_uri) {

    int ix;
    int uri_ix;
    int ii;
    int is_dup;

    is_dup = 0;
    ix = nsl_find_name_space_index(nsl, name_space);
    if (ix >= nsl->nsl_nname_space ||
        strcmp(name_space, nsl->nsl_aname_space[ix])) {
        uri_ix = nsl_find_name_space_uri_index(nsl, name_space_uri);

        if (nsl->nsl_nname_space >= nsl->nsl_mname_space) {
                if (!nsl->nsl_mname_space) nsl->nsl_mname_space = 16;
                else                       nsl->nsl_mname_space *= 2;
                nsl->nsl_aname_space =
                    Realloc(nsl->nsl_aname_space, char*, nsl->nsl_mname_space);
                nsl->nsl_aname_space_uri =
                  Realloc(nsl->nsl_aname_space_uri,char*,nsl->nsl_mname_space);
                nsl->nsl_aname_space_uri_sorted =
                  Realloc(nsl->nsl_aname_space_uri_sorted,int,nsl->nsl_mname_space);
        }
        for (ii = nsl->nsl_nname_space; ii > ix; ii--) {
            nsl->nsl_aname_space[ii]     = nsl->nsl_aname_space[ii - 1];
            nsl->nsl_aname_space_uri[ii] = nsl->nsl_aname_space_uri[ii - 1];
        }
        nsl->nsl_aname_space[ix]     = Strdup(name_space);
        nsl->nsl_aname_space_uri[ix] = Strdup(name_space_uri);

        /* name_space_uri */
        for (ii = nsl->nsl_nname_space; ii >= 0; ii--) {
            /* insert new uri */
            if (ii > uri_ix) {
                nsl->nsl_aname_space_uri_sorted[ii] = 
                    nsl->nsl_aname_space_uri_sorted[ii - 1];
            }
            /* adjust index for inserted record */
            if (nsl->nsl_aname_space_uri_sorted[ii] >= ix) {
                nsl->nsl_aname_space_uri_sorted[ii] += 1;
            }
        }

        nsl->nsl_aname_space_uri_sorted[uri_ix] = ix;

        nsl->nsl_nname_space += 1;
    } else {
        is_dup = 1;
    }

    return (is_dup);
}
/***************************************************************/
/* WSDL parsing constructors                                   */
/***************************************************************/
struct nslist * new_nslist(struct nslist * parent_nsl) {

    struct nslist * nsl;

    nsl = New(struct nslist, 1);
    nsl->nsl_parent_nslist      = parent_nsl;
    nsl->nsl_mname_space        = 0;
    nsl->nsl_nname_space        = 0;
    nsl->nsl_aname_space        = NULL;
    nsl->nsl_aname_space_uri    = NULL;
    nsl->nsl_aname_space_uri_sorted = NULL;

    return (nsl);
}
/***************************************************************/
void free_nslist(struct nslist * nsl) {

    int ii;

    for (ii = 0; ii < nsl->nsl_nname_space; ii++) {
        Free(nsl->nsl_aname_space[ii]);
        Free(nsl->nsl_aname_space_uri[ii]);
    }
    Free(nsl->nsl_aname_space);
    Free(nsl->nsl_aname_space_uri);
    Free(nsl->nsl_aname_space_uri_sorted);

    Free(nsl);
}
/***************************************************************/
struct wsdocumentation * new_wsdocumentation(void) {

    struct wsdocumentation * wstd;

    wstd = New(struct wsdocumentation, 1);
    wstd->wstd_source = NULL;
    wstd->wstd_xml_lang = NULL;
    wstd->wstd_cdata = NULL;

    return (wstd);
}
/***************************************************************/
struct wsappinfo * new_wsappinfo(void) {

    struct wsappinfo * wsti;

    wsti = New(struct wsappinfo, 1);
    wsti->wsti_source = NULL;
    wsti->wsti_cdata = NULL;

    return (wsti);
}
/***************************************************************/
struct wsannotation * new_wsannotation(void) {

    struct wsannotation * wsta;

    wsta = New(struct wsannotation, 1);
    wsta->wsta_id = NULL;
    wsta->wsta_ncontent = 0;
    wsta->wsta_acontent = NULL;

    return (wsta);
}
/***************************************************************/
struct wsanyattribute * new_wsanyattribute(void) {

    struct wsanyattribute * wsty;

    wsty = New(struct wsanyattribute, 1);
    wsty->wsty_id = NULL;
    wsty->wsty_namespace = NULL;
    wsty->wsty_notNamespace = NULL;
    wsty->wsty_notQName = 0;
    wsty->wsty_processContents = NULL;
    wsty->wsty_ncontent = 0;
    wsty->wsty_acontent = NULL;

    return (wsty);
}
/***************************************************************/
struct wsany * new_wsany(void) {

    struct wsany * wstz;

    wstz = New(struct wsany, 1);
    wstz->wstz_id = NULL;
    wstz->wstz_maxOccurs = NULL;
    wstz->wstz_minOccurs = NULL;
    wstz->wstz_namespace = NULL;
    wstz->wstz_notNamespace = NULL;
    wstz->wstz_notQName = 0;
    wstz->wstz_processContents = NULL;
    wstz->wstz_ncontent = 0;
    wstz->wstz_acontent = NULL;

    return (wstz);
}
/***************************************************************/
struct wsall * new_wsall(void) {

    struct wsall * wstwa;

    wstwa = New(struct wsall, 1);
    wstwa->wstwa_id = NULL;
    wstwa->wstwa_maxOccurs = NULL;
    wstwa->wstwa_minOccurs = NULL;
    wstwa->wstwa_ncontent = 0;
    wstwa->wstwa_acontent = NULL;

    return (wstwa);
}
/***************************************************************/
struct wschoice * new_wschoice(void) {

    struct wschoice * wstwc;

    wstwc = New(struct wschoice, 1);
    wstwc->wstwc_id = NULL;
    wstwc->wstwc_maxOccurs = NULL;
    wstwc->wstwc_minOccurs = NULL;
    wstwc->wstwc_ncontent = 0;
    wstwc->wstwc_acontent = NULL;

    return (wstwc);
}
/***************************************************************/
struct wssequence * new_wssequence(void) {

    struct wssequence * wstq;

    wstq = New(struct wssequence, 1);
    wstq->wstq_id = NULL;
    wstq->wstq_maxOccurs = NULL;
    wstq->wstq_minOccurs = NULL;
    wstq->wstq_ncontent = 0;
    wstq->wstq_acontent = NULL;

    return (wstq);
}
/***************************************************************/
struct wselement * new_wselement(void) {

    struct wselement * wste;

    wste = New(struct wselement, 1);
    wste->wste_abstract = NULL;
    wste->wste_block = NULL;
    wste->wste_default = NULL;
    wste->wste_final = NULL;
    wste->wste_fixed = NULL;
    wste->wste_form = NULL;
    wste->wste_id = NULL;
    wste->wste_maxOccurs = NULL;
    wste->wste_minOccurs = NULL;
    wste->wste_name = NULL;
    wste->wste_nillable = NULL;
    wste->wste_ref = NULL;
    wste->wste_substitutionGroup = NULL;
    wste->wste_targetNamespace = NULL;
    wste->wste_type = NULL;
    wste->wste_ncontent = 0;
    wste->wste_acontent = NULL;

    return (wste);
}
/***************************************************************/
struct wsgroup * new_wsgroup(void) {

    struct wsgroup * wstg;

    wstg = New(struct wsgroup, 1);
    wstg->wstg_id = NULL;
    wstg->wstg_maxOccurs = NULL;
    wstg->wstg_minOccurs = NULL;
    wstg->wstg_name = NULL;
    wstg->wstg_ref = NULL;
    wstg->wstg_ncontent = 0;
    wstg->wstg_acontent = NULL;

    return (wstg);
}
/***************************************************************/
struct wsschema * new_wsschema(struct wstypes * wst) {

    struct wsschema * wsts;

    wsts = New(struct wsschema, 1);

    wsts->wsts_attributeFormDefault = NULL;
    wsts->wsts_blockDefault = NULL;
    wsts->wsts_defaultAttributes = NULL;
    wsts->wsts_xpathDefaultNamespace = NULL;
    wsts->wsts_elementFormDefault = NULL;
    wsts->wsts_finalDefault = NULL;
    wsts->wsts_id = NULL;
    wsts->wsts_targetNamespace = NULL;
    wsts->wsts_version = NULL;
    wsts->wsts_xml_lang = NULL;
    wsts->wsts_wstypes_ref = wst;
    wsts->wsts_tns = NULL;
    wsts->wsts_nslist             = new_nslist(wst->wst_wsdef_ref->wsd_nslist);

    wsts->wsts_name_space[0]    = '\0';
    wsts->wsts_schematype_dbt   = NULL;

    return (wsts);
}
/***************************************************************/
struct wsattribute * new_wsattribute(void) {

    struct wsattribute * wstb;

    wstb = New(struct wsattribute, 1);
    wstb->wstb_default = NULL;
    wstb->wstb_fixed = NULL;
    wstb->wstb_form = NULL;
    wstb->wstb_id = NULL;
    wstb->wstb_name = NULL;
    wstb->wstb_ref = NULL;
    wstb->wstb_targetNamespace = NULL;
    wstb->wstb_type = NULL;
    wstb->wstb_use = NULL;
    wstb->wstb_inheritable = NULL;
    wstb->wstb_ncontent = 0;
    wstb->wstb_acontent = NULL;

    return (wstb);
}
/***************************************************************/
struct wsimport * new_wsimport(void) {

    struct wsimport * wstf;

    wstf = New(struct wsimport, 1);
    wstf->wstf_id = NULL;
    wstf->wstf_namespace = NULL;
    wstf->wstf_schemaLocation = NULL;
    wstf->wstf_ncontent = 0;
    wstf->wstf_acontent = NULL;

    return (wstf);
}
/***************************************************************/
struct wssimplecontent * new_wssimplecontent(void) {

    struct wssimplecontent * wstp;

    wstp = New(struct wssimplecontent, 1);

    wstp->wstp_id = NULL;
    wstp->wstp_ncontent = 0;
    wstp->wstp_acontent = NULL;

    return (wstp);
}
/***************************************************************/
struct wssimpletype * new_wssimpletype(void) {

    struct wssimpletype * wstt;

    wstt = New(struct wssimpletype, 1);

    wstt->wstt_final = NULL;
    wstt->wstt_id = NULL;
    wstt->wstt_name = NULL;
    wstt->wstt_ncontent = 0;
    wstt->wstt_acontent = NULL;

    return (wstt);
}
/***************************************************************/
struct wsenumeration * new_wsenumeration(void) {

    struct wsenumeration * wstu;

    wstu = New(struct wsenumeration, 1);

    wstu->wstu_id = NULL;
    wstu->wstu_value = NULL;

    return (wstu);
}
/***************************************************************/
struct wsrestriction * new_wsrestriction(void) {

    struct wsrestriction * wstr;

    wstr = New(struct wsrestriction, 1);

    wstr->wstr_base = NULL;
    wstr->wstr_id = NULL;
    wstr->wstr_ncontent = 0;
    wstr->wstr_acontent = NULL;

    return (wstr);
}
/***************************************************************/
struct wscomplextype * new_wscomplextype(void) {

    struct wscomplextype * wstx;

    wstx = New(struct wscomplextype, 1);

    wstx->wstx_abstract = NULL;
    wstx->wstx_block = NULL;
    wstx->wstx_final = NULL;
    wstx->wstx_id = NULL;
    wstx->wstx_mixed = NULL;
    wstx->wstx_name = NULL;
    wstx->wstx_defaultAttributesApply = NULL;
    wstx->wstx_ncontent = 0;
    wstx->wstx_acontent = NULL;

    return (wstx);
}
/***************************************************************/
struct wscomplexcontent * new_wscomplexcontent(void) {

    struct wscomplexcontent * wstk;

    wstk = New(struct wscomplexcontent, 1);

    wstk->wstk_id = NULL;
    wstk->wstk_mixed = NULL;
    wstk->wstk_ncontent = 0;
    wstk->wstk_acontent = NULL;

    return (wstk);
}
/***************************************************************/
struct wslist * new_wslist(void) {

    struct wslist * wstl;

    wstl = New(struct wslist, 1);

    wstl->wstl_id = NULL;
    wstl->wstl_itemType = NULL;
    wstl->wstl_ncontent = 0;
    wstl->wstl_acontent = NULL;

    return (wstl);
}
/***************************************************************/
struct wsunion * new_wsunion(void) {

    struct wsunion * wstv;

    wstv = New(struct wsunion, 1);

    wstv->wstv_id = NULL;
    wstv->wstv_memberTypes = NULL;
    wstv->wstv_ncontent = 0;
    wstv->wstv_acontent = NULL;

    return (wstv);
}
/***************************************************************/
struct wsextension * new_wsextension(void) {

    struct wsextension * wstn;

    wstn = New(struct wsextension, 1);

    wstn->wstn_base = NULL;
    wstn->wstn_id = NULL;
    wstn->wstn_ncontent = 0;
    wstn->wstn_acontent = NULL;

    return (wstn);
}
/***************************************************************/
struct wscontent * new_wscontent(int content_from,
                    int content_type,
                    char * qname) {

    struct wscontent * wstc;

    wstc = New(struct wscontent, 1);

    wstc->wstc_content_from     = content_from;
    wstc->wstc_content_type     = content_type;
    wstc->wstc_qname            = qname;
    wstc->wstc_itm.wstcu_void   = NULL;              

    return (wstc);
}
/***************************************************************/
struct wsschematype * new_wsschematype(const char * itm_name,
                int itm_name_len) {

    struct wsschematype * wsst;

    wsst = New(struct wsschematype, 1);
    wsst->wsst_sch_itm_name = New(char, itm_name_len + 1);
    memcpy(wsst->wsst_sch_itm_name, itm_name, itm_name_len + 1);
    wsst->wsst_sch_content = NULL;

    return (wsst);
}
/***************************************************************/
int get_num_wsschema(struct wstypes * wst)
{
    return (wst->wst_num_wsschema);
}
/***************************************************************/
struct wstypes * new_wstypes(struct wsdef * wsd) {

    struct wstypes * wst;

    wst = New(struct wstypes, 1);
    wst->wst_wsdef_ref = wsd;
    wst->wst_num_wsschema = 0;
    wst->wst_wsschema_dbt = dbtree_new();

/*  wst->wst_nwsschema = 0; */
/*  wst->wst_awsschema = NULL; */

    return (wst);
}
/***************************************************************/
struct wspart * new_wspart(void) {

    struct wspart * wsmp;

    wsmp = New(struct wspart, 1);
    wsmp->wsmp_name = NULL;
    wsmp->wsmp_element = NULL;
    wsmp->wsmp_type = NULL;

    return (wsmp);
}
/***************************************************************/
struct wsmessage * new_wsmessage() {

    struct wsmessage * wsm;

    wsm = New(struct wsmessage, 1);
    wsm->wsm_name = NULL;
    wsm->wsm_nwspart = 0;
    wsm->wsm_awspart = NULL;
    wsm->wsm_qname = NULL;

    return (wsm);
}
/***************************************************************/
struct wspopinfo * new_wspopinfo(int ptyp) {

    struct wspopinfo * wsyi;

    wsyi = New(struct wspopinfo, 1);
    wsyi->wsyi_ptyp     = ptyp;
    wsyi->wsyi_name     = NULL;
    wsyi->wsyi_message  = NULL;
    wsyi->wsyi_cdata    = NULL;

    return (wsyi);
}
/***************************************************************/
struct wspoperation * new_wspoperation(void) {

    struct wspoperation * wsyo;

    wsyo = New(struct wspoperation, 1);
    wsyo->wsyo_name          = NULL;
    wsyo->wsyo_nwspopinfo = 0;
    wsyo->wsyo_awspopinfo = NULL;

    return (wsyo);
}
/***************************************************************/
struct wsyoplist * new_wsyoplist(void) {

    struct wsyoplist * wsyl;

    wsyl = New(struct wsyoplist, 1);
    wsyl->wsyl_nwspoperation = 0;
    wsyl->wsyl_awspoperation = NULL;

    return (wsyl);
}
/***************************************************************/
struct wsporttype * new_wsporttype(void) {

    struct wsporttype * wsy;

    wsy = New(struct wsporttype, 1);
    wsy->wsy_name           = NULL;
    wsy->wsy_qname          = NULL;
    wsy->wsy_wsyoplist_dbt  = NULL;

    return (wsy);
}
/***************************************************************/
struct wsport * new_wsport(struct wsservice * wss) {

    struct wsport * wsp;

    wsp = New(struct wsport, 1);
    wsp->wsp_wsservice_ref = wss;
    wsp->wsp_name = NULL;
    wsp->wsp_binding = NULL;
    wsp->wsp_location = NULL;
    wsp->wsp_port_name_space = NULL;

    return (wsp);
}
/***************************************************************/
struct wssoapop * new_wssoapop(int optyp) {

    struct wssoapop * wsbiso;

    wsbiso = New(struct wssoapop, 1);
    wsbiso->wsbiso_optyp     = optyp;
    wsbiso->wsbiso_soap_body_use        = NULL;
    wsbiso->wsbiso_soap_header_message  = NULL;
    wsbiso->wsbiso_soap_header_part     = NULL;
    wsbiso->wsbiso_soap_header_use      = NULL;
    wsbiso->wsbiso_soap_fault_name      = NULL;
    wsbiso->wsbiso_soap_fault_use       = NULL;

    return (wsbiso);
}
/***************************************************************/
struct wsbopinfo * new_wsbopinfo(int btyp) {

    struct wsbopinfo * wsbi;

    wsbi = New(struct wsbopinfo, 1);
    wsbi->wsbi_btyp     = btyp;
    wsbi->wsbi_nwssoapop = 0;
    wsbi->wsbi_awssoapop = NULL;

    return (wsbi);
}
/***************************************************************/
struct wsboperation * new_wsboperation(struct wsbinding * wsb, int binding_type) {

    struct wsboperation * wsbo;

    wsbo = New(struct wsboperation, 1);
    wsbo->wsbo_wsbinding_ref = wsb;
    wsbo->wsbo_binding_type = binding_type;
    wsbo->wsbo_name = NULL;
    wsbo->wsbo_soap_soapAction = NULL;
    wsbo->wsbo_soap_style = NULL;
    wsbo->wsbo_nwsbopinfo = 0;
    wsbo->wsbo_awsbopinfo = NULL;

    return (wsbo);
}
/***************************************************************/
struct wsboplist * new_wsboplist(void) {

    struct wsboplist * wsbl;

    wsbl = New(struct wsboplist, 1);
    wsbl->wsbl_nwsboperation = 0;
    wsbl->wsbl_awsboperation = NULL;

    return (wsbl);
}
/***************************************************************/
struct wsbinding * new_wsbinding(struct wsdef * wsd) {

    struct wsbinding * wsb;

    wsb = New(struct wsbinding, 1);
    wsb->wsb_name               = NULL;
    wsb->wsb_qname              = NULL;
    wsb->wsb_wsdef_ref          = wsd;
    wsb->wsb_type               = NULL;
    wsb->wsb_binding_type       = 0;
    wsb->wsb_soap_transport     = NULL;

    return (wsb);
}
/***************************************************************/
struct wsservice * new_wsservice(struct wsdef * wsd) {

    struct wsservice * wss;

    wss = New(struct wsservice, 1);
    wss->wss_name = NULL;
    wss->wss_cdata = NULL;
    wss->wss_wsdef_ref          = wsd;

    return (wss);
}
/***************************************************************/
struct wsdef * new_wsdef(void) {

    struct wsdef * wsd;

    wsd = New(struct wsdef, 1);
    wsd->wsd_wstypes        = NULL;
    wsd->wsd_wsmessage_dbt  = NULL;
    wsd->wsd_wsporttype_dbt = NULL;
    wsd->wsd_wsbinding_dbt  = NULL;
    wsd->wsd_wsservice_dbt  = NULL;

    wsd->wsd_name               = NULL;
    wsd->wsd_targetNamespace    = NULL;
    wsd->wsd_tns                = NULL;
    wsd->wsd_nslist             = new_nslist(NULL);
    wsd->wsd_name_space[0]      = '\0';

    return (wsd);
}
/***************************************************************/
struct wsglob * new_wsglob(void) {

    struct wsglob * wsg;

    wsg = New(struct wsglob, 1);
    wsg->wsg_nerrs              = 0;
    wsg->wsg_errs               = NULL;
    wsg->wsg_wsd                = NULL;
    wsg->wsg_wssopl             = NULL;

    return (wsg);
}
/***************************************************************/
/* WSDL parsing destructors                                    */
/***************************************************************/
void free_wscontent(struct wscontent * wstc);
/***************************************************************/
void free_wsdocumentation(struct wsdocumentation * wstd) {

    Free(wstd->wstd_source);
    Free(wstd->wstd_xml_lang);
    Free(wstd->wstd_cdata);

    Free(wstd);
}
/***************************************************************/
void free_wsappinfo(struct wsappinfo * wsti) {

    Free(wsti->wsti_source);
    Free(wsti->wsti_cdata);

    Free(wsti);
}
/***************************************************************/
void free_wsannotation(struct wsannotation * wsta) {

    int ii;

    Free(wsta->wsta_id);

    for (ii = 0; ii < wsta->wsta_ncontent; ii++ ) {
        free_wscontent(wsta->wsta_acontent[ii]);
    }
    Free(wsta->wsta_acontent);

    Free(wsta);
}
/***************************************************************/
void free_wsanyattribute(struct wsanyattribute * wsty) {

    int ii;

    Free(wsty->wsty_id);
    Free(wsty->wsty_namespace);
    Free(wsty->wsty_notNamespace);
    Free(wsty->wsty_notQName);
    Free(wsty->wsty_processContents);

    for (ii = 0; ii < wsty->wsty_ncontent; ii++ ) {
        free_wscontent(wsty->wsty_acontent[ii]);
    }
    Free(wsty->wsty_acontent);

    Free(wsty);
}
/***************************************************************/
void free_wsany(struct wsany * wstz) {

    int ii;

    Free(wstz->wstz_id);
    Free(wstz->wstz_maxOccurs);
    Free(wstz->wstz_minOccurs);
    Free(wstz->wstz_namespace);
    Free(wstz->wstz_notNamespace);
    Free(wstz->wstz_notQName);
    Free(wstz->wstz_processContents);

    for (ii = 0; ii < wstz->wstz_ncontent; ii++ ) {
        free_wscontent(wstz->wstz_acontent[ii]);
    }
    Free(wstz->wstz_acontent);

    Free(wstz);
}
/***************************************************************/
void free_wsall(struct wsall * wstwa) {

    int ii;

    Free(wstwa->wstwa_id);
    Free(wstwa->wstwa_maxOccurs);
    Free(wstwa->wstwa_minOccurs);

    for (ii = 0; ii < wstwa->wstwa_ncontent; ii++ ) {
        free_wscontent(wstwa->wstwa_acontent[ii]);
    }
    Free(wstwa->wstwa_acontent);

    Free(wstwa);
}
/***************************************************************/
void free_wschoice(struct wschoice * wstwc) {

    int ii;

    Free(wstwc->wstwc_id);
    Free(wstwc->wstwc_maxOccurs);
    Free(wstwc->wstwc_minOccurs);

    for (ii = 0; ii < wstwc->wstwc_ncontent; ii++ ) {
        free_wscontent(wstwc->wstwc_acontent[ii]);
    }
    Free(wstwc->wstwc_acontent);

    Free(wstwc);
}
/***************************************************************/
void free_wssequence(struct wssequence * wstq) {

    int ii;

    Free(wstq->wstq_id);
    Free(wstq->wstq_maxOccurs);
    Free(wstq->wstq_minOccurs);

    for (ii = 0; ii < wstq->wstq_ncontent; ii++ ) {
        free_wscontent(wstq->wstq_acontent[ii]);
    }
    Free(wstq->wstq_acontent);

    Free(wstq);
}
/***************************************************************/
void free_wselement(struct wselement * wste) {

    int ii;

    Free(wste->wste_abstract);
    Free(wste->wste_block);
    Free(wste->wste_default);
    Free(wste->wste_final);
    Free(wste->wste_fixed);
    Free(wste->wste_form);
    Free(wste->wste_id);
    Free(wste->wste_maxOccurs);
    Free(wste->wste_minOccurs);
    Free(wste->wste_name);
    Free(wste->wste_nillable);
    Free(wste->wste_ref);
    Free(wste->wste_substitutionGroup);
    Free(wste->wste_targetNamespace);
    Free(wste->wste_type);

    for (ii = 0; ii < wste->wste_ncontent; ii++ ) {
        free_wscontent(wste->wste_acontent[ii]);
    }
    Free(wste->wste_acontent);

    Free(wste);
}
/***************************************************************/
void free_wsgroup(struct wsgroup * wstg) {

    int ii;

    Free(wstg->wstg_id);
    Free(wstg->wstg_maxOccurs);
    Free(wstg->wstg_minOccurs);
    Free(wstg->wstg_name);
    Free(wstg->wstg_ref);

    for (ii = 0; ii < wstg->wstg_ncontent; ii++ ) {
        free_wscontent(wstg->wstg_acontent[ii]);
    }
    Free(wstg->wstg_acontent);

    Free(wstg);
}
/***************************************************************/
void free_wscomplextype(struct wscomplextype * wstx) {

    int ii;

    Free(wstx->wstx_abstract);
    Free(wstx->wstx_block);
    Free(wstx->wstx_final);
    Free(wstx->wstx_id);
    Free(wstx->wstx_mixed);
    Free(wstx->wstx_name);
    Free(wstx->wstx_defaultAttributesApply);

    for (ii = 0; ii < wstx->wstx_ncontent; ii++ ) {
        free_wscontent(wstx->wstx_acontent[ii]);
    }
    Free(wstx->wstx_acontent);

    Free(wstx);
}
/***************************************************************/
void free_wscomplexcontent(struct wscomplexcontent * wstk) {

    int ii;

    Free(wstk->wstk_id);
    Free(wstk->wstk_mixed);

    for (ii = 0; ii < wstk->wstk_ncontent; ii++ ) {
        free_wscontent(wstk->wstk_acontent[ii]);
    }
    Free(wstk->wstk_acontent);

    Free(wstk);
}
/***************************************************************/
void free_wslist(struct wslist * wstl) {

    int ii;

    Free(wstl->wstl_id);
    Free(wstl->wstl_itemType);

    for (ii = 0; ii < wstl->wstl_ncontent; ii++ ) {
        free_wscontent(wstl->wstl_acontent[ii]);
    }
    Free(wstl->wstl_acontent);

    Free(wstl);
}
/***************************************************************/
void free_wsunion(struct wsunion * wstv) {

    int ii;

    Free(wstv->wstv_id);
    Free(wstv->wstv_memberTypes);

    for (ii = 0; ii < wstv->wstv_ncontent; ii++ ) {
        free_wscontent(wstv->wstv_acontent[ii]);
    }
    Free(wstv->wstv_acontent);

    Free(wstv);
}
/***************************************************************/
void free_wsextension(struct wsextension * wstn) {

    int ii;

    Free(wstn->wstn_base);
    Free(wstn->wstn_id);

    for (ii = 0; ii < wstn->wstn_ncontent; ii++ ) {
        free_wscontent(wstn->wstn_acontent[ii]);
    }
    Free(wstn->wstn_acontent);

    Free(wstn);
}
/***************************************************************/
void free_wssimplecontent(struct wssimplecontent * wstp) {

    int ii;

    Free(wstp->wstp_id);

    for (ii = 0; ii < wstp->wstp_ncontent; ii++ ) {
        free_wscontent(wstp->wstp_acontent[ii]);
    }
    Free(wstp->wstp_acontent);

    Free(wstp);
}
/***************************************************************/
void free_wsattribute(struct wsattribute * wstb) {

    int ii;

    Free(wstb->wstb_default);
    Free(wstb->wstb_fixed);
    Free(wstb->wstb_form);
    Free(wstb->wstb_id);
    Free(wstb->wstb_name);
    Free(wstb->wstb_ref);
    Free(wstb->wstb_targetNamespace);
    Free(wstb->wstb_type);
    Free(wstb->wstb_use);
    Free(wstb->wstb_inheritable);

    for (ii = 0; ii < wstb->wstb_ncontent; ii++ ) {
        free_wscontent(wstb->wstb_acontent[ii]);
    }
    Free(wstb->wstb_acontent);

    Free(wstb);
}
/***************************************************************/
void free_wsimport(struct wsimport * wstf) {

    int ii;

    Free(wstf->wstf_id);
    Free(wstf->wstf_namespace);
    Free(wstf->wstf_schemaLocation);

    for (ii = 0; ii < wstf->wstf_ncontent; ii++ ) {
        free_wscontent(wstf->wstf_acontent[ii]);
    }
    Free(wstf->wstf_acontent);

    Free(wstf);
}
/***************************************************************/
void free_wssimpletype(struct wssimpletype * wstt) {

    int ii;

    Free(wstt->wstt_final);
    Free(wstt->wstt_id);
    Free(wstt->wstt_name);

    for (ii = 0; ii < wstt->wstt_ncontent; ii++ ) {
        free_wscontent(wstt->wstt_acontent[ii]);
    }
    Free(wstt->wstt_acontent);

    Free(wstt);
}
/***************************************************************/
void free_wsrestriction(struct wsrestriction * wstr) {

    int ii;

    Free(wstr->wstr_base);
    Free(wstr->wstr_id);

    for (ii = 0; ii < wstr->wstr_ncontent; ii++ ) {
        free_wscontent(wstr->wstr_acontent[ii]);
    }
    Free(wstr->wstr_acontent);

    Free(wstr);
}
/***************************************************************/
void free_wsenumeration(struct wsenumeration * wstu) {

    Free(wstu->wstu_id);
    Free(wstu->wstu_value);

    Free(wstu);
}
/***************************************************************/
void free_wscontent(struct wscontent * wstc) {

    if (!wstc) return;

    Free(wstc->wstc_qname);

    switch (wstc->wstc_content_type) {
        case WSST_ELEMENT :
            free_wselement(wstc->wstc_itm.wstcu_wselement);
            break;
        case WSST_COMPLEX_TYPE :
            free_wscomplextype(wstc->wstc_itm.wstcu_wscomplextype);
            break;
        case WSST_ANY :
            free_wsany(wstc->wstc_itm.wstcu_wsany);
            break;
        case WSST_ANY_ATTRIBUTE :
            free_wsanyattribute(wstc->wstc_itm.wstcu_wsanyattribute);
            break;
        case WSST_SEQUENCE :
            free_wssequence(wstc->wstc_itm.wstcu_wssequence);
            break;
        case WSST_SIMPLE_TYPE :
            free_wssimpletype(wstc->wstc_itm.wstcu_wssimpletype);
            break;
        case WSST_RESTRICTION :
            free_wsrestriction(wstc->wstc_itm.wstcu_wsrestriction);
            break;
        case WSST_ENUMERATION :
            free_wsenumeration(wstc->wstc_itm.wstcu_wsenumeration);
            break;
        case WSST_COMPLEX_CONTENT :
            free_wscomplexcontent(wstc->wstc_itm.wstcu_wscomplexcontent);
            break;
        case WSST_EXTENSION :
            free_wsextension(wstc->wstc_itm.wstcu_wsextension);
            break;
        case WSST_SIMPLE_CONTENT :
            free_wssimplecontent(wstc->wstc_itm.wstcu_wssimplecontent);
            break;
        case WSST_ATTRIBUTE :
            free_wsattribute(wstc->wstc_itm.wstcu_wsattribute);
            break;
        case WSST_LIST :
            free_wslist(wstc->wstc_itm.wstcu_wslist);
            break;
        case WSST_UNION :
            free_wsunion(wstc->wstc_itm.wstcu_wsunion);
            break;
        case WSST_ANNOTATION :
            free_wsannotation(wstc->wstc_itm.wstcu_wsannotation);
            break;
        case WSST_APPINFO :
            free_wsappinfo(wstc->wstc_itm.wstcu_wsappinfo);
            break;
        case WSST_DOCUMENTATION :
            free_wsdocumentation(wstc->wstc_itm.wstcu_wsdocumentation);
            break;
        case WSST_ALL :
            free_wsall(wstc->wstc_itm.wstcu_wsall);
            break;
        case WSST_CHOICE :
            free_wschoice(wstc->wstc_itm.wstcu_wschoice);
            break;
        case WSST_GROUP :
            free_wsgroup(wstc->wstc_itm.wstcu_wsgroup);
            break;
        default:
            break;
    }

    Free(wstc);
}
/***************************************************************/
void free_wsschematype(struct wsschematype * wsst) {

    Free(wsst->wsst_sch_itm_name);
    free_wscontent(wsst->wsst_sch_content);

    Free(wsst);
}
/***************************************************************/
void freev_wsschematype(void * vwste) {

    free_wsschematype((struct wsschematype *)vwste);
}
/***************************************************************/
void free_wsschema(struct wsschema * wsts) {

    if (wsts->wsts_schematype_dbt)
        dbtree_free(wsts->wsts_schematype_dbt, freev_wsschematype);

    Free(wsts->wsts_attributeFormDefault);
    Free(wsts->wsts_blockDefault);
    Free(wsts->wsts_defaultAttributes);
    Free(wsts->wsts_xpathDefaultNamespace);
    Free(wsts->wsts_elementFormDefault);
    Free(wsts->wsts_finalDefault);
    Free(wsts->wsts_id);
    Free(wsts->wsts_targetNamespace);
    Free(wsts->wsts_version);
    Free(wsts->wsts_xml_lang);
    Free(wsts->wsts_tns);
    free_nslist(wsts->wsts_nslist);

    Free(wsts);
}
/***************************************************************/
void free_vwsschema(void * vwsts) {

    struct wsschema * wsts = (struct wsschema *)vwsts;

    free_wsschema(wsts);
}
/***************************************************************/
void free_wstypes(struct wstypes * wst) {

    dbtree_free(wst->wst_wsschema_dbt, free_vwsschema);

    Free(wst);
}
/***************************************************************/
void free_wspart(struct wspart * wsmp) {

    Free(wsmp->wsmp_name);
    Free(wsmp->wsmp_element);
    Free(wsmp->wsmp_type);
    Free(wsmp);
}
/***************************************************************/
void free_wsmessage(struct wsmessage * wsm) {

    int ii;

    for (ii = 0; ii < wsm->wsm_nwspart; ii++) {
        free_wspart(wsm->wsm_awspart[ii]);
    }
    Free(wsm->wsm_awspart);

    Free(wsm->wsm_qname);
    Free(wsm->wsm_name);
    Free(wsm);
}
/***************************************************************/
void freev_wsmessage(void * vwsm) {

    free_wsmessage((struct wsmessage *)vwsm);
}
/***************************************************************/
void free_wspopinfo(struct wspopinfo * wsyi) {

    Free(wsyi->wsyi_name);
    Free(wsyi->wsyi_message);
    Free(wsyi->wsyi_cdata);
    Free(wsyi);
}
/***************************************************************/
void free_wspoperation(struct wspoperation * wsyo) {

    int ii;

    for (ii = 0; ii < wsyo->wsyo_nwspopinfo; ii++) {
        free_wspopinfo(wsyo->wsyo_awspopinfo[ii]);
    }
    Free(wsyo->wsyo_awspopinfo);

    Free(wsyo->wsyo_name);

    Free(wsyo);
}
/***************************************************************/
void free_wsyoplist(struct wsyoplist * wsyl) {

    int ii;

    for (ii = 0; ii < wsyl->wsyl_nwspoperation; ii++) {
        free_wspoperation(wsyl->wsyl_awspoperation[ii]);
    }
    Free(wsyl->wsyl_awspoperation);

    Free(wsyl);
}
/***************************************************************/
void freev_wsyoplist(void * vwsyl) {

    free_wsyoplist((struct wsyoplist *)vwsyl);
}
/***************************************************************/
void free_wsporttype(struct wsporttype * wsy) {

    if (wsy->wsy_wsyoplist_dbt)
        dbtree_free(wsy->wsy_wsyoplist_dbt, freev_wsyoplist);

    Free(wsy->wsy_name);
    Free(wsy->wsy_qname);
    Free(wsy);
}
/***************************************************************/
void freev_wsporttype(void * vwsy) {

    free_wsporttype((struct wsporttype *)vwsy);
}
/***************************************************************/
void free_wssoapop(struct wssoapop * wsbiso) {

    Free(wsbiso->wsbiso_soap_body_use);
    Free(wsbiso->wsbiso_soap_header_message);
    Free(wsbiso->wsbiso_soap_header_part);
    Free(wsbiso->wsbiso_soap_header_use);
    Free(wsbiso->wsbiso_soap_fault_name);
    Free(wsbiso->wsbiso_soap_fault_use);
    Free(wsbiso);
}
/***************************************************************/
void free_wsbopinfo(struct wsbopinfo * wsbi) {


    int ii;

    for (ii = 0; ii < wsbi->wsbi_nwssoapop; ii++) {
        free_wssoapop(wsbi->wsbi_awssoapop[ii]);
    }
    Free(wsbi->wsbi_awssoapop);

    Free(wsbi);
}
/***************************************************************/
void free_wsboperation(struct wsboperation * wsbo) {

    int ii;

    for (ii = 0; ii < wsbo->wsbo_nwsbopinfo; ii++) {
        free_wsbopinfo(wsbo->wsbo_awsbopinfo[ii]);
    }
    Free(wsbo->wsbo_awsbopinfo);

    Free(wsbo->wsbo_name);
    Free(wsbo->wsbo_soap_soapAction);
    Free(wsbo->wsbo_soap_style);
    Free(wsbo);
}
/***************************************************************/
void free_wsboplist(struct wsboplist * wsbl) {

    int ii;

    for (ii = 0; ii < wsbl->wsbl_nwsboperation; ii++) {
        free_wsboperation(wsbl->wsbl_awsboperation[ii]);
    }
    Free(wsbl->wsbl_awsboperation);

    Free(wsbl);
}
/***************************************************************/
void freev_wsboplist(void * vwsbl) {

    free_wsboplist((struct wsboplist *)vwsbl);
}
/***************************************************************/
void free_wsbinding(struct wsbinding * wsb) {

    Free(wsb->wsb_name);
    Free(wsb->wsb_qname);
    Free(wsb->wsb_type);
    Free(wsb->wsb_soap_transport);

    if (wsb->wsb_wsboplist_dbt)
        dbtree_free(wsb->wsb_wsboplist_dbt, freev_wsboplist);

    Free(wsb);
}
/***************************************************************/
void freev_wsbinding(void * vwsb) {

    free_wsbinding((struct wsbinding *)vwsb);
}
/***************************************************************/
void free_wsport(struct wsport * wsp) {

    Free(wsp->wsp_name);
    Free(wsp->wsp_binding);
    Free(wsp->wsp_location);
    Free(wsp->wsp_port_name_space);
    Free(wsp);
}
/***************************************************************/
void freev_wsport(void * vwsp) {

    free_wsport((struct wsport *)vwsp);
}
/***************************************************************/
void free_wsservice(struct wsservice * wss) {

    if (wss->wss_wsport_dbt)
        dbtree_free(wss->wss_wsport_dbt, freev_wsport);

    Free(wss->wss_name);
    Free(wss->wss_cdata);
    Free(wss);
}
/***************************************************************/
void freev_wsservice(void * vwss) {

    free_wsservice((struct wsservice *)vwss);
}
/***************************************************************/
void free_wsdef(struct wsdef * wsd) {

    if (!wsd) return;

    free_nslist(wsd->wsd_nslist);

    Free(wsd->wsd_name);
    Free(wsd->wsd_targetNamespace);
    Free(wsd->wsd_tns);

    if (wsd->wsd_wstypes)       free_wstypes(wsd->wsd_wstypes);

    if (wsd->wsd_wsmessage_dbt)
        dbtree_free(wsd->wsd_wsmessage_dbt, freev_wsmessage);

    if (wsd->wsd_wsporttype_dbt)
        dbtree_free(wsd->wsd_wsporttype_dbt, freev_wsporttype);

    if (wsd->wsd_wsbinding_dbt)
        dbtree_free(wsd->wsd_wsbinding_dbt, freev_wsbinding);

    if (wsd->wsd_wsservice_dbt)
        dbtree_free(wsd->wsd_wsservice_dbt, freev_wsservice);

    Free(wsd);
}
/***************************************************************/
/* WSDL add functions                                          */
/***************************************************************/
struct wsschema * add_new_wsschema(struct wstypes * wst, char * tns) {

    struct wsschema * wsts;

    wsts = new_wsschema(wst);
    if (tns) {
        wsts->wsts_tns = Strdup(tns);
    }

    wst->wst_num_wsschema += 1;

    if (tns) {
        dbtree_insert(wst->wst_wsschema_dbt, tns, IStrlen(tns) + 1, wsts);
    } else {
        dbtree_insert(wst->wst_wsschema_dbt, "", 1, wsts);
    }

    return (wsts);
}
/***************************************************************/
struct wspart * add_new_wspart(struct wsmessage * wsm) {

    struct wspart * wsmp;

    wsmp = new_wspart();
    wsm->wsm_awspart =
        Realloc(wsm->wsm_awspart, struct wspart*, wsm->wsm_nwspart + 1);
    wsm->wsm_awspart[wsm->wsm_nwspart] = wsmp;
    wsm->wsm_nwspart += 1;

    return (wsmp);
}
/***************************************************************/
/*
struct wselement * new_wsst_element(struct wsschematype * wsst) {

    struct wscontent * wstc;
    struct wselement * wste;

    wstc = new_wscontent(WSST_SCHEMA, WSST_ELEMENT);
    wste = new_wselement();
    wstc->wstc_itm.wstcu_wselement = wste;
    wsst->wsst_sch_content = wstc;

    return (wste);
}
*/
/***************************************************************/
#ifdef OLD_WAY
char * calc_qname(struct nslist * nsl,
            const char * tns,
            const char * element_name) {

    int tnslen;
    int ellen;
    char * qname;

    if (!tns || !tns[0]) {
        qname = Strdup(element_name);
    } else {
        tnslen = IStrlen(tns);
        ellen  = IStrlen(element_name);
        qname = New(char, tnslen + ellen + 2);
        memcpy(qname, tns, tnslen);
        qname[tnslen] = ':';
        memcpy(qname + tnslen + 1, element_name, ellen + 1);
    }

    return (qname);
}
/***************************************************************/
char * get_normalized_name(struct nslist * nsl, const char * qname)
{
    int nslen;
    int qnlen;
    int norm_len;
    char * ns_uri;
    char * norm_ns;
    char * norm_name;
    char name_space[XML_NAMESPACE_SZ];

    norm_ns = NULL;
    ns_uri  = NULL;
    xml_get_name_space(qname, name_space, XML_NAMESPACE_SZ);
    ns_uri = nsl_find_name_space(nsl, name_space);
    if (ns_uri) norm_ns = nsl_find_name_space_uri(nsl, ns_uri);

    if (!norm_ns) {
        norm_name = Strdup(qname);
    } else {
        nslen = IStrlen(name_space);
        qnlen = IStrlen(qname);
        norm_len = IStrlen(norm_ns);
        norm_name = New(char, qnlen + norm_len - nslen + 2);
        if (norm_len) {
            memcpy(norm_name, norm_ns, norm_len);
            norm_name[norm_len] = ':';
            strcpy(norm_name + norm_len, qname + nslen + 1);
        } else {
            strcpy(norm_name, qname + nslen + 1);
        }
    }

    return (norm_name);
}
/***************************************************************/
void ** wsdl_find_qname(struct nslist * nsl,
                        struct dbtree * dbt,
                        const char * qname)
{
/*
** We sometimes need to call get_normalized_name() because the
** name space used to index the dbtree is not the same as the one
** we are looking for, but each points to the same uri.  Therefore
** it needs to be able to find it.  One way to solve this problem is
** to use the uri in the dbtree.  This solution attempts to workaround
** doing that.
**
** The ebay wsdl requires this feature, but so far no others do.
*/
    void ** vhand;
    char * norm_name;

    vhand = dbtree_find(dbt, (char*)qname, IStrlen(qname) + 1);
    if (!vhand) {
        norm_name = get_normalized_name(nsl, qname);
        vhand = dbtree_find(dbt, norm_name, IStrlen(norm_name) + 1);
        Free(norm_name);
    }

    return (vhand);
}
#endif /* OLD_WAY */
/***************************************************************/
char * calc_qname(struct nslist * nsl,
            const char * tns,
            const char * element_name) {

    char * qname;
    char * nsuri;
    char * name_space;

    if (!tns || !tns[0]) {
        nsuri = nsl_find_name_space(nsl, "", 1);
    } else {
        nsuri = nsl_find_name_space(nsl, tns, 1);
    }

    if (!nsuri) {
        qname = Strdup(element_name);
    } else {
        name_space = nsl_find_name_space_uri(nsl, nsuri, 1);
        if (!name_space || !name_space[0]) {
            qname = Strdup(element_name);
        } else {
            qname = Smprintf("%s:%s", name_space, element_name);
        }
    }

    return (qname);
}
/***************************************************************/
char * calc_reqname(struct nslist * nsl,
            const char * qname) {

    char * reqname;
    const char * element_name;
    char name_space[XML_NAMESPACE_SZ];

    xml_get_name_space(qname, name_space, XML_NAMESPACE_SZ);

    if (!name_space[0]) {
        element_name = qname;
    } else {
        element_name = qname + strlen(name_space) + 1;
    }

    reqname = calc_qname(nsl, name_space, element_name);

    return (reqname);
}
/***************************************************************/
void ** wsdl_find_qname(struct nslist * nsl,
                        struct dbtree * dbt,
                        const char * qname)
{
    void ** vhand;
    char * reqname;

    reqname = calc_reqname(nsl, qname);

    vhand = dbtree_find(dbt, (char*)reqname, IStrlen(reqname) + 1);
    Free(reqname);

    return (vhand);
}
/***************************************************************/
char * find_qname_uri(struct wstypes * wst,
            const char * qname) {

/*
** I am not sure if we should check the schema(s) for the namespace.
*/
    char * nsuri;
    char name_space[XML_NAMESPACE_SZ];

#ifdef CHECK_SCHEMAS
    struct wsschema * wsts;
    void ** vhand;

    xml_get_name_space(qname, name_space, XML_NAMESPACE_SZ);
    nsuri = NULL;
    vhand = dbtree_find(wst->wst_wsschema_dbt,
            name_space, IStrlen(name_space) + 1);
    if (vhand) {
        wsts = *((struct wsschema **)vhand);
        nsuri = nsl_find_name_space(wsts->wsts_nslist, name_space, 1);
    } else {
        nsuri = nsl_find_name_space(wst->wst_wsdef_ref->wsd_nslist, name_space, 1);
    }
#else
    xml_get_name_space(qname, name_space, XML_NAMESPACE_SZ);
    nsuri = nsl_find_name_space(wst->wst_wsdef_ref->wsd_nslist, name_space, 1);
#endif

    return (nsuri);
}
/***************************************************************/
struct wscontent * wsdl_find_wscontent(struct wstypes * wst,
                        const char * qname)
{
    void ** vhand;
    struct wsschematype * wsst;
    struct wscontent * wstc;
    struct wsschema * wsts;
    char name_space[XML_NAMESPACE_SZ];

    wstc = NULL;
    xml_get_name_space(qname, name_space, XML_NAMESPACE_SZ);
    vhand = dbtree_find(wst->wst_wsschema_dbt,
            name_space, IStrlen(name_space) + 1);
    if (vhand) {
        wsts = *((struct wsschema **)vhand);
        vhand = wsdl_find_qname(wsts->wsts_nslist,
                   wsts->wsts_schematype_dbt, qname);
        if (vhand) {
            wsst = *((struct wsschematype **)vhand);
            wstc = wsst->wsst_sch_content;
        }
    }

    return (wstc);
}
/***************************************************************/
struct wscontent * add_new_schema_content(struct wsschema * wsts,
                            char * element_name,
                            int content_type) {

/*
** Not sure how to handle duplicates.  For example, this duplicate
** appears in ChannelAdvisor OrderService:
**
**      <s:element name="APICredentials" type="tns:APICredentials" />
**      <s:complexType name="APICredentials">
**        <s:sequence>
**          <s:element minOccurs="0" maxOccurs="1" name="DeveloperKey" type="s:string" />
**          <s:element minOccurs="0" maxOccurs="1" name="Password" type="s:string" />
**        </s:sequence>
**        <s:anyAttribute />
**      </s:complexType>
**
** This also appears:
**
**      <s:complexType name="ArrayOfInt">
**        <s:sequence>
**          <s:element minOccurs="0" maxOccurs="unbounded" name="int" type="s:int" />
**        </s:sequence>
**      </s:complexType>
**
**    -- later --
**
**      <s:complexType name="ArrayOfInt">
**        <s:sequence>
**          <s:element minOccurs="0" maxOccurs="unbounded" name="int" type="s:int" />
**        </s:sequence>
**      </s:complexType>
**
** 11/02/2015: For now, just delete prior version
**
*/
    struct wscontent * wstc;
    struct wsschematype * wsst;
    struct wsschematype * ewsst;
    void ** vhand;
    int nlen;
    int is_dup;
    char * qname;

    qname = calc_qname(wsts->wsts_nslist,
        wsts->wsts_tns, element_name);
    nlen = strlen(qname);
    wstc = new_wscontent(WSST_SCHEMA, content_type, qname);

    if (!wsts->wsts_schematype_dbt) {
        wsts->wsts_schematype_dbt = dbtree_new();
        vhand = NULL;
    } else {
        vhand = dbtree_find(wsts->wsts_schematype_dbt,
                        qname, nlen + 1);
    }

    if (vhand) {
        ewsst = *((struct wsschematype **)vhand);
        free_wscontent(ewsst->wsst_sch_content);
        ewsst->wsst_sch_content = wstc;
    } else {
        wsst = new_wsschematype(qname, nlen);
        is_dup = dbtree_insert(wsts->wsts_schematype_dbt,
                        qname, nlen + 1, wsst);
        if (is_dup) {
            free_wsschematype(wsst);
            Free(wstc);
            wstc = NULL;                
        } else {
            wsst->wsst_sch_content = wstc;
        }
    }

    return (wstc);
}
/***************************************************************/
struct wscontent * add_new_element_content(struct wselement * wste,
                int content_type) {

    struct wscontent * wstc;

    wstc = new_wscontent(WSST_ELEMENT, content_type, NULL);
    wste->wste_acontent =
        Realloc(wste->wste_acontent, struct wscontent*,wste->wste_ncontent + 1);
    wste->wste_acontent[wste->wste_ncontent] = wstc;
    wste->wste_ncontent += 1;

    return (wstc);
}
/***************************************************************/
struct wscontent * add_new_complex_type_content(struct wscomplextype * wstx,
                int content_type) {

    struct wscontent * wstc;

    wstc = new_wscontent(WSST_COMPLEX_TYPE, content_type, NULL);
    wstx->wstx_acontent =
        Realloc(wstx->wstx_acontent, struct wscontent*,wstx->wstx_ncontent + 1);
    wstx->wstx_acontent[wstx->wstx_ncontent] = wstc;
    wstx->wstx_ncontent += 1;

    return (wstc);
}
/***************************************************************/
struct wscontent * add_new_sequence_content(struct wssequence * wstq,
                int content_type) {

    struct wscontent * wstc;

    wstc = new_wscontent(WSST_SEQUENCE, content_type, NULL);
    wstq->wstq_acontent =
        Realloc(wstq->wstq_acontent, struct wscontent*,wstq->wstq_ncontent + 1);
    wstq->wstq_acontent[wstq->wstq_ncontent] = wstc;
    wstq->wstq_ncontent += 1;

    return (wstc);
}
/***************************************************************/
struct wscontent * add_new_extension_content(struct wsextension * wstn,
                int content_type) {

    struct wscontent * wstc;

    wstc = new_wscontent(WSST_SEQUENCE, content_type, NULL);
    wstn->wstn_acontent =
        Realloc(wstn->wstn_acontent, struct wscontent*,wstn->wstn_ncontent + 1);
    wstn->wstn_acontent[wstn->wstn_ncontent] = wstc;
    wstn->wstn_ncontent += 1;

    return (wstc);
}
/***************************************************************/
struct wscontent * add_new_complex_content_content(struct wscomplexcontent * wstk,
                int content_type) {

    struct wscontent * wstc;

    wstc = new_wscontent(WSST_SEQUENCE, content_type, NULL);
    wstk->wstk_acontent =
        Realloc(wstk->wstk_acontent, struct wscontent*,wstk->wstk_ncontent + 1);
    wstk->wstk_acontent[wstk->wstk_ncontent] = wstc;
    wstk->wstk_ncontent += 1;

    return (wstc);
}
/***************************************************************/
struct wscontent * add_new_simple_content_content(struct wssimplecontent * wstp,
                int content_type) {

    struct wscontent * wstc;

    wstc = new_wscontent(WSST_SEQUENCE, content_type, NULL);
    wstp->wstp_acontent =
        Realloc(wstp->wstp_acontent, struct wscontent*,wstp->wstp_ncontent + 1);
    wstp->wstp_acontent[wstp->wstp_ncontent] = wstc;
    wstp->wstp_ncontent += 1;

    return (wstc);
}
/***************************************************************/
struct wscontent * add_new_list_content(struct wslist * wstl,
                int content_type) {

    struct wscontent * wstc;

    wstc = new_wscontent(WSST_LIST, content_type, NULL);
    wstl->wstl_acontent =
        Realloc(wstl->wstl_acontent, struct wscontent*,wstl->wstl_ncontent + 1);
    wstl->wstl_acontent[wstl->wstl_ncontent] = wstc;
    wstl->wstl_ncontent += 1;

    return (wstc);
}
/***************************************************************/
struct wscontent * add_new_annotation_content(struct wsannotation * wsta,
                int content_type) {

    struct wscontent * wstc;

    wstc = new_wscontent(WSST_SEQUENCE, content_type, NULL);
    wsta->wsta_acontent =
        Realloc(wsta->wsta_acontent, struct wscontent*,wsta->wsta_ncontent + 1);
    wsta->wsta_acontent[wsta->wsta_ncontent] = wstc;
    wsta->wsta_ncontent += 1;

    return (wstc);
}
/***************************************************************/
struct wscontent * add_new_union_content(struct wsunion * wstv,
                int content_type) {

    struct wscontent * wstc;

    wstc = new_wscontent(WSST_UNION, content_type, NULL);
    wstv->wstv_acontent =
        Realloc(wstv->wstv_acontent, struct wscontent*,wstv->wstv_ncontent + 1);
    wstv->wstv_acontent[wstv->wstv_ncontent] = wstc;
    wstv->wstv_ncontent += 1;

    return (wstc);
}
/***************************************************************/
struct wscontent * add_new_attribute_content(struct wsattribute * wstb,
                int content_type) {

    struct wscontent * wstc;

    wstc = new_wscontent(WSST_ATTRIBUTE, content_type, NULL);
    wstb->wstb_acontent =
        Realloc(wstb->wstb_acontent, struct wscontent*,wstb->wstb_ncontent + 1);
    wstb->wstb_acontent[wstb->wstb_ncontent] = wstc;
    wstb->wstb_ncontent += 1;

    return (wstc);
}
/***************************************************************/
struct wscontent * add_new_anyattribute_content(struct wsanyattribute * wsty,
                int content_type) {

    struct wscontent * wstc;

    wstc = new_wscontent(WSST_ANY_ATTRIBUTE, content_type, NULL);
    wsty->wsty_acontent =
        Realloc(wsty->wsty_acontent, struct wscontent*,wsty->wsty_ncontent + 1);
    wsty->wsty_acontent[wsty->wsty_ncontent] = wstc;
    wsty->wsty_ncontent += 1;

    return (wstc);
}
/***************************************************************/
struct wscontent * add_new_any_content(struct wsany * wstz,
                int content_type) {

    struct wscontent * wstc;

    wstc = new_wscontent(WSST_ANY, content_type, NULL);
    wstz->wstz_acontent =
        Realloc(wstz->wstz_acontent, struct wscontent*,wstz->wstz_ncontent + 1);
    wstz->wstz_acontent[wstz->wstz_ncontent] = wstc;
    wstz->wstz_ncontent += 1;

    return (wstc);
}
/***************************************************************/
struct wscontent * add_new_all_content(struct wsall * wstwa,
                int content_type) {

    struct wscontent * wstc;

    wstc = new_wscontent(WSST_ALL, content_type, NULL);
    wstwa->wstwa_acontent =
        Realloc(wstwa->wstwa_acontent, struct wscontent*,wstwa->wstwa_ncontent + 1);
    wstwa->wstwa_acontent[wstwa->wstwa_ncontent] = wstc;
    wstwa->wstwa_ncontent += 1;

    return (wstc);
}
/***************************************************************/
struct wscontent * add_new_group_content(struct wsgroup * wstg,
                int content_type) {

    struct wscontent * wstc;

    wstc = new_wscontent(WSST_GROUP, content_type, NULL);
    wstg->wstg_acontent =
        Realloc(wstg->wstg_acontent, struct wscontent*,wstg->wstg_ncontent + 1);
    wstg->wstg_acontent[wstg->wstg_ncontent] = wstc;
    wstg->wstg_ncontent += 1;

    return (wstc);
}
/***************************************************************/
struct wscontent * add_new_choice_content(struct wschoice * wstwc,
                int content_type) {

    struct wscontent * wstc;

    wstc = new_wscontent(WSST_CHOICE, content_type, NULL);
    wstwc->wstwc_acontent =
        Realloc(wstwc->wstwc_acontent, struct wscontent*,wstwc->wstwc_ncontent + 1);
    wstwc->wstwc_acontent[wstwc->wstwc_ncontent] = wstc;
    wstwc->wstwc_ncontent += 1;

    return (wstc);
}
/***************************************************************/
struct wscontent * add_new_simple_type_content(struct wssimpletype * wstt,
                int content_type) {

    struct wscontent * wstc;

    wstc = new_wscontent(WSST_SIMPLE_TYPE, content_type, NULL);
    wstt->wstt_acontent =
        Realloc(wstt->wstt_acontent, struct wscontent*,wstt->wstt_ncontent + 1);
    wstt->wstt_acontent[wstt->wstt_ncontent] = wstc;
    wstt->wstt_ncontent += 1;

    return (wstc);
}
/***************************************************************/
struct wscontent * add_new_restriction_content(struct wsrestriction * wstr,
                int content_type) {

    struct wscontent * wstc;

    wstc = new_wscontent(WSST_RESTRICTION, content_type, NULL);
    wstr->wstr_acontent =
        Realloc(wstr->wstr_acontent, struct wscontent*,wstr->wstr_ncontent + 1);
    wstr->wstr_acontent[wstr->wstr_ncontent] = wstc;
    wstr->wstr_ncontent += 1;

    return (wstc);
}
/***************************************************************/
/* WSDL parsing error functions                                */
/***************************************************************/
static char * ws_set_error_msg(struct wsglob * wsg,
        int eref,
        const char * eparm) {

    int eix;
    int emsglen;
    char * emsg;
    char emsgno[32];

    if (eref < 0) {
        switch (eref) {
            case WSE_EXPSCHEMA:
                emsg = Smprintf("Expecting 'schema'. Found '%s'", 
                            eparm);
                break;
            case WSE_BADSCHEMAATTR:
                emsg = Smprintf("Expecting 'schema' attribute for '%s'", 
                            eparm);
                break;
            case WSE_EXPELEMENT:
                emsg = Smprintf("Unexpected schema content. Found '%s'",
                            eparm);
                break;
            case WSE_EXPNAME:
                emsg = Smprintf("Expecting 'name' attribute for '%s'", 
                            eparm);
                break;
            case WSE_DUPELEMENT:
                emsg = Smprintf("Duplicate element. Found '%s'", 
                            eparm);
                break;
            case WSE_BADELEMENTATTR:
                emsg = Smprintf("Unexpected element attribute. Found '%s'",
                            eparm);
                break;
            case WSE_ELEMENTCONTENT:
                emsg = Smprintf("Unexpected element content. Found '%s'",
                            eparm);
                break;
            case WSE_BADCOMPLEXTYPEATTR:
                emsg = Smprintf("Unexpected complex type attribute. Found '%s'",
                            eparm);
                break;
            case WSE_COMPLEXTYPECONTENT:
                emsg = Smprintf("Unexpected complex type content. Found '%s'",
                            eparm);
                break;
            case WSE_BADSEQUENCEATTR:
                emsg = Smprintf("Unexpected sequence attribute. Found '%s'",
                            eparm);
                break;
            case WSE_SEQUENCECONTENT:
                emsg = Smprintf("Unexpected sequence content. Found '%s'",
                            eparm);
                break;
            case WSE_BADSIMPLETYPEATTR:
                emsg = Smprintf("Unexpected simple type attribute. Found '%s'",
                            eparm);
                break;
            case WSE_SIMPLETYPECONTENT:
                emsg = Smprintf("Unexpected simple type content. Found '%s'",
                            eparm);
                break;
            case WSE_BADENUMERATIONATTR:
                emsg = Smprintf("Unexpected enumeration attribute. Found '%s'",
                            eparm);
                break;
            case WSE_ENUMERATIONCONTENT:
                emsg = Smprintf("Unexpected enumeration content. Found '%s'",
                            eparm);
                break;
            case WSE_BADRESTRICTIONATTR:
                emsg = Smprintf("Unexpected restriction attribute. Found '%s'",
                            eparm);
                break;
            case WSE_RESTRICTIONCONTENT:
                emsg = Smprintf("Unexpected restriction content. Found '%s'",
                            eparm);
                break;
            case WSE_BADANYATTRIBUTEATTR:
                emsg = Smprintf("Unexpected anyAttribute attribute. Found '%s'",
                            eparm);
                break;
            case WSE_ANYATTRIBUTECONTENT:
                emsg = Smprintf("Unexpected anyAttribute content. Found '%s'",
                            eparm);
                break;
            case WSE_BADEXTENSIONATTR:
                emsg = Smprintf("Unexpected extension attribute. Found '%s'",
                            eparm);
                break;
            case WSE_EXTENSIONCONTENT:
                emsg = Smprintf("Unexpected extension content. Found '%s'",
                            eparm);
                break;
            case WSE_BADCOMPLEXCONTENTATTR:
                emsg = Smprintf("Unexpected complexContent attribute. Found '%s'",
                            eparm);
                break;
            case WSE_COMPLEXCONTENTCONTENT:
                emsg = Smprintf("Unexpected complexContent content. Found '%s'",
                            eparm);
                break;
            case WSE_BADSIMPLECONTENTATTR:
                emsg = Smprintf("Unexpected simpleContent attribute. Found '%s'",
                            eparm);
                break;
            case WSE_SIMPLECONTENTCONTENT:
                emsg = Smprintf("Unexpected simpleContent content. Found '%s'",
                            eparm);
                break;
            case WSE_BADATTRIBUTEATTR:
                emsg = Smprintf("Unexpected attribute attribute. Found '%s'",
                            eparm);
                break;
            case WSE_ATTRIBUTECONTENT:
                emsg = Smprintf("Unexpected attribute content. Found '%s'",
                            eparm);
                break;
            case WSE_DEFINITIONCONTENT:
                emsg = Smprintf("Unexpected definition content. Found '%s'",
                            eparm);
                break;
            case WSE_MULTIPLETYPES:
                emsg = Smprintf("Multiple types in definition. Found '%s'",
                            eparm);
                break;
            case WSE_MESSAGENAMEREQD:
                emsg = Smprintf("The message name attribute is required.");
                break;
            case WSE_BADMESSAGEATTR:
                emsg = Smprintf("Unexpected message attribute. Found '%s'",
                            eparm);
                break;
            case WSE_MESSAGECONTENT:
                emsg = Smprintf("Unexpected message content. Found '%s'",
                            eparm);
                break;
            case WSE_BADPARTATTR:
                emsg = Smprintf("Unexpected part attribute. Found '%s'",
                            eparm);
                break;
            case WSE_PARTCONTENT:
                emsg = Smprintf("Unexpected part content. Found '%s'",
                            eparm);
                break;
            case WSE_PORTTYPENAMEREQD:
                emsg = Smprintf("The portType name attribute is required.");
                break;
            case WSE_OPERATIONNAMEREQD:
                emsg = Smprintf("The operation name attribute is required.");
                break;
            case WSE_BINDINGNAMEREQD:
                emsg = Smprintf("The binding name attribute is required.");
                break;
            case WSE_SERVICENAMEREQD:
                emsg = Smprintf("The service name attribute is required.");
                break;
            case WSE_BADPORTTYPEATTR:
                emsg = Smprintf("Unexpected portType attribute. Found '%s'",
                            eparm);
                break;
            case WSE_PORTTYPECONTENT:
                emsg = Smprintf("Unexpected portType content. Found '%s'",
                            eparm);
                break;
            case WSE_BADPORTATTR:
                emsg = Smprintf("Unexpected port attribute. Found '%s'",
                            eparm);
                break;
            case WSE_PORTCONTENT:
                emsg = Smprintf("Unexpected port content. Found '%s'",
                            eparm);
                break;
            case WSE_BINDINGCONTENT:
                emsg = Smprintf("Unexpected binding content. Found '%s'",
                            eparm);
                break;
            case WSE_BADSERVICEATTR:
                emsg = Smprintf("Unexpected service attribute. Found '%s'",
                            eparm);
                break;
            case WSE_SERVICECONTENT:
                emsg = Smprintf("Unexpected service content. Found '%s'",
                            eparm);
                break;
            case WSE_NOCONTENTEXPECTED:
                emsg = Smprintf("Unexpected content for '%s'",
                            eparm);
                break;
            case WSE_BADPORTTYPEELATTR:
                emsg = Smprintf("Unexpected attribute of portType content. Found '%s'",
                            eparm);
            case WSE_OPERATIONCONTENT:
                emsg = Smprintf("Unexpected operation content. Found '%s'",
                            eparm);
                break;
            case WSE_SOAPOPCONTENT:
                emsg = Smprintf("Unexpected SOAP operation content. Found '%s'",
                            eparm);
                break;
            case WSE_PORTNAMEREQD:
                emsg = Smprintf("The port name attribute is required.");
                break;
            case WSE_LISTCONTENT:
                emsg = Smprintf("Unexpected list content. Found '%s'",
                            eparm);
                break;
            case WSE_UNIONCONTENT:
                emsg = Smprintf("Unexpected union content. Found '%s'",
                            eparm);
                break;
            case WSE_BADLISTATTR:
                emsg = Smprintf("Unexpected list attribute. Found '%s'",
                            eparm);
                break;
            case WSE_BADUNIONATTR:
                emsg = Smprintf("Unexpected union attribute. Found '%s'",
                            eparm);
                break;
            case WSE_SCHEMANAMESPACE:
                emsg = Smprintf("Invalid schema namespace. Found '%s'",
                            eparm);
                break;
            case WSE_BADANNOTATIONATTR:
                emsg = Smprintf("Unexpected annotation attribute. Found '%s'",
                            eparm);
                break;
            case WSE_ANNOTATIONCONTENT:
                emsg = Smprintf("Unexpected annotation content. Found '%s'",
                            eparm);
                break;
            case WSE_BADAPPINFOATTR:
                emsg = Smprintf("Unexpected appinfo attribute. Found '%s'",
                            eparm);
                break;
            case WSE_APPINFOCONTENT:
                emsg = Smprintf("Unexpected appinfo content. Found '%s'",
                            eparm);
                break;
            case WSE_BADDOCUMENTATIONATTR:
                emsg = Smprintf("Unexpected documentation attribute. Found '%s'",
                            eparm);
                break;
            case WSE_DOCUMENTATIONCONTENT:
                emsg = Smprintf("Unexpected documentation content. Found '%s'",
                            eparm);
                break;
            case WSE_BADALLATTR:
                emsg = Smprintf("Unexpected all attribute. Found '%s'",
                            eparm);
                break;
            case WSE_ALLCONTENT:
                emsg = Smprintf("Unexpected all content. Found '%s'",
                            eparm);
                break;
            case WSE_BADCHOICEATTR:
                emsg = Smprintf("Unexpected choice attribute. Found '%s'",
                            eparm);
                break;
            case WSE_CHOICECONTENT:
                emsg = Smprintf("Unexpected choice content. Found '%s'",
                            eparm);
                break;
            case WSE_BADGROUPATTR:
                emsg = Smprintf("Unexpected group attribute. Found '%s'",
                            eparm);
                break;
            case WSE_GROUPCONTENT:
                emsg = Smprintf("Unexpected group content. Found '%s'",
                            eparm);
                break;
            case WSE_UNIMPLEMENTEDXLS:
                emsg = Smprintf("Unimplemented XML Schema feature '%s'",
                            eparm);
                break;
            case WSE_NOSUCHOPERATION:
                emsg = Smprintf("Operation not found: '%s'",
                            eparm);
                break;
            case WSE_OPERATIONNOTUNIQUE:
                emsg = Smprintf("Operation not unique: '%s'",
                            eparm);
                break;
            case WSE_CALLPARAMETERERROR:
                emsg = Smprintf("Call parameter error: '%s'",
                            eparm);
                break;
            case WSE_UNSUPPORTEDBINDING:
                emsg = Smprintf("Unsupported binding: '%s'",
                            eparm);
                break;
            case WSE_INSUFFICIENTPARMS:
                emsg = Smprintf("Insufficient number of arguments supplied");
                break;
            case WSE_UNIMPLEMENTEDPARM:
                emsg = Smprintf("Unimplemented argument '%s'",
                            eparm);
                break;
            case WSE_TOOMANYPARMS:
                emsg = Smprintf("Too many arguments supplied");
                break;
            case WSE_FINDELEMENTTYPE:
                emsg = Smprintf("Element type not found: '%s'",
                            eparm);
                break;
            case WSE_UNSUPPORTEDPARMTYPE:
                emsg = Smprintf("Unsupported argument type supplied: '%s'",
                            eparm);
                break;
            case WSE_PARMCONVERSION:
                emsg = Smprintf("Unsupported argument conversion: '%s'",
                            eparm);
                break;
            case WSE_FINDNAMESPACE:
                emsg = Smprintf("Namespace not found: '%s'",
                            eparm);
                break;
            case WSE_XMLRESPONSE:
                emsg = Smprintf("XML error reading response: %s",
                            eparm);
                break;
            case WSE_EXPCLOSEPAREN:
                emsg = Smprintf("CALL expecting close parenthesis. Found: %s",
                            eparm);
                break;
            case WSE_UNSUPPORTEDSEQYPE:
                emsg = Smprintf("Unsupported sequence argument type supplied: '%s'",
                            eparm);
                break;
            case WSE_NOSEQCONTENTNAME:
                emsg = Smprintf("Sequence content does not match an object.");
                break;
            case WSE_FINDPARM:
                emsg = Smprintf("Cannot find WSDL argument: '%s'",
                            eparm);
                break;

            /********/
            default:
                emsg = Smprintf("Unknown error with '%s'", eparm);
                break;
        }

        emsglen = IStrlen(emsg);
        emsg = Realloc(emsg, char, emsglen + sizeof(emsgno));
        sprintf(emsgno, " WSERR (%d)", eref);
        strcpy(emsg + emsglen, emsgno);
    } else {
        eix = eref - WS_ERR_IX_DELTA;
        if (eix < 0 || eix >= wsg->wsg_nerrs) return NULL;
        if (!wsg->wsg_errs[eix]) return NULL;

        emsg = Smprintf("%s WSERR %d",
            wsg->wsg_errs[eix]->wsx_errmsg, wsg->wsg_errs[eix]->wsx_errnum);
    }

    return (emsg);
}
/***************************************************************/
int ws_set_error(struct wsglob * wsg, int inwserr, char *fmt, ...) {

    va_list args;
    int eix;
    char * emsg;
    char * eparm;
    struct wserr * wsx;

    va_start (args, fmt);
    eparm = Vsmprintf (fmt, args);
    va_end (args);

    emsg = ws_set_error_msg(wsg, inwserr, eparm);
    Free(eparm);

    eix = 0;
    while (eix < wsg->wsg_nerrs && wsg->wsg_errs[eix]) eix++;

    if (eix == wsg->wsg_nerrs) {
        wsg->wsg_errs = Realloc(wsg->wsg_errs, struct wserr*, wsg->wsg_nerrs + 1);
        eix = wsg->wsg_nerrs;
        wsg->wsg_nerrs += 1;
    }

    wsx = New(struct wserr, 1);
    wsg->wsg_errs[eix] = wsx;

    wsx->wsx_errmsg = emsg;
    wsx->wsx_errnum = inwserr;

    return (eix + WS_ERR_IX_DELTA);
}
/***************************************************************/
int ws_clear_error(struct wsglob * wsg, int eref) {

    int eix;

    if (eref < 0) return (0);

    eix = eref - WS_ERR_IX_DELTA;
    if (eix < 0 || eix >= wsg->wsg_nerrs) return (-1);

    Free(wsg->wsg_errs[eix]->wsx_errmsg);
    Free(wsg->wsg_errs[eix]);
    wsg->wsg_errs[eix] = NULL;

    while (wsg->wsg_nerrs > 0 && wsg->wsg_errs[wsg->wsg_nerrs - 1] == NULL)
        wsg->wsg_nerrs -= 1;

    return (0);
}
/***************************************************************/
void ws_clear_all_errors(struct wsglob * wsg) {

    int eix;

    while (wsg->wsg_nerrs > 0) {
        eix = wsg->wsg_nerrs;
        while (eix > 0 && wsg->wsg_errs[eix - 1] == NULL) eix--;
        if (eix) {
            ws_clear_error(wsg, eix - 1 + WS_ERR_IX_DELTA);
        } else {
            wsg->wsg_nerrs = 0; /* stop loop */
        }
    }

    Free(wsg->wsg_errs);
    wsg->wsg_errs = NULL;
}
/***************************************************************/
const char * ws_get_error_msg(struct wsglob * wsg, int eref) {

    int eix;
    char * emsg;

    eix = eref - WS_ERR_IX_DELTA;
    if (eix < 0 || eix >= wsg->wsg_nerrs) return NULL;
    if (!wsg->wsg_errs[eix]) return NULL;

    emsg = wsg->wsg_errs[eix]->wsx_errmsg;

    return (emsg);
}
/***************************************************************/
static int ws_show_error(struct wsglob * wsg, FILE * outf, int eref) {

    const char * emsg;

    emsg = ws_get_error_msg(wsg, eref);
    if (!emsg) return (-1);

    fprintf(outf, "%s\n", emsg);

    ws_clear_error(wsg, eref);

    return (0);
}
/***************************************************************/
void free_wsglob(struct wsglob * wsg) {

    if (!wsg) return;

    if (wsg->wsg_wssopl) free_wssoplist(wsg->wsg_wssopl);

    free_wsdef(wsg->wsg_wsd);
    
    ws_clear_all_errors(wsg);

    Free(wsg);
}
/***************************************************************/
/* WSDL parsing forward prototypes                             */
/***************************************************************/
int parse_wsdl_sch_complex_type(struct wsglob * wsg,
                    struct xmltree * xtree_wstx,
                    const char * name_space,
                    struct wscomplextype * wstx);

int parse_wsdl_sch_element(struct wsglob * wsg,
                    struct xmltree * xtree_wste,
                    const char * name_space,
                    struct wselement * wste);

int parse_wsdl_sch_sequence(struct wsglob * wsg,
                    struct xmltree * xtree_wstq,
                    const char * name_space,
                    struct wssequence * wstq);

int parse_wsdl_sch_simple_type(struct wsglob * wsg,
                    struct xmltree * xtree_wstt,
                    const char * name_space,
                    struct wssimpletype * wstt);

int parse_restriction_content(struct wsglob * wsg,
                    struct xmltree * xtree_wstr,
                    const char * name_space,
                    struct wsrestriction * wstr);

int parse_wsdl_sch_any_attribute(struct wsglob * wsg,
                    struct xmltree * xtree_wsty,
                    const char * name_space,
                    struct wsanyattribute * wsty);

int parse_wsdl_sch_any(struct wsglob * wsg,
                    struct xmltree * xtree_wstz,
                    const char * name_space,
                    struct wsany * wstz);

int parse_wsdl_sch_extension(struct wsglob * wsg,
                    struct xmltree * xtree_wstn,
                    const char * name_space,
                    struct wsextension * wstn);

int parse_wsdl_sch_all(struct wsglob * wsg,
                    struct xmltree * xtree_wstwa,
                    const char * name_space,
                    struct wsall * wstwa);

int parse_wsdl_sch_choice(struct wsglob * wsg,
                    struct xmltree * xtree_wstwc,
                    const char * name_space,
                    struct wschoice * wstwc);

int parse_wsdl_sch_group(struct wsglob * wsg,
                    struct xmltree * xtree_wstg,
                    const char * name_space,
                    struct wsgroup * wstg);

int parse_wsdl_sch_annotation(struct wsglob * wsg,
                    struct xmltree * xtree_wsta,
                    const char * name_space,
                    struct wsannotation * wsta);

/***************************************************************/
/* WSDL parsing functions                                      */

/***************************************************************/
#if TEST_NAME_SPACES
void nsl_test_name_spaces(struct nslist * nsl) {

    int ii;
    int nerrs;
    char * ns;
    char * nsuri;

    printf("Testing %d namespaces.\n", nsl->nsl_nname_space);

    nerrs = 0;
    for (ii = 0; ii < nsl->nsl_nname_space; ii++) {
        ns = nsl_find_name_space(nsl, nsl->nsl_aname_space[ii]);
        if (!ns) {
            printf("Cannot find name space '%s'\n", nsl->nsl_aname_space[ii]);
            nerrs++;
        }
        nsuri = nsl_find_name_space_uri(nsl, nsl->nsl_aname_space_uri[ii]);
        if (!nsuri) {
            printf("Cannot find name space URI '%s'\n", nsl->nsl_aname_space_uri[ii]);
            nerrs++;
        }
    }

    if (!nerrs) {
        printf("OK.\n");
    } else {
        printf("%d errors.\n", nerrs);
    }
}
#endif
/***************************************************************/
int parse_enumeration_content(struct wsglob * wsg,
                    struct xmltree * xtree_wstu,
                    const char * name_space,
                    struct wsenumeration * wstu) {
/*
  Content: (annotation?, (restriction | list | union))
*/

    int wstat = 0;

    /* TODO: */

    return (wstat);
}
/***************************************************************/
int parse_wsdl_sch_enumeration(struct wsglob * wsg,
                    struct xmltree * xtree_wstu,
                    const char * name_space,
                    struct wsenumeration * wstu) {
/*
4.3.5.2 XML Representation of enumeration Schema Components

<enumeration
  id = ID
  value = anySimpleType
  {any attributes with non-schema namespace . . .}>
  Content: (annotation?)
</enumeration>
*/

    int wstat = 0;
    int atix;

    atix = 0;
    while (!wstat && atix < xtree_wstu->nattrs) {
        if (!strcmp(xtree_wstu->attrnam[atix], WSSH_TYP_ID)) {
            wstu->wstu_id = Strdup(xtree_wstu->attrval[atix]);

        } else if (!strcmp(xtree_wstu->attrnam[atix], WSSH_TYP_VALUE)) {
            wstu->wstu_value = Strdup(xtree_wstu->attrval[atix]);

        } else {
            wstat = ws_set_error(wsg, WSE_BADENUMERATIONATTR,
                    "%s", xtree_wstu->attrnam[atix]);
        }
        atix++;
    }

    if (!wstat && xtree_wstu->nsubel) {
        wstat = parse_enumeration_content(wsg, xtree_wstu, name_space, wstu);
    }

    return (wstat);
}
/***************************************************************/
int parse_base_content(struct wsglob * wsg,
                        struct xmltree * xtree_wstc,
                        const char * name_space,
                        char * base_type,
                        int * p_ncontent,
                        struct wscontent *** p_acontent) {
/*
*/

    int wstat = 0;
    int trix;
    /* struct wscontent * wstc; */

    trix = 0;
    while (!wstat && trix < xtree_wstc->nsubel) {
        trix++;
    }

    return (wstat);
}
/***************************************************************/
int parse_restriction_content(struct wsglob * wsg,
                    struct xmltree * xtree_wstr,
                    const char * name_space,
                    struct wsrestriction * wstr) {
/*
  Content: (annotation?, (simpleType?,
    (minExclusive | minInclusive | maxExclusive | maxInclusive |
     totalDigits | fractionDigits | length | minLength | maxLength |
     enumeration | whiteSpace | pattern | assertion | explicitTimezone |
     {any with namespace: ##other})*))
*/

    int wstat = 0;
    int trix;
    struct wscontent * wstc;

    trix = 0;
    while (!wstat && trix < xtree_wstr->nsubel) {
        if (xml_full_name_equals(xtree_wstr->subel[trix]->elname,
            name_space, WSSH_ENUMERATION)) {
            wstc = add_new_restriction_content(wstr, WSST_ENUMERATION);
            wstc->wstc_itm.wstcu_wsenumeration = new_wsenumeration();
            wstat = parse_wsdl_sch_enumeration(wsg, xtree_wstr->subel[trix],
                name_space, wstc->wstc_itm.wstcu_wsenumeration);

        } else if (xml_name_space_equals(xtree_wstr->subel[trix]->elname,
                    name_space)) {
            wstat = ws_set_error(wsg, WSE_RESTRICTIONCONTENT,
                        "%s", xtree_wstr->subel[trix]->elname);
        }
        trix++;
    }

    return (wstat);
}
/***************************************************************/
int parse_wsdl_sch_restriction(struct wsglob * wsg,
                    struct xmltree * xtree_wstr,
                    const char * name_space,
                    struct wsrestriction * wstr) {
/*
<restriction
  base = QName
  id = ID
  {any attributes with non-schema namespace . . .}>
  Content: (annotation?, (simpleType?,
    (minExclusive | minInclusive | maxExclusive | maxInclusive |
     totalDigits | fractionDigits | length | minLength | maxLength |
     enumeration | whiteSpace | pattern | assertion | explicitTimezone |
     {any with namespace: ##other})*))
</restriction>
*/

    int wstat = 0;
    int atix;

    atix = 0;
    while (!wstat && atix < xtree_wstr->nattrs) {
        if (!strcmp(xtree_wstr->attrnam[atix], WSSH_TYP_BASE)) {
            wstr->wstr_base = Strdup(xtree_wstr->attrval[atix]);

        } else if (!strcmp(xtree_wstr->attrnam[atix], WSSH_TYP_ID)) {
            wstr->wstr_id = Strdup(xtree_wstr->attrval[atix]);

        } else {
            wstat = ws_set_error(wsg, WSE_BADRESTRICTIONATTR,
                    "%s", xtree_wstr->attrnam[atix]);
        }
        atix++;
    }

    if (!wstat && xtree_wstr->nsubel) {
        if (wstr->wstr_base) {
            wstat = parse_base_content(wsg, xtree_wstr, name_space, wstr->wstr_base,
                &(wstr->wstr_ncontent), &(wstr->wstr_acontent));
        } else {
            wstat = parse_restriction_content(wsg, xtree_wstr, name_space, wstr);
        }
    }

    return (wstat);
}
/***************************************************************/
int parse_list_content(struct wsglob * wsg,
                    struct xmltree * xtree_wstl,
                    const char * name_space,
                    struct wslist * wstl) {
/*
  Content: (annotation?, simpleType?)
*/

    int wstat = 0;
    int trix;
    struct wscontent * wstc;

    trix = 0;
    while (!wstat && trix < xtree_wstl->nsubel) {
        if (xml_full_name_equals(xtree_wstl->subel[trix]->elname,
            name_space, WSSH_ANNOTATION)) {
            wstc = add_new_list_content(wstl, WSST_ANNOTATION);
            wstc->wstc_itm.wstcu_wsannotation = new_wsannotation();
            wstat = parse_wsdl_sch_annotation(wsg, xtree_wstl->subel[trix],
                name_space, wstc->wstc_itm.wstcu_wsannotation);

        } else if (xml_full_name_equals(xtree_wstl->subel[trix]->elname,
            name_space, WSSH_SIMPLE_TYPE)) {
            wstc = add_new_list_content(wstl, WSST_SIMPLE_TYPE);
            wstc->wstc_itm.wstcu_wssimpletype = new_wssimpletype();
            wstat = parse_wsdl_sch_simple_type(wsg, xtree_wstl->subel[trix],
                name_space, wstc->wstc_itm.wstcu_wssimpletype);

        } else if (xml_name_space_equals(xtree_wstl->subel[trix]->elname,
                    name_space)) {
            wstat = ws_set_error(wsg, WSE_LISTCONTENT,
                        "%s", xtree_wstl->subel[trix]->elname);
        }
        trix++;
    }

    return (wstat);
}
/***************************************************************/
int parse_wsdl_sch_list(struct wsglob * wsg,
                    struct xmltree * xtree_wstl,
                    const char * name_space,
                    struct wslist * wstl) {
/*
<list
  id = ID
  itemType = QName
  {any attributes with non-schema namespace . . .}>
  Content: (annotation?, simpleType?)
</list>
*/

    int wstat = 0;
    int atix;

    atix = 0;
    while (!wstat && atix < xtree_wstl->nattrs) {
        if (!strcmp(xtree_wstl->attrnam[atix], WSSH_TYP_ID)) {
            wstl->wstl_id = Strdup(xtree_wstl->attrval[atix]);

        } else if (!strcmp(xtree_wstl->attrnam[atix], WSSH_TYP_ITEM_TYPE)) {
            wstl->wstl_itemType = Strdup(xtree_wstl->attrval[atix]);

        } else {
            wstat = ws_set_error(wsg, WSE_BADLISTATTR,
                    "%s", xtree_wstl->attrnam[atix]);
        }
        atix++;
    }

    if (!wstat && xtree_wstl->nsubel) {
        wstat = parse_list_content(wsg, xtree_wstl, name_space, wstl);
    }

    return (wstat);
}
/***************************************************************/
int parse_documentation_content(struct wsglob * wsg,
                    struct xmltree * xtree_wstd,
                    const char * name_space,
                    struct wsdocumentation * wstd) {
/*
  Content: (appinfo | documentation)*
*/

    int wstat = 0;
    int trix;

    trix = 0;
    while (!wstat && trix < xtree_wstd->nsubel) {
        if (xml_name_space_equals(xtree_wstd->subel[trix]->elname,
            name_space)) {
            wstat = ws_set_error(wsg, WSE_DOCUMENTATIONCONTENT,
                        "%s", xtree_wstd->subel[trix]->elname);
        }
        trix++;
    }

    return (wstat);
}
/***************************************************************/
int parse_wsdl_sch_documentation(struct wsglob * wsg,
                    struct xmltree * xtree_wstd,
                    const char * name_space,
                    struct wsdocumentation * wstd) {
/*
<documentation
  source = anyURI
  xml:lang = language
  {any attributes with non-schema namespace . . .}>
  Content: ({any})*
</documentation>
*/

    int wstat = 0;
    int atix;

    atix = 0;
    while (!wstat && atix < xtree_wstd->nattrs) {
        if (!strcmp(xtree_wstd->attrnam[atix], WSSH_TYP_SOURCE)) {
            wstd->wstd_source = Strdup(xtree_wstd->attrval[atix]);

        } else if (!strcmp(xtree_wstd->attrnam[atix], WSSH_TYP_XML_LANG)) {
            wstd->wstd_xml_lang = Strdup(xtree_wstd->attrval[atix]);

        } else {
            wstat = ws_set_error(wsg, WSE_BADDOCUMENTATIONATTR,
                    "%s", xtree_wstd->attrnam[atix]);
        }
        atix++;
    }

    if (!wstat && xtree_wstd->eldata) {
        wstd->wstd_cdata = Strdup(xtree_wstd->eldata);
    }

    if (!wstat && xtree_wstd->nsubel) {
        wstat = parse_documentation_content(wsg, xtree_wstd, name_space, wstd);
    }

    return (wstat);
}
/***************************************************************/
int parse_appinfo_content(struct wsglob * wsg,
                    struct xmltree * xtree_wsti,
                    const char * name_space,
                    struct wsappinfo * wsti) {
/*
  Content: (appinfo | documentation)*
*/

    int wstat = 0;
    int trix;

    trix = 0;
    while (!wstat && trix < xtree_wsti->nsubel) {
        if (!xml_name_space_equals(xtree_wsti->subel[trix]->elname,
            name_space)) {

            /* ignore content from different name space */
        } else {
            /* ebay wsdl conains "<xs:CallName>GetItem</xs:CallName>" */
            /* I think this is a bug for them. */

            /* wstat = ws_set_error(wsg, WSE_APPINFOCONTENT,    */
            /*          "%s", xtree_wsti->subel[trix]->elname); */
        }
        trix++;
    }

    return (wstat);
}
/***************************************************************/
int parse_wsdl_sch_appinfo(struct wsglob * wsg,
                    struct xmltree * xtree_wsti,
                    const char * name_space,
                    struct wsappinfo * wsti) {
/*
<appinfo
  source = anyURI
  {any attributes with non-schema namespace . . .}>
  Content: ({any})*
</appinfo>
*/

    int wstat = 0;
    int atix;

    atix = 0;
    while (!wstat && atix < xtree_wsti->nattrs) {
        if (!strcmp(xtree_wsti->attrnam[atix], WSSH_TYP_SOURCE)) {
            wsti->wsti_source = Strdup(xtree_wsti->attrval[atix]);

        } else {
            wstat = ws_set_error(wsg, WSE_BADAPPINFOATTR,
                    "%s", xtree_wsti->attrnam[atix]);
        }
        atix++;
    }

    if (!wstat && xtree_wsti->eldata) {
        wsti->wsti_cdata = Strdup(xtree_wsti->eldata);
    }

    if (!wstat && xtree_wsti->nsubel) {
        wstat = parse_appinfo_content(wsg, xtree_wsti, name_space, wsti);
    }

    return (wstat);
}
/***************************************************************/
int parse_annotation_content(struct wsglob * wsg,
                    struct xmltree * xtree_wsta,
                    const char * name_space,
                    struct wsannotation * wsta) {
/*
  Content: (appinfo | documentation)*
*/

    int wstat = 0;
    int trix;
    struct wscontent * wstc;

    trix = 0;
    while (!wstat && trix < xtree_wsta->nsubel) {
        if (xml_full_name_equals(xtree_wsta->subel[trix]->elname,
            name_space, WSSH_APPINFO)) {
            wstc = add_new_annotation_content(wsta, WSST_APPINFO);
            wstc->wstc_itm.wstcu_wsappinfo = new_wsappinfo();
            wstat = parse_wsdl_sch_appinfo(wsg, xtree_wsta->subel[trix],
                name_space, wstc->wstc_itm.wstcu_wsappinfo);

        } else if (xml_full_name_equals(xtree_wsta->subel[trix]->elname,
            name_space, WSSH_DOCUMENTATION)) {
            wstc = add_new_annotation_content(wsta, WSST_DOCUMENTATION);
            wstc->wstc_itm.wstcu_wsdocumentation = new_wsdocumentation();
            wstat = parse_wsdl_sch_documentation(wsg, xtree_wsta->subel[trix],
                name_space, wstc->wstc_itm.wstcu_wsdocumentation);

        } else if (xml_name_space_equals(xtree_wsta->subel[trix]->elname,
                    name_space)) {
            wstat = ws_set_error(wsg, WSE_ANNOTATIONCONTENT,
                        "%s", xtree_wsta->subel[trix]->elname);
        }
        trix++;
    }

    return (wstat);
}
/***************************************************************/
int parse_wsdl_sch_annotation(struct wsglob * wsg,
                    struct xmltree * xtree_wsta,
                    const char * name_space,
                    struct wsannotation * wsta) {
/*
<annotation
  id = ID
  {any attributes with non-schema namespace . . .}>
  Content: (appinfo | documentation)*
</annotation>
*/

    int wstat = 0;
    int atix;

    atix = 0;
    while (!wstat && atix < xtree_wsta->nattrs) {
        if (!strcmp(xtree_wsta->attrnam[atix], WSSH_TYP_ID)) {
            wsta->wsta_id = Strdup(xtree_wsta->attrval[atix]);

        } else {
            wstat = ws_set_error(wsg, WSE_BADANNOTATIONATTR,
                    "%s", xtree_wsta->attrnam[atix]);
        }
        atix++;
    }

    if (!wstat && xtree_wsta->nsubel) {
        wstat = parse_annotation_content(wsg, xtree_wsta, name_space, wsta);
    }

    return (wstat);
}
/***************************************************************/
int parse_union_content(struct wsglob * wsg,
                    struct xmltree * xtree_wstv,
                    const char * name_space,
                    struct wsunion * wstv) {
/*
  Content: (annotation?, simpleType*)
*/

    int wstat = 0;
    int trix;
    struct wscontent * wstc;

    trix = 0;
    while (!wstat && trix < xtree_wstv->nsubel) {
        if (xml_full_name_equals(xtree_wstv->subel[trix]->elname,
            name_space, WSSH_ANNOTATION)) {
            wstc = add_new_union_content(wstv, WSST_ANNOTATION);
            wstc->wstc_itm.wstcu_wsannotation = new_wsannotation();
            wstat = parse_wsdl_sch_annotation(wsg, xtree_wstv->subel[trix],
                name_space, wstc->wstc_itm.wstcu_wsannotation);

        } else if (xml_full_name_equals(xtree_wstv->subel[trix]->elname,
            name_space, WSSH_SIMPLE_TYPE)) {
            wstc = add_new_union_content(wstv, WSST_SIMPLE_TYPE);
            wstc->wstc_itm.wstcu_wssimpletype = new_wssimpletype();
            wstat = parse_wsdl_sch_simple_type(wsg, xtree_wstv->subel[trix],
                name_space, wstc->wstc_itm.wstcu_wssimpletype);

        } else if (xml_name_space_equals(xtree_wstv->subel[trix]->elname,
                    name_space)) {
            wstat = ws_set_error(wsg, WSE_UNIONCONTENT,
                        "%s", xtree_wstv->subel[trix]->elname);
        }
        trix++;
    }

    return (wstat);
}
/***************************************************************/
int parse_wsdl_sch_union(struct wsglob * wsg,
                    struct xmltree * xtree_wstv,
                    const char * name_space,
                    struct wsunion * wstv) {
/*
<union
  id = ID
  memberTypes = List of QName
  {any attributes with non-schema namespace . . .}>
  Content: (annotation?, simpleType*)
</union>
*/

    int wstat = 0;
    int atix;

    atix = 0;
    while (!wstat && atix < xtree_wstv->nattrs) {
        if (!strcmp(xtree_wstv->attrnam[atix], WSSH_TYP_ID)) {
            wstv->wstv_id = Strdup(xtree_wstv->attrval[atix]);

        } else if (!strcmp(xtree_wstv->attrnam[atix], WSSH_TYP_MEMBER_TYPES)) {
            wstv->wstv_memberTypes = Strdup(xtree_wstv->attrval[atix]);

        } else {
            wstat = ws_set_error(wsg, WSE_BADUNIONATTR,
                    "%s", xtree_wstv->attrnam[atix]);
        }
        atix++;
    }

    if (!wstat && xtree_wstv->nsubel) {
        wstat = parse_union_content(wsg, xtree_wstv, name_space, wstv);
    }

    return (wstat);
}
/***************************************************************/
int parse_all_content(struct wsglob * wsg,
                    struct xmltree * xtree_wstwa,
                    const char * name_space,
                    struct wsall * wstwa) {
/*
  Content: (annotation?, (element | any | group)*)
*/

    int wstat = 0;
    int trix;
    struct wscontent * wstc;

    trix = 0;
    while (!wstat && trix < xtree_wstwa->nsubel) {
        if (xml_full_name_equals(xtree_wstwa->subel[trix]->elname,
            name_space, WSSH_ANNOTATION)) {
            wstc = add_new_all_content(wstwa, WSST_ANNOTATION);
            wstc->wstc_itm.wstcu_wsannotation = new_wsannotation();
            wstat = parse_wsdl_sch_annotation(wsg, xtree_wstwa->subel[trix],
                name_space, wstc->wstc_itm.wstcu_wsannotation);

        } else if (xml_full_name_equals(xtree_wstwa->subel[trix]->elname,
            name_space, WSSH_ELEMENT)) {
            wstc = add_new_all_content(wstwa, WSST_ELEMENT);
            wstc->wstc_itm.wstcu_wselement = new_wselement();
            wstat = parse_wsdl_sch_element(wsg, xtree_wstwa->subel[trix],
                name_space, wstc->wstc_itm.wstcu_wselement);

        } else if (xml_full_name_equals(xtree_wstwa->subel[trix]->elname,
            name_space, WSSH_GROUP)) {
            wstc = add_new_all_content(wstwa, WSST_GROUP);
            wstc->wstc_itm.wstcu_wsgroup = new_wsgroup();
            wstat = parse_wsdl_sch_group(wsg, xtree_wstwa->subel[trix],
                name_space, wstc->wstc_itm.wstcu_wsgroup);

        } else if (xml_full_name_equals(xtree_wstwa->subel[trix]->elname,
            name_space, WSSH_ANY)) {
            wstc = add_new_all_content(wstwa, WSST_ANY);
            wstc->wstc_itm.wstcu_wsany = new_wsany();
            wstat = parse_wsdl_sch_any(wsg, xtree_wstwa->subel[trix],
                name_space, wstc->wstc_itm.wstcu_wsany);

        } else if (xml_name_space_equals(xtree_wstwa->subel[trix]->elname,
                    name_space)) {
            wstat = ws_set_error(wsg, WSE_ALLCONTENT,
                        "%s", xtree_wstwa->subel[trix]->elname);
        }
        trix++;
    }

    return (wstat);
}
/***************************************************************/
int parse_wsdl_sch_all(struct wsglob * wsg,
                    struct xmltree * xtree_wstwa,
                    const char * name_space,
                    struct wsall * wstwa) {
/*
<all
  id = ID
  maxOccurs = (0 | 1) : 1
  minOccurs = (0 | 1) : 1
  {any attributes with non-schema namespace . . .}>
  Content: (annotation?, (element | any | group)*)
</all>
*/

    int wstat = 0;
    int atix;

    atix = 0;
    while (!wstat && atix < xtree_wstwa->nattrs) {
        if (!strcmp(xtree_wstwa->attrnam[atix], WSSH_TYP_ID)) {
            wstwa->wstwa_id = Strdup(xtree_wstwa->attrval[atix]);

        } else if (!strcmp(xtree_wstwa->attrnam[atix], WSSH_TYP_MAXOCCURS)) {
            wstwa->wstwa_maxOccurs = Strdup(xtree_wstwa->attrval[atix]);

        } else if (!strcmp(xtree_wstwa->attrnam[atix], WSSH_TYP_MINOCCURS)) {
            wstwa->wstwa_minOccurs = Strdup(xtree_wstwa->attrval[atix]);

        } else {
            wstat = ws_set_error(wsg, WSE_BADALLATTR,
                    "%s", xtree_wstwa->attrnam[atix]);
        }
        atix++;
    }

    if (!wstat && xtree_wstwa->nsubel) {
        wstat = parse_all_content(wsg, xtree_wstwa, name_space, wstwa);
    }

    return (wstat);
}
/***************************************************************/
int parse_choice_content(struct wsglob * wsg,
                    struct xmltree * xtree_wstwc,
                    const char * name_space,
                    struct wschoice * wstwc) {
/*
  Content: (annotation?, (element | group | choice | sequence | any)*)
*/

    int wstat = 0;
    int trix;
    struct wscontent * wstc;

    trix = 0;
    while (!wstat && trix < xtree_wstwc->nsubel) {
        if (xml_full_name_equals(xtree_wstwc->subel[trix]->elname,
            name_space, WSSH_ANNOTATION)) {
            wstc = add_new_choice_content(wstwc, WSST_ANNOTATION);
            wstc->wstc_itm.wstcu_wsannotation = new_wsannotation();
            wstat = parse_wsdl_sch_annotation(wsg, xtree_wstwc->subel[trix],
                name_space, wstc->wstc_itm.wstcu_wsannotation);

        } else if (xml_full_name_equals(xtree_wstwc->subel[trix]->elname,
            name_space, WSSH_ELEMENT)) {
            wstc = add_new_choice_content(wstwc, WSST_ELEMENT);
            wstc->wstc_itm.wstcu_wselement = new_wselement();
            wstat = parse_wsdl_sch_element(wsg, xtree_wstwc->subel[trix],
                name_space, wstc->wstc_itm.wstcu_wselement);

        } else if (xml_full_name_equals(xtree_wstwc->subel[trix]->elname,
            name_space, WSSH_GROUP)) {
            wstc = add_new_choice_content(wstwc, WSST_GROUP);
            wstc->wstc_itm.wstcu_wsgroup = new_wsgroup();
            wstat = parse_wsdl_sch_group(wsg, xtree_wstwc->subel[trix],
                name_space, wstc->wstc_itm.wstcu_wsgroup);

        } else if (xml_full_name_equals(xtree_wstwc->subel[trix]->elname,
            name_space, WSSH_SEQUENCE)) {
            wstc = add_new_choice_content(wstwc, WSST_SEQUENCE);
            wstc->wstc_itm.wstcu_wssequence = new_wssequence();
            wstat = parse_wsdl_sch_sequence(wsg, xtree_wstwc->subel[trix],
                name_space, wstc->wstc_itm.wstcu_wssequence);

        } else if (xml_full_name_equals(xtree_wstwc->subel[trix]->elname,
            name_space, WSSH_ANY)) {
            wstc = add_new_choice_content(wstwc, WSST_ANY);
            wstc->wstc_itm.wstcu_wsany = new_wsany();
            wstat = parse_wsdl_sch_any(wsg, xtree_wstwc->subel[trix],
                name_space, wstc->wstc_itm.wstcu_wsany);

        } else if (xml_name_space_equals(xtree_wstwc->subel[trix]->elname,
                    name_space)) {
            wstat = ws_set_error(wsg, WSE_CHOICECONTENT,
                        "%s", xtree_wstwc->subel[trix]->elname);
        }
        trix++;
    }

    return (wstat);
}
/***************************************************************/
int parse_wsdl_sch_choice(struct wsglob * wsg,
                    struct xmltree * xtree_wstwc,
                    const char * name_space,
                    struct wschoice * wstwc) {
/*
<choice
  id = ID
  maxOccurs = (nonNegativeInteger | unbounded)  : 1
  minOccurs = nonNegativeInteger : 1
  {any attributes with non-schema namespace . . .}>
  Content: (annotation?, (element | group | choice | sequence | any)*)
</choice>
*/

    int wstat = 0;
    int atix;

    atix = 0;
    while (!wstat && atix < xtree_wstwc->nattrs) {
        if (!strcmp(xtree_wstwc->attrnam[atix], WSSH_TYP_ID)) {
            wstwc->wstwc_id = Strdup(xtree_wstwc->attrval[atix]);

        } else if (!strcmp(xtree_wstwc->attrnam[atix], WSSH_TYP_MAXOCCURS)) {
            wstwc->wstwc_maxOccurs = Strdup(xtree_wstwc->attrval[atix]);

        } else if (!strcmp(xtree_wstwc->attrnam[atix], WSSH_TYP_MINOCCURS)) {
            wstwc->wstwc_minOccurs = Strdup(xtree_wstwc->attrval[atix]);

        } else {
            wstat = ws_set_error(wsg, WSE_BADCHOICEATTR,
                    "%s", xtree_wstwc->attrnam[atix]);
        }
        atix++;
    }

    if (!wstat && xtree_wstwc->nsubel) {
        wstat = parse_choice_content(wsg, xtree_wstwc, name_space, wstwc);
    }

    return (wstat);
}



/***************************************************************/
int parse_group_content(struct wsglob * wsg,
                    struct xmltree * xtree_wstg,
                    const char * name_space,
                    struct wsgroup * wstg) {
/*
  Content: (annotation?, (all | choice | sequence)?)
*/

    int wstat = 0;
    int trix;
    struct wscontent * wstc;

    trix = 0;
    while (!wstat && trix < xtree_wstg->nsubel) {
        if (xml_full_name_equals(xtree_wstg->subel[trix]->elname,
            name_space, WSSH_ANNOTATION)) {
            wstc = add_new_group_content(wstg, WSST_ANNOTATION);
            wstc->wstc_itm.wstcu_wsannotation = new_wsannotation();
            wstat = parse_wsdl_sch_annotation(wsg, xtree_wstg->subel[trix],
                name_space, wstc->wstc_itm.wstcu_wsannotation);

        } else if (xml_full_name_equals(xtree_wstg->subel[trix]->elname,
            name_space, WSSH_ALL)) {
            wstc = add_new_group_content(wstg, WSST_ALL);
            wstc->wstc_itm.wstcu_wsall = new_wsall();
            wstat = parse_wsdl_sch_all(wsg, xtree_wstg->subel[trix],
                name_space, wstc->wstc_itm.wstcu_wsall);

        } else if (xml_full_name_equals(xtree_wstg->subel[trix]->elname,
            name_space, WSSH_CHOICE)) {
            wstc = add_new_group_content(wstg, WSST_CHOICE);
            wstc->wstc_itm.wstcu_wschoice = new_wschoice();
            wstat = parse_wsdl_sch_choice(wsg, xtree_wstg->subel[trix],
                name_space, wstc->wstc_itm.wstcu_wschoice);

        } else if (xml_full_name_equals(xtree_wstg->subel[trix]->elname,
            name_space, WSSH_SEQUENCE)) {
            wstc = add_new_group_content(wstg, WSST_SEQUENCE);
            wstc->wstc_itm.wstcu_wssequence = new_wssequence();
            wstat = parse_wsdl_sch_sequence(wsg, xtree_wstg->subel[trix],
                name_space, wstc->wstc_itm.wstcu_wssequence);

        } else if (xml_name_space_equals(xtree_wstg->subel[trix]->elname,
                    name_space)) {
            wstat = ws_set_error(wsg, WSE_GROUPCONTENT,
                        "%s", xtree_wstg->subel[trix]->elname);
        }
        trix++;
    }

    return (wstat);
}
/***************************************************************/
int parse_wsdl_sch_group(struct wsglob * wsg,
                    struct xmltree * xtree_wstg,
                    const char * name_space,
                    struct wsgroup * wstg) {
/*
<group
  id = ID
  maxOccurs = (nonNegativeInteger | unbounded)  : 1
  minOccurs = nonNegativeInteger : 1
  name = NCName
  ref = QName
  {any attributes with non-schema namespace . . .}>
  Content: (annotation?, (all | choice | sequence)?)
</group>
*/

    int wstat = 0;
    int atix;

    atix = 0;
    while (!wstat && atix < xtree_wstg->nattrs) {
        if (!strcmp(xtree_wstg->attrnam[atix], WSSH_TYP_ID)) {
            wstg->wstg_id = Strdup(xtree_wstg->attrval[atix]);

        } else if (!strcmp(xtree_wstg->attrnam[atix], WSSH_TYP_MAXOCCURS)) {
            wstg->wstg_maxOccurs = Strdup(xtree_wstg->attrval[atix]);

        } else if (!strcmp(xtree_wstg->attrnam[atix], WSSH_TYP_MINOCCURS)) {
            wstg->wstg_minOccurs = Strdup(xtree_wstg->attrval[atix]);

        } else if (!strcmp(xtree_wstg->attrnam[atix], WSSH_TYP_NAME)) {
            wstg->wstg_name = Strdup(xtree_wstg->attrval[atix]);

        } else if (!strcmp(xtree_wstg->attrnam[atix], WSSH_TYP_REF)) {
            wstg->wstg_ref = Strdup(xtree_wstg->attrval[atix]);

        } else {
            wstat = ws_set_error(wsg, WSE_BADGROUPATTR,
                    "%s", xtree_wstg->attrnam[atix]);
        }
        atix++;
    }

    if (!wstat && xtree_wstg->nsubel) {
        wstat = parse_group_content(wsg, xtree_wstg, name_space, wstg);
    }

    return (wstat);
}

















/***************************************************************/
int parse_simple_type_content(struct wsglob * wsg,
                    struct xmltree * xtree_wstt,
                    const char * name_space,
                    struct wssimpletype * wstt) {
/*
  Content: (annotation?, (restriction | list | union))
*/

    int wstat = 0;
    int trix;
    struct wscontent * wstc;

    trix = 0;
    while (!wstat && trix < xtree_wstt->nsubel) {
        if (xml_full_name_equals(xtree_wstt->subel[trix]->elname,
            name_space, WSSH_RESTRICTION)) {
            wstc = add_new_simple_type_content(wstt, WSST_RESTRICTION);
            wstc->wstc_itm.wstcu_wsrestriction = new_wsrestriction();
            wstat = parse_wsdl_sch_restriction(wsg, xtree_wstt->subel[trix],
                name_space, wstc->wstc_itm.wstcu_wsrestriction);

        } else if (xml_full_name_equals(xtree_wstt->subel[trix]->elname,
            name_space, WSSH_LIST)) {
            wstc = add_new_simple_type_content(wstt, WSST_LIST);
            wstc->wstc_itm.wstcu_wslist = new_wslist();
            wstat = parse_wsdl_sch_list(wsg, xtree_wstt->subel[trix],
                name_space, wstc->wstc_itm.wstcu_wslist);

        } else if (xml_full_name_equals(xtree_wstt->subel[trix]->elname,
            name_space, WSSH_UNION)) {
            wstc = add_new_simple_type_content(wstt, WSST_UNION);
            wstc->wstc_itm.wstcu_wsunion = new_wsunion();
            wstat = parse_wsdl_sch_union(wsg, xtree_wstt->subel[trix],
                name_space, wstc->wstc_itm.wstcu_wsunion);

        } else if (xml_full_name_equals(xtree_wstt->subel[trix]->elname,
            name_space, WSSH_ANNOTATION)) {
            wstc = add_new_simple_type_content(wstt, WSST_ANNOTATION);
            wstc->wstc_itm.wstcu_wsannotation = new_wsannotation();
            wstat = parse_wsdl_sch_annotation(wsg, xtree_wstt->subel[trix],
                name_space, wstc->wstc_itm.wstcu_wsannotation);

        } else if (xml_name_space_equals(xtree_wstt->subel[trix]->elname,
                    name_space)) {
            wstat = ws_set_error(wsg, WSE_SIMPLETYPECONTENT,
                        "%s", xtree_wstt->subel[trix]->elname);
        }
        trix++;
    }

    return (wstat);
}
/***************************************************************/
int parse_wsdl_sch_simple_type(struct wsglob * wsg,
                    struct xmltree * xtree_wstt,
                    const char * name_space,
                    struct wssimpletype * wstt) {
/*
3.16.2 XML Representation of Simple Type Definition Schema Components

<simpleType
  final = (#all | List of (list | union | restriction | extension)) 
  id = ID
  name = NCName
  {any attributes with non-schema namespace . . .}>
  Content: (annotation?, (restriction | list | union))
</simpleType>
*/

    int wstat = 0;
    int atix;

    atix = 0;
    while (!wstat && atix < xtree_wstt->nattrs) {
        if (!strcmp(xtree_wstt->attrnam[atix], WSSH_TYP_FINAL)) {
            wstt->wstt_final = Strdup(xtree_wstt->attrval[atix]);

        } else if (!strcmp(xtree_wstt->attrnam[atix], WSSH_TYP_ID)) {
            wstt->wstt_id = Strdup(xtree_wstt->attrval[atix]);

        } else if (!strcmp(xtree_wstt->attrnam[atix], WSSH_TYP_NAME)) {
            wstt->wstt_name = Strdup(xtree_wstt->attrval[atix]);

        } else {
            wstat = ws_set_error(wsg, WSE_BADSIMPLETYPEATTR,
                    "%s", xtree_wstt->attrnam[atix]);
        }
        atix++;
    }

    if (!wstat && xtree_wstt->nsubel) {
        wstat = parse_simple_type_content(wsg, xtree_wstt, name_space, wstt);
    }

    return (wstat);
}
/***************************************************************/
int parse_sequence_content(struct wsglob * wsg,
                    struct xmltree * xtree_wstq,
                    const char * name_space,
                    struct wssequence * wstq) {
/*
  Content: (annotation?, (element | group | choice | sequence | any)*)
*/
    int wstat = 0;
    int trix;
    struct wscontent * wstc;

    trix = 0;
    while (!wstat && trix < xtree_wstq->nsubel) {
        if (xml_full_name_equals(xtree_wstq->subel[trix]->elname,
            name_space, WSSH_ELEMENT)) {
            wstc = add_new_sequence_content(wstq, WSST_ELEMENT);
            wstc->wstc_itm.wstcu_wselement = new_wselement();
            wstat = parse_wsdl_sch_element(wsg, xtree_wstq->subel[trix],
                name_space, wstc->wstc_itm.wstcu_wselement);

        } else if (xml_full_name_equals(xtree_wstq->subel[trix]->elname,
            name_space, WSSH_SEQUENCE)) {
            wstc = add_new_sequence_content(wstq, WSST_SEQUENCE);
            wstc->wstc_itm.wstcu_wssequence = new_wssequence();
            wstat = parse_wsdl_sch_sequence(wsg, xtree_wstq->subel[trix],
                name_space, wstc->wstc_itm.wstcu_wssequence);

        } else if (xml_full_name_equals(xtree_wstq->subel[trix]->elname,
            name_space, WSSH_ANY)) {
            wstc = add_new_sequence_content(wstq, WSST_ANY);
            wstc->wstc_itm.wstcu_wsany = new_wsany();
            wstat = parse_wsdl_sch_any(wsg, xtree_wstq->subel[trix],
                name_space, wstc->wstc_itm.wstcu_wsany);

        } else if (xml_full_name_equals(xtree_wstq->subel[trix]->elname,
            name_space, WSSH_ANNOTATION)) {
            wstc = add_new_sequence_content(wstq, WSST_ANNOTATION);
            wstc->wstc_itm.wstcu_wsannotation = new_wsannotation();
            wstat = parse_wsdl_sch_annotation(wsg, xtree_wstq->subel[trix],
                name_space, wstc->wstc_itm.wstcu_wsannotation);

        } else if (xml_full_name_equals(xtree_wstq->subel[trix]->elname,
            name_space, WSSH_CHOICE)) {
            wstc = add_new_sequence_content(wstq, WSST_CHOICE);
            wstc->wstc_itm.wstcu_wschoice = new_wschoice();
            wstat = parse_wsdl_sch_choice(wsg, xtree_wstq->subel[trix],
                name_space, wstc->wstc_itm.wstcu_wschoice);

        } else if (xml_full_name_equals(xtree_wstq->subel[trix]->elname,
            name_space, WSSH_GROUP)) {
            wstc = add_new_sequence_content(wstq, WSST_GROUP);
            wstc->wstc_itm.wstcu_wsgroup = new_wsgroup();
            wstat = parse_wsdl_sch_group(wsg, xtree_wstq->subel[trix],
                name_space, wstc->wstc_itm.wstcu_wsgroup);

        } else if (xml_name_space_equals(xtree_wstq->subel[trix]->elname,
                    name_space)) {
            wstat = ws_set_error(wsg, WSE_SEQUENCECONTENT,
                        "%s", xtree_wstq->subel[trix]->elname);
        }
        trix++;
    }

    return (wstat);
}
/***************************************************************/
int parse_wsdl_sch_sequence(struct wsglob * wsg,
                    struct xmltree * xtree_wstq,
                    const char * name_space,
                    struct wssequence * wstq)
{
/*
<sequence
  id = ID
  maxOccurs = (nonNegativeInteger | unbounded)  : 1
  minOccurs = nonNegativeInteger : 1
  {any attributes with non-schema namespace . . .}>
  Content: (annotation?, (element | group | choice | sequence | any)*)
</sequence>
*/
    int wstat = 0;
    int atix;

    atix = 0;
    while (!wstat && atix < xtree_wstq->nattrs) {
        if (!strcmp(xtree_wstq->attrnam[atix], WSSH_TYP_ID)) {
            wstq->wstq_id = Strdup(xtree_wstq->attrval[atix]);

        } else if (!strcmp(xtree_wstq->attrnam[atix], WSSH_TYP_MAXOCCURS)) {
            wstq->wstq_maxOccurs = Strdup(xtree_wstq->attrval[atix]);

        } else if (!strcmp(xtree_wstq->attrnam[atix], WSSH_TYP_MINOCCURS)) {
            wstq->wstq_minOccurs = Strdup(xtree_wstq->attrval[atix]);

        } else {
            wstat = ws_set_error(wsg, WSE_BADSEQUENCEATTR,
                    "%s", xtree_wstq->attrnam[atix]);
        }
        atix++;
    }

    if (!wstat && xtree_wstq->nsubel) {
        wstat = parse_sequence_content(wsg, xtree_wstq, name_space, wstq);
    }

    return (wstat);
}
/***************************************************************/
int parse_attribute_content(struct wsglob * wsg,
                    struct xmltree * xtree_wstb,
                    const char * name_space,
                    struct wsattribute * wstb) {
/*
  Content: (annotation?, openContent?, ((group | all | choice | sequence)?,
            ((attribute | attributeGroup)*, anyAttribute?), assert*))
*/
    int wstat = 0;
    int trix;
    struct wscontent * wstc;

    trix = 0;
    while (!wstat && trix < xtree_wstb->nsubel) {
        if (xml_full_name_equals(xtree_wstb->subel[trix]->elname,
            name_space, WSSH_ANY_ATTRIBUTE)) {
            wstc = add_new_attribute_content(wstb, WSST_ANY_ATTRIBUTE);
            wstc->wstc_itm.wstcu_wsanyattribute = new_wsanyattribute();
            wstat = parse_wsdl_sch_any_attribute(wsg, xtree_wstb->subel[trix],
                name_space, wstc->wstc_itm.wstcu_wsanyattribute);

        } else if (xml_full_name_equals(xtree_wstb->subel[trix]->elname,
            name_space, WSSH_ANNOTATION)) {
            wstc = add_new_attribute_content(wstb, WSST_ANNOTATION);
            wstc->wstc_itm.wstcu_wsannotation = new_wsannotation();
            wstat = parse_wsdl_sch_annotation(wsg, xtree_wstb->subel[trix],
                name_space, wstc->wstc_itm.wstcu_wsannotation);

        } else if (xml_name_space_equals(xtree_wstb->subel[trix]->elname,
                    name_space)) {
            wstat = ws_set_error(wsg, WSE_ATTRIBUTECONTENT,
                        "%s", xtree_wstb->subel[trix]->elname);
        }
        trix++;
    }

    return (wstat);
}
/***************************************************************/
int parse_wsdl_sch_attribute(struct wsglob * wsg,
                    struct xmltree * xtree_wstb,
                    const char * name_space,
                    struct wsattribute * wstb)
{
/*
<attribute
  default = string
  fixed = string
  form = (qualified | unqualified)
  id = ID
  name = NCName
  ref = QName
  targetNamespace = anyURI
  type = QName
  use = (optional | prohibited | required) : optional
  inheritable = boolean
  {any attributes with non-schema namespace . . .}>
  Content: (annotation?, simpleType?)
</attribute>
*/
    int wstat = 0;
    int atix;

    atix = 0;
    while (!wstat && atix < xtree_wstb->nattrs) {
        if (!strcmp(xtree_wstb->attrnam[atix], WSSH_TYP_DEFAULT)) {
            wstb->wstb_default = Strdup(xtree_wstb->attrval[atix]);

        } else if (!strcmp(xtree_wstb->attrnam[atix], WSSH_TYP_FIXED)) {
            wstb->wstb_fixed = Strdup(xtree_wstb->attrval[atix]);

        } else if (!strcmp(xtree_wstb->attrnam[atix], WSSH_TYP_FORM)) {
            wstb->wstb_form = Strdup(xtree_wstb->attrval[atix]);

        } else if (!strcmp(xtree_wstb->attrnam[atix], WSSH_TYP_ID)) {
            wstb->wstb_id = Strdup(xtree_wstb->attrval[atix]);

        } else if (!strcmp(xtree_wstb->attrnam[atix], WSSH_TYP_NAME)) {
            wstb->wstb_name = Strdup(xtree_wstb->attrval[atix]);

        } else if (!strcmp(xtree_wstb->attrnam[atix], WSSH_TYP_REF)) {
            wstb->wstb_ref = Strdup(xtree_wstb->attrval[atix]);

        } else if (!strcmp(xtree_wstb->attrnam[atix],
                            WSSH_TYP_TARGETNAMESPACE)) {
            wstb->wstb_targetNamespace = Strdup(xtree_wstb->attrval[atix]);

        } else if (!strcmp(xtree_wstb->attrnam[atix], WSSH_TYP_TYPE)) {
            wstb->wstb_type = Strdup(xtree_wstb->attrval[atix]);

        } else if (!strcmp(xtree_wstb->attrnam[atix], WSSH_TYP_USE)) {
            wstb->wstb_use = Strdup(xtree_wstb->attrval[atix]);

        } else if (!strcmp(xtree_wstb->attrnam[atix], WSSH_TYP_INHERITABLE)) {
            wstb->wstb_inheritable = Strdup(xtree_wstb->attrval[atix]);

        } else {
            wstat = ws_set_error(wsg, WSE_BADATTRIBUTEATTR,
                    "%s", xtree_wstb->attrnam[atix]);
        }
        atix++;
    }

    if (!wstat && xtree_wstb->nsubel) {
        wstat = parse_attribute_content(wsg, xtree_wstb, name_space, wstb);
    }

    return (wstat);
}


/***************************************************************/
int parse_simple_content_content(struct wsglob * wsg,
                    struct xmltree * xtree_wstp,
                    const char * name_space,
                    struct wssimplecontent * wstp) {
/*
  Content: (annotation?, (restriction | extension))
*/
    int wstat = 0;
    int trix;
    struct wscontent * wstc;

    trix = 0;
    while (!wstat && trix < xtree_wstp->nsubel) {
        if (xml_full_name_equals(xtree_wstp->subel[trix]->elname,
            name_space, WSSH_EXTENSION)) {
            wstc = add_new_simple_content_content(wstp, WSST_EXTENSION);
            wstc->wstc_itm.wstcu_wsextension = new_wsextension();
            wstat = parse_wsdl_sch_extension(wsg, xtree_wstp->subel[trix],
                name_space, wstc->wstc_itm.wstcu_wsextension);

        } else if (xml_name_space_equals(xtree_wstp->subel[trix]->elname,
                    name_space)) {
            wstat = ws_set_error(wsg, WSE_SIMPLECONTENTCONTENT,
                        "%s", xtree_wstp->subel[trix]->elname);
        }
        trix++;
    }

    return (wstat);
}
/***************************************************************/
int parse_wsdl_sch_simple_content(struct wsglob * wsg,
                    struct xmltree * xtree_wstp,
                    const char * name_space,
                    struct wssimplecontent * wstp)
{
/*
<simpleContent
  id = ID
  {any attributes with non-schema namespace . . .}>
  Content: (annotation?, (restriction | extension))
</simpleContent>
*/
    int wstat = 0;
    int atix;

    atix = 0;
    while (!wstat && atix < xtree_wstp->nattrs) {
        if (!strcmp(xtree_wstp->attrnam[atix], WSSH_TYP_ID)) {
            wstp->wstp_id = Strdup(xtree_wstp->attrval[atix]);

        } else {
            wstat = ws_set_error(wsg, WSE_BADSIMPLECONTENTATTR,
                    "%s", xtree_wstp->attrnam[atix]);
        }
        atix++;
    }

    if (!wstat && xtree_wstp->nsubel) {
        wstat = parse_simple_content_content(wsg, xtree_wstp, name_space, wstp);
    }

    return (wstat);
}
/***************************************************************/
int parse_extension_content(struct wsglob * wsg,
                    struct xmltree * xtree_wstn,
                    const char * name_space,
                    struct wsextension * wstn) {
/*
  Content: (annotation?, openContent?, ((group | all | choice | sequence)?,
            ((attribute | attributeGroup)*, anyAttribute?), assert*))
*/
    int wstat = 0;
    int trix;
    struct wscontent * wstc;

    trix = 0;
    while (!wstat && trix < xtree_wstn->nsubel) {
        if (xml_full_name_equals(xtree_wstn->subel[trix]->elname,
            name_space, WSSH_SEQUENCE)) {
            wstc = add_new_extension_content(wstn, WSST_SEQUENCE);
            wstc->wstc_itm.wstcu_wssequence = new_wssequence();
            wstat = parse_wsdl_sch_sequence(wsg, xtree_wstn->subel[trix],
                name_space, wstc->wstc_itm.wstcu_wssequence);

        } else if (xml_full_name_equals(xtree_wstn->subel[trix]->elname,
            name_space, WSSH_ATTRIBUTE)) {
            wstc = add_new_extension_content(wstn, WSST_ATTRIBUTE);
            wstc->wstc_itm.wstcu_wsattribute = new_wsattribute();
            wstat = parse_wsdl_sch_attribute(wsg, xtree_wstn->subel[trix],
                name_space, wstc->wstc_itm.wstcu_wsattribute);

        } else if (xml_full_name_equals(xtree_wstn->subel[trix]->elname,
            name_space, WSSH_ALL)) {
            wstc = add_new_extension_content(wstn, WSST_ALL);
            wstc->wstc_itm.wstcu_wsall = new_wsall();
            wstat = parse_wsdl_sch_all(wsg, xtree_wstn->subel[trix],
                name_space, wstc->wstc_itm.wstcu_wsall);

        } else if (xml_full_name_equals(xtree_wstn->subel[trix]->elname,
            name_space, WSSH_CHOICE)) {
            wstc = add_new_extension_content(wstn, WSST_CHOICE);
            wstc->wstc_itm.wstcu_wschoice = new_wschoice();
            wstat = parse_wsdl_sch_choice(wsg, xtree_wstn->subel[trix],
                name_space, wstc->wstc_itm.wstcu_wschoice);

        } else if (xml_full_name_equals(xtree_wstn->subel[trix]->elname,
            name_space, WSSH_GROUP)) {
            wstc = add_new_extension_content(wstn, WSST_GROUP);
            wstc->wstc_itm.wstcu_wsgroup = new_wsgroup();
            wstat = parse_wsdl_sch_group(wsg, xtree_wstn->subel[trix],
                name_space, wstc->wstc_itm.wstcu_wsgroup);

        } else if (xml_name_space_equals(xtree_wstn->subel[trix]->elname,
                    name_space)) {
            wstat = ws_set_error(wsg, WSE_EXTENSIONCONTENT,
                        "%s", xtree_wstn->subel[trix]->elname);
        }
        trix++;
    }

    return (wstat);
}
/***************************************************************/
int parse_wsdl_sch_extension(struct wsglob * wsg,
                    struct xmltree * xtree_wstn,
                    const char * name_space,
                    struct wsextension * wstn)
{
/*
<extension
  base = QName
  id = ID
  {any attributes with non-schema namespace . . .}>
  Content: (annotation?, openContent?, ((group | all | choice | sequence)?,
            ((attribute | attributeGroup)*, anyAttribute?), assert*))
</extension>
*/
    int wstat = 0;
    int atix;

    atix = 0;
    while (!wstat && atix < xtree_wstn->nattrs) {
        if (!strcmp(xtree_wstn->attrnam[atix], WSSH_TYP_BASE)) {
            wstn->wstn_base = Strdup(xtree_wstn->attrval[atix]);

        } else if (!strcmp(xtree_wstn->attrnam[atix], WSSH_TYP_ID)) {
            wstn->wstn_id = Strdup(xtree_wstn->attrval[atix]);

        } else {
            wstat = ws_set_error(wsg, WSE_BADEXTENSIONATTR,
                    "%s", xtree_wstn->attrnam[atix]);
        }
        atix++;
    }

    if (!wstat && xtree_wstn->nsubel) {
        wstat = parse_extension_content(wsg, xtree_wstn, name_space, wstn);
    }

    return (wstat);
}
/***************************************************************/
int parse_complex_content_content(struct wsglob * wsg,
                    struct xmltree * xtree_wstk,
                    const char * name_space,
                    struct wscomplexcontent * wstk) {
/*
  Content: (annotation?, (restriction | extension))
*/
    int wstat = 0;
    int trix;
    struct wscontent * wstc;

    trix = 0;
    while (!wstat && trix < xtree_wstk->nsubel) {
        if (xml_full_name_equals(xtree_wstk->subel[trix]->elname,
            name_space, WSSH_EXTENSION)) {
            wstc = add_new_complex_content_content(wstk, WSST_EXTENSION);
            wstc->wstc_itm.wstcu_wsextension = new_wsextension();
            wstat = parse_wsdl_sch_extension(wsg, xtree_wstk->subel[trix],
                name_space, wstc->wstc_itm.wstcu_wsextension);

        } else if (xml_full_name_equals(xtree_wstk->subel[trix]->elname,
            name_space, WSSH_ANNOTATION)) {
            wstc = add_new_complex_content_content(wstk, WSST_ANNOTATION);
            wstc->wstc_itm.wstcu_wsannotation = new_wsannotation();
            wstat = parse_wsdl_sch_annotation(wsg, xtree_wstk->subel[trix],
                name_space, wstc->wstc_itm.wstcu_wsannotation);

        } else if (xml_name_space_equals(xtree_wstk->subel[trix]->elname,
                    name_space)) {
            wstat = ws_set_error(wsg, WSE_COMPLEXCONTENTCONTENT,
                        "%s", xtree_wstk->subel[trix]->elname);
        }
        trix++;
    }

    return (wstat);
}
/***************************************************************/
int parse_wsdl_sch_complex_content(struct wsglob * wsg,
                    struct xmltree * xtree_wstk,
                    const char * name_space,
                    struct wscomplexcontent * wstk)
{
/*
<complexContent
  id = ID
  mixed = boolean
  {any attributes with non-schema namespace . . .}>
  Content: (annotation?, (restriction | extension))
</complexContent>
*/
    int wstat = 0;
    int atix;

    atix = 0;
    while (!wstat && atix < xtree_wstk->nattrs) {
        if (!strcmp(xtree_wstk->attrnam[atix], WSSH_TYP_ID)) {
            wstk->wstk_id = Strdup(xtree_wstk->attrval[atix]);

        } else if (!strcmp(xtree_wstk->attrnam[atix], WSSH_TYP_MIXED)) {
            wstk->wstk_mixed = Strdup(xtree_wstk->attrval[atix]);

        } else {
            wstat = ws_set_error(wsg, WSE_BADCOMPLEXCONTENTATTR,
                    "%s", xtree_wstk->attrnam[atix]);
        }
        atix++;
    }

    if (!wstat && xtree_wstk->nsubel) {
        wstat = parse_complex_content_content(wsg, xtree_wstk, name_space, wstk);
    }

    return (wstat);
}
/***************************************************************/
int parse_any_attribute_content(struct wsglob * wsg,
                    struct xmltree * xtree_wsty,
                    const char * name_space,
                    struct wsanyattribute * wsty) {
/*
  Content: (annotation?)
*/
    int wstat = 0;
    int trix;
    struct wscontent * wstc;

    trix = 0;
    while (!wstat && trix < xtree_wsty->nsubel) {
        if (xml_full_name_equals(xtree_wsty->subel[trix]->elname,
            name_space, WSSH_ANNOTATION)) {
            wstc = add_new_anyattribute_content(wsty, WSST_ANNOTATION);
            wstc->wstc_itm.wstcu_wsannotation = new_wsannotation();
            wstat = parse_wsdl_sch_annotation(wsg, xtree_wsty->subel[trix],
                name_space, wstc->wstc_itm.wstcu_wsannotation);

        } else if (xml_name_space_equals(xtree_wsty->subel[trix]->elname,
                    name_space)) {
            wstat = ws_set_error(wsg, WSE_ANYATTRIBUTECONTENT,
                        "%s", xtree_wsty->subel[trix]->elname);
        }
        trix++;
    }

    return (wstat);
}
/***************************************************************/
int parse_wsdl_sch_any_attribute(struct wsglob * wsg,
                    struct xmltree * xtree_wsty,
                    const char * name_space,
                    struct wsanyattribute * wsty)
{
/*
<anyAttribute
  id = ID
  namespace = ((##any | ##other) | List of (anyURI | (##targetNamespace | ##local)) ) 
  notNamespace = List of (anyURI | (##targetNamespace | ##local)) 
  notQName = List of (QName | ##defined) 
  processContents = (lax | skip | strict) : strict
  {any attributes with non-schema namespace . . .}>
  Content: (annotation?)
</anyAttribute>
*/
    int wstat = 0;
    int atix;

    atix = 0;
    while (!wstat && atix < xtree_wsty->nattrs) {
        if (!strcmp(xtree_wsty->attrnam[atix], WSSH_TYP_ID)) {
            wsty->wsty_id = Strdup(xtree_wsty->attrval[atix]);

        } else if (!strcmp(xtree_wsty->attrnam[atix], WSSH_TYP_NAMESPACE)) {
            wsty->wsty_namespace = Strdup(xtree_wsty->attrval[atix]);

        } else if (!strcmp(xtree_wsty->attrnam[atix], WSSH_TYP_NOTNAMESPACE)) {
            wsty->wsty_notNamespace = Strdup(xtree_wsty->attrval[atix]);

        } else if (!strcmp(xtree_wsty->attrnam[atix], WSSH_TYP_NOTQNAME)) {
            wsty->wsty_notQName = Strdup(xtree_wsty->attrval[atix]);

        } else if (!strcmp(xtree_wsty->attrnam[atix], WSSH_TYP_PROCESSCONTENTS)) {
            wsty->wsty_processContents = Strdup(xtree_wsty->attrval[atix]);

        } else {
            wstat = ws_set_error(wsg, WSE_BADANYATTRIBUTEATTR,
                    "%s", xtree_wsty->attrnam[atix]);
        }
        atix++;
    }

    if (!wstat && xtree_wsty->nsubel) {
        wstat = parse_any_attribute_content(wsg, xtree_wsty, name_space, wsty);
    }

    return (wstat);
}









/***************************************************************/
int parse_any_content(struct wsglob * wsg,
                    struct xmltree * xtree_wstz,
                    const char * name_space,
                    struct wsany * wstz) {
/*
  Content: (annotation?)
*/
    int wstat = 0;
    int trix;
    struct wscontent * wstc;

    trix = 0;
    while (!wstat && trix < xtree_wstz->nsubel) {
        if (xml_full_name_equals(xtree_wstz->subel[trix]->elname,
            name_space, WSSH_ANNOTATION)) {
            wstc = add_new_any_content(wstz, WSST_ANNOTATION);
            wstc->wstc_itm.wstcu_wsannotation = new_wsannotation();
            wstat = parse_wsdl_sch_annotation(wsg, xtree_wstz->subel[trix],
                name_space, wstc->wstc_itm.wstcu_wsannotation);


        } else if (xml_name_space_equals(xtree_wstz->subel[trix]->elname,
                    name_space)) {
            wstat = ws_set_error(wsg, WSE_ANYATTRIBUTECONTENT,
                        "%s", xtree_wstz->subel[trix]->elname);
        }
        trix++;
    }

    return (wstat);
}
/***************************************************************/
int parse_wsdl_sch_any(struct wsglob * wsg,
                    struct xmltree * xtree_wstz,
                    const char * name_space,
                    struct wsany * wstz)
{
/*
<any
  id = ID
  maxOccurs = (nonNegativeInteger | unbounded)  : 1
  minOccurs = nonNegativeInteger : 1
  namespace = ((##any | ##other) | List of (anyURI | (##targetNamespace | ##local)) ) 
  notNamespace = List of (anyURI | (##targetNamespace | ##local)) 
  notQName = List of (QName | (##defined | ##definedSibling)) 
  processContents = (lax | skip | strict) : strict
  {any attributes with non-schema namespace . . .}>
  Content: (annotation?)
</any>
*/
    int wstat = 0;
    int atix;

    atix = 0;
    while (!wstat && atix < xtree_wstz->nattrs) {
        if (!strcmp(xtree_wstz->attrnam[atix], WSSH_TYP_ID)) {
            wstz->wstz_id = Strdup(xtree_wstz->attrval[atix]);

        } else if (!strcmp(xtree_wstz->attrnam[atix], WSSH_TYP_MAXOCCURS)) {
            wstz->wstz_maxOccurs = Strdup(xtree_wstz->attrval[atix]);

        } else if (!strcmp(xtree_wstz->attrnam[atix], WSSH_TYP_MINOCCURS)) {
            wstz->wstz_minOccurs = Strdup(xtree_wstz->attrval[atix]);

        } else if (!strcmp(xtree_wstz->attrnam[atix], WSSH_TYP_NAMESPACE)) {
            wstz->wstz_namespace = Strdup(xtree_wstz->attrval[atix]);

        } else if (!strcmp(xtree_wstz->attrnam[atix], WSSH_TYP_NOTNAMESPACE)) {
            wstz->wstz_notNamespace = Strdup(xtree_wstz->attrval[atix]);

        } else if (!strcmp(xtree_wstz->attrnam[atix], WSSH_TYP_NOTQNAME)) {
            wstz->wstz_notQName = Strdup(xtree_wstz->attrval[atix]);

        } else if (!strcmp(xtree_wstz->attrnam[atix], WSSH_TYP_PROCESSCONTENTS)) {
            wstz->wstz_processContents = Strdup(xtree_wstz->attrval[atix]);

        } else {
            wstat = ws_set_error(wsg, WSE_BADANYATTRIBUTEATTR,
                    "%s", xtree_wstz->attrnam[atix]);
        }
        atix++;
    }

    if (!wstat && xtree_wstz->nsubel) {
        wstat = parse_any_content(wsg, xtree_wstz, name_space, wstz);
    }

    return (wstat);
}
/***************************************************************/
int parse_complex_type_content(struct wsglob * wsg,
                    struct xmltree * xtree_wstx,
                    const char * name_space,
                    struct wscomplextype * wstx) {
/*
  Content: (annotation?, (simpleContent | complexContent |
        (openContent?, (group | all | choice | sequence)?,
        ((attribute | attributeGroup)*, anyAttribute?), assert*)))
*/

    int wstat = 0;
    int trix;
    struct wscontent * wstc;

    trix = 0;
    while (!wstat && trix < xtree_wstx->nsubel) {
        if (xml_full_name_equals(xtree_wstx->subel[trix]->elname,
            name_space, WSSH_COMPLEX_TYPE)) {
            wstc = add_new_complex_type_content(wstx, WSST_COMPLEX_TYPE);
            wstc->wstc_itm.wstcu_wscomplextype = new_wscomplextype();
            wstat = parse_wsdl_sch_complex_type(wsg, xtree_wstx->subel[trix],
                name_space, wstc->wstc_itm.wstcu_wscomplextype);

        } else if (xml_full_name_equals(xtree_wstx->subel[trix]->elname,
            name_space, WSSH_COMPLEX_CONTENT)) {
            wstc = add_new_complex_type_content(wstx, WSST_COMPLEX_CONTENT);
            wstc->wstc_itm.wstcu_wscomplexcontent = new_wscomplexcontent();
            wstat = parse_wsdl_sch_complex_content(wsg, xtree_wstx->subel[trix],
                name_space, wstc->wstc_itm.wstcu_wscomplexcontent);

        } else if (xml_full_name_equals(xtree_wstx->subel[trix]->elname,
            name_space, WSSH_SIMPLE_CONTENT)) {
            wstc = add_new_complex_type_content(wstx, WSST_SIMPLE_CONTENT);
            wstc->wstc_itm.wstcu_wssimplecontent = new_wssimplecontent();
            wstat = parse_wsdl_sch_simple_content(wsg, xtree_wstx->subel[trix],
                name_space, wstc->wstc_itm.wstcu_wssimplecontent);

        } else if (xml_full_name_equals(xtree_wstx->subel[trix]->elname,
            name_space, WSSH_SEQUENCE)) {
            wstc = add_new_complex_type_content(wstx, WSST_SEQUENCE);
            wstc->wstc_itm.wstcu_wssequence = new_wssequence();
            wstat = parse_wsdl_sch_sequence(wsg, xtree_wstx->subel[trix],
                name_space, wstc->wstc_itm.wstcu_wssequence);

        } else if (xml_full_name_equals(xtree_wstx->subel[trix]->elname,
            name_space, WSSH_ANY_ATTRIBUTE)) {
            wstc = add_new_complex_type_content(wstx, WSST_ANY_ATTRIBUTE);
            wstc->wstc_itm.wstcu_wsanyattribute = new_wsanyattribute();
            wstat = parse_wsdl_sch_any_attribute(wsg, xtree_wstx->subel[trix],
                name_space, wstc->wstc_itm.wstcu_wsanyattribute);

        } else if (xml_full_name_equals(xtree_wstx->subel[trix]->elname,
            name_space, WSSH_ATTRIBUTE)) {
            wstc = add_new_complex_type_content(wstx, WSST_ATTRIBUTE);
            wstc->wstc_itm.wstcu_wsattribute = new_wsattribute();
            wstat = parse_wsdl_sch_attribute(wsg, xtree_wstx->subel[trix],
                name_space, wstc->wstc_itm.wstcu_wsattribute);

        } else if (xml_full_name_equals(xtree_wstx->subel[trix]->elname,
            name_space, WSSH_ANNOTATION)) {
            wstc = add_new_complex_type_content(wstx, WSST_ANNOTATION);
            wstc->wstc_itm.wstcu_wsannotation = new_wsannotation();
            wstat = parse_wsdl_sch_annotation(wsg, xtree_wstx->subel[trix],
                name_space, wstc->wstc_itm.wstcu_wsannotation);

        } else if (xml_full_name_equals(xtree_wstx->subel[trix]->elname,
            name_space, WSSH_ALL)) {
            wstc = add_new_complex_type_content(wstx, WSST_ALL);
            wstc->wstc_itm.wstcu_wsall = new_wsall();
            wstat = parse_wsdl_sch_all(wsg, xtree_wstx->subel[trix],
                name_space, wstc->wstc_itm.wstcu_wsall);

        } else if (xml_full_name_equals(xtree_wstx->subel[trix]->elname,
            name_space, WSSH_CHOICE)) {
            wstc = add_new_complex_type_content(wstx, WSST_CHOICE);
            wstc->wstc_itm.wstcu_wschoice = new_wschoice();
            wstat = parse_wsdl_sch_choice(wsg, xtree_wstx->subel[trix],
                name_space, wstc->wstc_itm.wstcu_wschoice);

        } else if (xml_full_name_equals(xtree_wstx->subel[trix]->elname,
            name_space, WSSH_GROUP)) {
            wstc = add_new_complex_type_content(wstx, WSST_GROUP);
            wstc->wstc_itm.wstcu_wsgroup = new_wsgroup();
            wstat = parse_wsdl_sch_group(wsg, xtree_wstx->subel[trix],
                name_space, wstc->wstc_itm.wstcu_wsgroup);

        } else if (xml_name_space_equals(xtree_wstx->subel[trix]->elname,
                    name_space)) {
            wstat = ws_set_error(wsg, WSE_COMPLEXTYPECONTENT,
                        "%s", xtree_wstx->subel[trix]->elname);
        }
        trix++;
    }

    return (wstat);
}
/***************************************************************/
int parse_wsdl_sch_complex_type(struct wsglob * wsg,
                    struct xmltree * xtree_wstx,
                    const char * name_space,
                    struct wscomplextype * wstx) {
/*
3.4.2 XML Representation of Complex Type Definition Schema Components

<complexType
  abstract = boolean : false
  block = (#all | List of (extension | restriction)) 
  final = (#all | List of (extension | restriction)) 
  id = ID
  mixed = boolean
  name = NCName
  defaultAttributesApply = boolean : true
  {any attributes with non-schema namespace . . .}>
</complexType>
*/

    int wstat = 0;
    int atix;

    atix = 0;
    while (!wstat && atix < xtree_wstx->nattrs) {
        if (!strcmp(xtree_wstx->attrnam[atix], WSSH_TYP_ABSTRACT)) {
            wstx->wstx_abstract = Strdup(xtree_wstx->attrval[atix]);

        } else if (!strcmp(xtree_wstx->attrnam[atix], WSSH_TYP_BLOCK)) {
            wstx->wstx_block = Strdup(xtree_wstx->attrval[atix]);

        } else if (!strcmp(xtree_wstx->attrnam[atix], WSSH_TYP_FINAL)) {
            wstx->wstx_final = Strdup(xtree_wstx->attrval[atix]);

        } else if (!strcmp(xtree_wstx->attrnam[atix], WSSH_TYP_ID)) {
            wstx->wstx_id = Strdup(xtree_wstx->attrval[atix]);

        } else if (!strcmp(xtree_wstx->attrnam[atix], WSSH_TYP_MIXED)) {
            wstx->wstx_mixed = Strdup(xtree_wstx->attrval[atix]);

        } else if (!strcmp(xtree_wstx->attrnam[atix], WSSH_TYP_NAME)) {
            wstx->wstx_name = Strdup(xtree_wstx->attrval[atix]);

        } else if (!strcmp(xtree_wstx->attrnam[atix],
                           WSSH_TYP_DEFAULTATTRIBUTESAPPLY)) {
            wstx->wstx_defaultAttributesApply =
                Strdup(xtree_wstx->attrval[atix]);

        } else {
            wstat = ws_set_error(wsg, WSE_BADCOMPLEXTYPEATTR,
                    "%s", xtree_wstx->attrnam[atix]);
        }
        atix++;
    }

    if (!wstat && xtree_wstx->nsubel) {
        wstat = parse_complex_type_content(wsg, xtree_wstx, name_space, wstx);
    }

    return (wstat);
}
/***************************************************************/
int parse_element_content(struct wsglob * wsg,
                    struct xmltree * xtree_wste,
                    const char * name_space,
                    struct wselement * wste) {
/*
** Content: (annotation?, ((simpleType | complexType)?, alternative*,
**                          (unique | key | keyref)*))
*/
    int wstat = 0;
    int trix;
    struct wscontent * wstc;

    trix = 0;
    while (!wstat && trix < xtree_wste->nsubel) {
        if (xml_full_name_equals(xtree_wste->subel[trix]->elname,
            name_space, WSSH_COMPLEX_TYPE)) {
            wstc = add_new_element_content(wste, WSST_COMPLEX_TYPE);
            wstc->wstc_itm.wstcu_wscomplextype = new_wscomplextype();
            wstat = parse_wsdl_sch_complex_type(wsg, xtree_wste->subel[trix],
                name_space, wstc->wstc_itm.wstcu_wscomplextype);

        } else if (xml_full_name_equals(xtree_wste->subel[trix]->elname,
            name_space, WSSH_SIMPLE_TYPE)) {
            wstc = add_new_element_content(wste, WSST_SIMPLE_TYPE);
            wstc->wstc_itm.wstcu_wssimpletype = new_wssimpletype();
            wstat = parse_wsdl_sch_simple_type(wsg, xtree_wste->subel[trix],
                name_space, wstc->wstc_itm.wstcu_wssimpletype);

        } else if (xml_full_name_equals(xtree_wste->subel[trix]->elname,
            name_space, WSSH_ANNOTATION)) {
            wstc = add_new_element_content(wste, WSST_ANNOTATION);
            wstc->wstc_itm.wstcu_wsannotation = new_wsannotation();
            wstat = parse_wsdl_sch_annotation(wsg, xtree_wste->subel[trix],
                name_space, wstc->wstc_itm.wstcu_wsannotation);

        } else if (xml_name_space_equals(xtree_wste->subel[trix]->elname,
                    name_space)) {
            wstat = ws_set_error(wsg, WSE_ELEMENTCONTENT,
                        "%s", xtree_wste->subel[trix]->elname);
        }
        trix++;
    }

    return (wstat);
}
/***************************************************************/
int parse_wsdl_sch_element(struct wsglob * wsg,
                    struct xmltree * xtree_wste,
                    const char * name_space,
                    struct wselement * wste)
{
/*
** See: http://www.w3.org/TR/xmlschema11-2/type-hierarchy-201104.longdesc.html
*/
    int wstat = 0;
    int atix;

    atix = 0;
    while (!wstat && atix < xtree_wste->nattrs) {
        if (!strcmp(xtree_wste->attrnam[atix], WSSH_TYP_ABSTRACT)) {
            wste->wste_abstract = Strdup(xtree_wste->attrval[atix]);

        } else if (!strcmp(xtree_wste->attrnam[atix], WSSH_TYP_BLOCK)) {
            wste->wste_block = Strdup(xtree_wste->attrval[atix]);

        } else if (!strcmp(xtree_wste->attrnam[atix], WSSH_TYP_DEFAULT)) {
            wste->wste_default = Strdup(xtree_wste->attrval[atix]);

        } else if (!strcmp(xtree_wste->attrnam[atix], WSSH_TYP_FINAL)) {
            wste->wste_final = Strdup(xtree_wste->attrval[atix]);

        } else if (!strcmp(xtree_wste->attrnam[atix], WSSH_TYP_FIXED)) {
            wste->wste_fixed = Strdup(xtree_wste->attrval[atix]);

        } else if (!strcmp(xtree_wste->attrnam[atix], WSSH_TYP_FORM)) {
            wste->wste_form = Strdup(xtree_wste->attrval[atix]);

        } else if (!strcmp(xtree_wste->attrnam[atix], WSSH_TYP_ID)) {
            wste->wste_id = Strdup(xtree_wste->attrval[atix]);

        } else if (!strcmp(xtree_wste->attrnam[atix], WSSH_TYP_MAXOCCURS)) {
            wste->wste_maxOccurs = Strdup(xtree_wste->attrval[atix]);

        } else if (!strcmp(xtree_wste->attrnam[atix], WSSH_TYP_MINOCCURS)) {
            wste->wste_minOccurs = Strdup(xtree_wste->attrval[atix]);

        } else if (!strcmp(xtree_wste->attrnam[atix], WSSH_TYP_NAME)) {
            wste->wste_name = Strdup(xtree_wste->attrval[atix]);

        } else if (!strcmp(xtree_wste->attrnam[atix], WSSH_TYP_NILLABLE)) {
            wste->wste_nillable = Strdup(xtree_wste->attrval[atix]);

        } else if (!strcmp(xtree_wste->attrnam[atix], WSSH_TYP_REF)) {
            wste->wste_ref = Strdup(xtree_wste->attrval[atix]);

        } else if (!strcmp(xtree_wste->attrnam[atix],
                            WSSH_TYP_SUBSTITUTIONGROUP)) {
            wste->wste_substitutionGroup = Strdup(xtree_wste->attrval[atix]);

        } else if (!strcmp(xtree_wste->attrnam[atix],
                            WSSH_TYP_TARGETNAMESPACE)) {
            wste->wste_targetNamespace = Strdup(xtree_wste->attrval[atix]);

        } else if (!strcmp(xtree_wste->attrnam[atix], WSSH_TYP_TYPE)) {
            wste->wste_type = Strdup(xtree_wste->attrval[atix]);

        } else {
            wstat = ws_set_error(wsg, WSE_BADELEMENTATTR,
                    "%s", xtree_wste->attrnam[atix]);
        }
        atix++;
    }

    if (!wstat && xtree_wste->nsubel) {
        wstat = parse_element_content(wsg, xtree_wste, name_space, wste);
    }

    return (wstat);
}
/***************************************************************/
int parse_wsdl_sch_import(struct wsglob * wsg,
                    struct xmltree * xtree_wstf,
                    const char * name_space,
                    struct wsimport * wstf) {
/*
**** Does not do anything now ****

<import
  id = ID
  namespace = anyURI
  schemaLocation = anyURI
  {any attributes with non-schema namespace . . .}>
  Content: (annotation?)
</import>
*/

    int wstat = 0;
    int atix;

    atix = 0;
    while (!wstat && atix < xtree_wstf->nattrs) {
        if (!strcmp(xtree_wstf->attrnam[atix], WSSH_TYP_ID)) {
            wstf->wstf_id = Strdup(xtree_wstf->attrval[atix]);

        } else if (!strcmp(xtree_wstf->attrnam[atix], WSSH_TYP_NAMESPACE)) {
            wstf->wstf_namespace = Strdup(xtree_wstf->attrval[atix]);

        } else if (!strcmp(xtree_wstf->attrnam[atix],
                           WSSH_TYP_SCHEMALOCATION)) {
            wstf->wstf_schemaLocation =
                Strdup(xtree_wstf->attrval[atix]);

        } else {
            wstat = ws_set_error(wsg, WSE_BADCOMPLEXTYPEATTR,
                    "%s", xtree_wstf->attrnam[atix]);
        }
        atix++;
    }

    if (!wstat && xtree_wstf->nsubel) {
            wstat = ws_set_error(wsg, WSE_NOCONTENTEXPECTED,
                    "%s <%s>", xtree_wstf->elname,
                    xtree_wstf->subel[0]->elname);
    }

    return (wstat);
}
/***************************************************************/
int parse_wsdl_schema_content(struct wsglob * wsg,
                    struct xmltree * xtree_wsts,
                    struct wsschema * wsts) {
/*
** See: http://www.w3.org/2001/XMLSchema
** See: http://www.w3schools.com/schema/schema_elements_ref.asp

<schema
  Content: ((include | import | redefine | override | annotation)*,
        (defaultOpenContent, annotation*)?,
        ((simpleType | complexType | group | attributeGroup | element |
          attribute | notation), annotation*)*)
</schema>
*/
    int wstat = 0;
    int trix;
    char * attr_val;
    struct wscontent * wstc;
    struct wsimport * wstf;
    char * ens_uri;
    int content_id;
    char content_name[32]; /* for annotations */
    char el_name_space[XML_NAMESPACE_SZ];

    trix = 0;
    content_id = 0;

    while (!wstat && trix < xtree_wsts->nsubel) {
        xml_get_name_space(xtree_wsts->subel[trix]->elname,
            el_name_space, XML_NAMESPACE_SZ);

        ens_uri = nsl_find_name_space(wsts->wsts_nslist,
                                      el_name_space, 1);
        if (!ens_uri) {
                wstat = ws_set_error(wsg, WSE_SCHEMANAMESPACE,
                            "%s", xtree_wsts->subel[trix]->elname);
        } else if (strcmp(ens_uri, NSURI_XSD)) {
            /* not xsd - ignore */
        } else {
            /* element */
            if (xml_item_name_equals(xtree_wsts->subel[trix]->elname,
                    WSSH_ELEMENT)) {
                attr_val = xml_find_attr(xtree_wsts->subel[trix], WSSH_NAME);
                if (!attr_val) {
                    wstat = ws_set_error(wsg, WSE_EXPNAME,
                            "%s", xtree_wsts->subel[trix]->elname);
                } else {
                    wstc = add_new_schema_content(wsts, attr_val, WSST_ELEMENT);
                    if (!wstc) {
                        wstat = ws_set_error(wsg, WSE_DUPELEMENT,
                            "%s", attr_val);
                    } else {
                        wstc->wstc_itm.wstcu_wselement = new_wselement();
                        wstat = parse_wsdl_sch_element(wsg, xtree_wsts->subel[trix],
                                el_name_space,
                                wstc->wstc_itm.wstcu_wselement);
                    }
                }

            /* simple type */
            } else if (xml_item_name_equals(xtree_wsts->subel[trix]->elname,
                    WSSH_SIMPLE_TYPE)) {
                attr_val = xml_find_attr(xtree_wsts->subel[trix], WSSH_NAME);
                if (!attr_val) {
                    wstat = ws_set_error(wsg, WSE_EXPNAME,
                            "%s", xtree_wsts->subel[trix]->elname);
                } else {
                    wstc = add_new_schema_content(wsts, attr_val, WSST_SIMPLE_TYPE);
                    if (!wstc) {
                        wstat = ws_set_error(wsg, WSE_DUPELEMENT,
                            "%s", attr_val);
                    } else {
                        wstc->wstc_itm.wstcu_wssimpletype = new_wssimpletype();
                        wstat = parse_wsdl_sch_simple_type(wsg,
                                xtree_wsts->subel[trix],
                                el_name_space,
                                wstc->wstc_itm.wstcu_wssimpletype);
                    }
                }

            /* complex type */
            } else if (xml_item_name_equals(xtree_wsts->subel[trix]->elname,
                    WSSH_COMPLEX_TYPE)) {
                attr_val = xml_find_attr(xtree_wsts->subel[trix], WSSH_NAME);
                if (!attr_val) {
                    wstat = ws_set_error(wsg, WSE_EXPNAME,
                            "%s", xtree_wsts->subel[trix]->elname);
                } else {
                    wstc = add_new_schema_content(wsts, attr_val, WSST_COMPLEX_TYPE);
                    if (!wstc) {
                        wstat = ws_set_error(wsg, WSE_DUPELEMENT,
                            "%s", attr_val);
                    } else {
                        wstc->wstc_itm.wstcu_wscomplextype = new_wscomplextype();
                        wstat = parse_wsdl_sch_complex_type(wsg,
                                xtree_wsts->subel[trix],
                                el_name_space,
                                wstc->wstc_itm.wstcu_wscomplextype);
                    }
                }

            /* import */
            } else if (xml_item_name_equals(xtree_wsts->subel[trix]->elname,
                    WSSH_IMPORT)) {
                wstf = new_wsimport();
                wstat = parse_wsdl_sch_import(wsg,
                        xtree_wsts->subel[trix],
                        el_name_space, wstf);
                free_wsimport(wstf);

            /* annotation */
            } else if (xml_item_name_equals(xtree_wsts->subel[trix]->elname,
                            WSSH_ANNOTATION)) {
                sprintf(content_name, "%s_%04d", "##annotation", content_id++);
                wstc = add_new_schema_content(wsts, content_name, WSST_ANNOTATION);
                wstc->wstc_itm.wstcu_wsannotation = new_wsannotation();
                wstat = parse_wsdl_sch_annotation(wsg, xtree_wsts->subel[trix],
                    el_name_space, wstc->wstc_itm.wstcu_wsannotation);

            } else {
                wstat = ws_set_error(wsg, WSE_EXPELEMENT,
                            "%s", xtree_wsts->subel[trix]->elname);
            }
        }
        trix++;
    }

    return (wstat);
}
/***************************************************************/
int parse_wsdl_schema(struct wsglob * wsg,
            struct xmltree * xtree_wsts,
            struct wsschema * wsts) {
/*
** See: http://www.w3.org/2001/XMLSchema
** See: http://www.w3schools.com/schema/schema_elements_ref.asp

<schema
  attributeFormDefault = (qualified | unqualified) : unqualified
  blockDefault = (#all | List of (extension | restriction | substitution))  : ''
  defaultAttributes = QName
  xpathDefaultNamespace = (anyURI | (##defaultNamespace | ##targetNamespace | ##local))  : ##local
  elementFormDefault = (qualified | unqualified) : unqualified
  finalDefault = (#all | List of (extension | restriction | list | union))  : ''
  id = ID
  targetNamespace = anyURI
  version = token
  xml:lang = language
  {any attributes with non-schema namespace . . .}>
  Content: ((include | import | redefine | override | annotation)*,
        (defaultOpenContent, annotation*)?,
        ((simpleType | complexType | group | attributeGroup | element |
          attribute | notation), annotation*)*)
</schema>
*/
    int wstat = 0;
    int atix;

    xml_get_name_space(xtree_wsts->elname, wsts->wsts_name_space, XML_NAMESPACE_SZ);

    atix = 0;
    while (!wstat && atix < xtree_wsts->nattrs) {
        if (!memcmp(xtree_wsts->attrnam[atix], "xmlns:", 6)) {
            nsl_insert_name_space(wsts->wsts_nslist,
                xtree_wsts->attrnam[atix] + 6, 
                xtree_wsts->attrval[atix]);

        } else if (!strcmp(xtree_wsts->attrnam[atix], WSSH_TYP_XMLNS)) {
            nsl_insert_name_space(wsts->wsts_nslist,
                "", 
                xtree_wsts->attrval[atix]);

        } else if (!strcmp(xtree_wsts->attrnam[atix], WSSH_TYP_ATTRIBUTEFORMDEFAULT)) {
            wsts->wsts_attributeFormDefault = Strdup(xtree_wsts->attrval[atix]);

        } else if (!strcmp(xtree_wsts->attrnam[atix], WSSH_TYP_BLOCKDEFAULT)) {
            wsts->wsts_blockDefault = Strdup(xtree_wsts->attrval[atix]);

        } else if (!strcmp(xtree_wsts->attrnam[atix], WSSH_TYP_DEFAULTATTRIBUTES)) {
            wsts->wsts_defaultAttributes = Strdup(xtree_wsts->attrval[atix]);

        } else if (!strcmp(xtree_wsts->attrnam[atix], WSSH_TYP_XPATHDEFAULTNAMESPACE)) {
            wsts->wsts_xpathDefaultNamespace = Strdup(xtree_wsts->attrval[atix]);

        } else if (!strcmp(xtree_wsts->attrnam[atix], WSSH_TYP_ELEMENTFORMDEFAULT)) {
            wsts->wsts_elementFormDefault = Strdup(xtree_wsts->attrval[atix]);

        } else if (!strcmp(xtree_wsts->attrnam[atix], WSSH_TYP_FINALDEFAULT)) {
            wsts->wsts_finalDefault = Strdup(xtree_wsts->attrval[atix]);

        } else if (!strcmp(xtree_wsts->attrnam[atix], WSSH_TYP_ID)) {
            wsts->wsts_id = Strdup(xtree_wsts->attrval[atix]);

        } else if (!strcmp(xtree_wsts->attrnam[atix], WSSH_TYP_TARGETNAMESPACE)) {
            wsts->wsts_targetNamespace = Strdup(xtree_wsts->attrval[atix]);

        } else if (!strcmp(xtree_wsts->attrnam[atix], WSSH_TYP_VERSION)) {
            wsts->wsts_version = Strdup(xtree_wsts->attrval[atix]);

        } else if (!strcmp(xtree_wsts->attrnam[atix], WSSH_TYP_XML_LANG)) {
            wsts->wsts_xml_lang = Strdup(xtree_wsts->attrval[atix]);

        } else {
            wstat = ws_set_error(wsg, WSE_BADSCHEMAATTR,
                    "%s", xtree_wsts->attrnam[atix]);
        }
        atix++;
    }
#if TEST_NAME_SPACES
    nsl_test_name_spaces(wsts->wsts_nslist);
#endif

    if (!wstat && xtree_wsts->nsubel) {
        wstat = parse_wsdl_schema_content(wsg, xtree_wsts, wsts);
    }

    return (wstat);
}
/***************************************************************/
int parse_wsdl_wstypes(struct wsglob * wsg,
            struct wsdef * wsd,
            struct xmltree * xtree_wst,
            struct wstypes * wst) {

    int wstat = 0;
    int trix;
    struct wsschema * wsts;
    char * attr_val;
    char * tns;

    trix = 0;

    while (!wstat && trix < xtree_wst->nsubel) {
        if (xml_item_name_equals(xtree_wst->subel[trix]->elname, WSDL_SCHEMA)) {
            tns = NULL;
            attr_val = xml_find_attr(xtree_wst->subel[trix], WSSH_TYP_TARGETNAMESPACE);
            if (attr_val) {
                tns = nsl_find_name_space_uri(wsd->wsd_nslist,
                                              attr_val, 1);
            }

            wsts = add_new_wsschema(wst, tns);
            wstat = parse_wsdl_schema(wsg, xtree_wst->subel[trix], wsts);
        } else {
            wstat = ws_set_error(wsg, WSE_EXPSCHEMA,
                        "%s", xtree_wst->subel[trix]->elname);
        }
        trix++;
    }

    return (wstat);
}
/***************************************************************/
int parse_wsdl_wspopinfo(struct wsglob * wsg,
            struct xmltree * xtree_wsyi,
            const char * name_space,
            struct wspopinfo * wsyi) {

    int wstat = 0;
    int atix;

    atix = 0;
    while (!wstat && atix < xtree_wsyi->nattrs) {
        if (!strcmp(xtree_wsyi->attrnam[atix], WSSH_TYP_NAME)) {
            wsyi->wsyi_name = Strdup(xtree_wsyi->attrval[atix]);

        } else if (!strcmp(xtree_wsyi->attrnam[atix], WSSH_TYP_MESSAGE)) {
            wsyi->wsyi_message = Strdup(xtree_wsyi->attrval[atix]);

        } else {
            wstat = ws_set_error(wsg, WSE_BADPORTTYPEELATTR,
                    "%s", xtree_wsyi->attrnam[atix]);
        }
        atix++;
    }

    if (!wstat && xtree_wsyi->nsubel) {
            wstat = ws_set_error(wsg, WSE_NOCONTENTEXPECTED,
                    "%s <%s>", xtree_wsyi->elname,
                    xtree_wsyi->subel[0]->elname);
    }

    return (wstat);
}
/***************************************************************/
struct wspopinfo * add_new_wspopinfo(struct wspoperation * wsyo, int ptyp) {

    struct wspopinfo * wsyi;

    wsyi = new_wspopinfo(ptyp);
    wsyo->wsyo_awspopinfo =
        Realloc(wsyo->wsyo_awspopinfo, struct wspopinfo*, wsyo->wsyo_nwspopinfo + 1);
    wsyo->wsyo_awspopinfo[wsyo->wsyo_nwspopinfo] = wsyi;
    wsyo->wsyo_nwspopinfo += 1;

    return (wsyi);
}
/***************************************************************/
int parse_porttype_operation_content(struct wsglob * wsg,
                    struct xmltree * xtree_wsyo,
                    const char * name_space,
                    struct wspoperation * wsyo) {

    int wstat = 0;
    int trix;
    struct wspopinfo * wsyi;

    trix = 0;
    while (!wstat && trix < xtree_wsyo->nsubel) {
        if (xml_full_name_equals(xtree_wsyo->subel[trix]->elname,
            name_space, WSSH_INPUT)) {
            wsyi = add_new_wspopinfo(wsyo, WSYO_PTYP_INPUT);
            wstat = parse_wsdl_wspopinfo(wsg, xtree_wsyo->subel[trix],
                name_space, wsyi);

        } else if (xml_full_name_equals(xtree_wsyo->subel[trix]->elname,
            name_space, WSSH_OUTPUT)) {
            wsyi = add_new_wspopinfo(wsyo, WSYO_PTYP_OUTPUT);
            wstat = parse_wsdl_wspopinfo(wsg, xtree_wsyo->subel[trix],
                name_space, wsyi);

        } else if (xml_full_name_equals(xtree_wsyo->subel[trix]->elname,
            name_space, WSSH_FAULT)) {
            wsyi = add_new_wspopinfo(wsyo, WSYO_PTYP_FAULT);
            wstat = parse_wsdl_wspopinfo(wsg, xtree_wsyo->subel[trix],
                name_space, wsyi);

        } else if (xml_full_name_equals(xtree_wsyo->subel[trix]->elname,
            name_space, WSSH_DOCUMENTATION)) {
            wsyi = add_new_wspopinfo(wsyo, WSYO_PTYP_DOCUMENTATION);
            if (xtree_wsyo->subel[trix]->eldata) {
                wsyi->wsyi_cdata = Strdup(xtree_wsyo->subel[trix]->eldata);
            }

        } else {
            wstat = ws_set_error(wsg, WSE_PORTTYPECONTENT,
                        "%s", xtree_wsyo->subel[trix]->elname);
        }

        trix++;
    }

    return (wstat);
}
/***************************************************************/
int parse_wsdl_porttype_operation(struct wsglob * wsg,
            struct xmltree * xtree_wsyo,
            const char * name_space,
            struct wspoperation * wsyo) {

/*
    <wsdl:portType name="nmtoken">
        <wsdl:operation name="nmtoken" .... /> *
    </wsdl:portType>
*/
    int wstat = 0;
    int atix;

    atix = 0;
    while (!wstat && atix < xtree_wsyo->nattrs) {
        if (!strcmp(xtree_wsyo->attrnam[atix], WSSH_TYP_NAME)) {
            wsyo->wsyo_name = Strdup(xtree_wsyo->attrval[atix]);

        } else {
            wstat = ws_set_error(wsg, WSE_BADPORTTYPEATTR,
                    "%s", xtree_wsyo->attrnam[atix]);
        }
        atix++;
    }

    if (!wstat && xtree_wsyo->nsubel) {
        wstat = parse_porttype_operation_content(wsg, xtree_wsyo, name_space, wsyo);
    }

    return (wstat);
}
/***************************************************************/
int add_wspoperation_to_wsporttype(struct wsglob * wsg,
                            struct wsporttype * wsy,
                            struct wspoperation * wsyo) {

/*
** Duplicates allowed, but not supported yet.
*/
    int wstat = 0;
    void ** vhand;
    int nlen;
    int is_dup;
    struct wsyoplist * wsyl;

    if (!wsyo->wsyo_name) {
        wstat = ws_set_error(wsg, WSE_OPERATIONNAMEREQD, "");
        return (wstat);
    }

    nlen = strlen(wsyo->wsyo_name);
    if (!wsy->wsy_wsyoplist_dbt) {
        wsy->wsy_wsyoplist_dbt = dbtree_new();
        vhand = NULL;
    } else {
        vhand = dbtree_find(wsy->wsy_wsyoplist_dbt,
                        wsyo->wsyo_name, nlen + 1);
    }

    if (vhand) {
        wsyl = *((struct wsyoplist **)vhand);
    } else {
        wsyl = new_wsyoplist();
        is_dup = dbtree_insert(wsy->wsy_wsyoplist_dbt,
                        wsyo->wsyo_name, nlen + 1, wsyl);
    }

    wsyl->wsyl_awspoperation =
        Realloc(wsyl->wsyl_awspoperation, struct wspopinfo*, wsyl->wsyl_nwspoperation + 1);
    wsyl->wsyl_awspoperation[wsyl->wsyl_nwspoperation] = wsyo;
    wsyl->wsyl_nwspoperation += 1;

    return (wstat);
}
/***************************************************************/
int parse_porttype_content(struct wsglob * wsg,
                    struct xmltree * xtree_wsy,
                    const char * name_space,
                    struct wsporttype * wsy) {

    int wstat = 0;
    int trix;
    struct wspoperation * wsyo;

    trix = 0;
    while (!wstat && trix < xtree_wsy->nsubel) {
        if (xml_full_name_equals(xtree_wsy->subel[trix]->elname,
            name_space, WSSH_OPERATION)) {
            wsyo = new_wspoperation();
            wstat = parse_wsdl_porttype_operation(wsg, xtree_wsy->subel[trix],
                    name_space, wsyo);
            if (!wstat) {
                wstat = add_wspoperation_to_wsporttype(wsg, wsy, wsyo);
            }

        } else {
            wstat = ws_set_error(wsg, WSE_PORTTYPECONTENT,
                        "%s", xtree_wsy->subel[trix]->elname);
        }

        trix++;
    }

    return (wstat);
}
/***************************************************************/
int parse_wsdl_wsporttype(struct wsglob * wsg,
            struct xmltree * xtree_wsy,
            const char * name_space,
            struct wsporttype * wsy) {

/*
    <wsdl:portType name="nmtoken">
        <wsdl:operation name="nmtoken" .... /> *
    </wsdl:portType>
*/
    int wstat = 0;
    int atix;

    atix = 0;
    while (!wstat && atix < xtree_wsy->nattrs) {
        if (!strcmp(xtree_wsy->attrnam[atix], WSSH_TYP_NAME)) {
            wsy->wsy_name = Strdup(xtree_wsy->attrval[atix]);

        } else {
            wstat = ws_set_error(wsg, WSE_BADPORTTYPEATTR,
                    "%s", xtree_wsy->attrnam[atix]);
        }
        atix++;
    }

    if (!wstat && xtree_wsy->nsubel) {
        wstat = parse_porttype_content(wsg, xtree_wsy, name_space, wsy);
    }

    return (wstat);
}
/***************************************************************/
int wsd_get_binding_type(struct wsdef * wsd,
                    const char * binding_name_space) {

    int binding_type;
    char * name_space_uri;

    binding_type = 0;
    name_space_uri = nsl_find_name_space(wsd->wsd_nslist, binding_name_space, 1);

    if (name_space_uri) {
        if (!strcmp(name_space_uri, NSURI_SOAP)) {
            binding_type = WSB_BINDING_TYPE_SOAP;
        } else if (!strcmp(name_space_uri, NSURI_SOAP12)) {
            binding_type = WSB_BINDING_TYPE_SOAP12;
        }
    }

    return (binding_type);
}
/***************************************************************/
int add_wsboperation_to_wsbinding(struct wsglob * wsg,
                            struct wsbinding * wsb,
                            struct wsboperation * wsbo) {

/*
** Duplicates allowed, but not supported yet.
*/
    int wstat = 0;
    void ** vhand;
    int nlen;
    int is_dup;
    struct wsboplist * wsbl;

    if (!wsbo->wsbo_name) {
        wstat = ws_set_error(wsg, WSE_OPERATIONNAMEREQD, "");
        return (wstat);
    }

    nlen = strlen(wsbo->wsbo_name);
    if (!wsb->wsb_wsboplist_dbt) {
        wsb->wsb_wsboplist_dbt = dbtree_new();
        vhand = NULL;
    } else {
        vhand = dbtree_find(wsb->wsb_wsboplist_dbt,
                        wsbo->wsbo_name, nlen + 1);
    }

    if (vhand) {
        wsbl = *((struct wsboplist **)vhand);
    } else {
        wsbl = new_wsboplist();
        is_dup = dbtree_insert(wsb->wsb_wsboplist_dbt,
                        wsbo->wsbo_name, nlen + 1, wsbl);
    }

    wsbl->wsbl_awsboperation =
        Realloc(wsbl->wsbl_awsboperation, struct wsbopinfo*, wsbl->wsbl_nwsboperation + 1);
    wsbl->wsbl_awsboperation[wsbl->wsbl_nwsboperation] = wsbo;
    wsbl->wsbl_nwsboperation += 1;

    return (wstat);
}
/***************************************************************/
struct wssoapop * add_new_wssoapop(struct wsbopinfo * wsbi, int btyp) {

    struct wssoapop * wsbiso;

    wsbiso = new_wssoapop(btyp);
    wsbi->wsbi_awssoapop =
        Realloc(wsbi->wsbi_awssoapop, struct wssoapop*, wsbi->wsbi_nwssoapop + 1);
    wsbi->wsbi_awssoapop[wsbi->wsbi_nwssoapop] = wsbiso;
    wsbi->wsbi_nwssoapop += 1;

    return (wsbiso);
}
/***************************************************************/
struct wsbopinfo * add_new_wsbopinfo(struct wsboperation * wsbo, int btyp) {

    struct wsbopinfo * wsbi;

    wsbi = new_wsbopinfo(btyp);
    wsbo->wsbo_awsbopinfo =
        Realloc(wsbo->wsbo_awsbopinfo, struct wsbopinfo*, wsbo->wsbo_nwsbopinfo + 1);
    wsbo->wsbo_awsbopinfo[wsbo->wsbo_nwsbopinfo] = wsbi;
    wsbo->wsbo_nwsbopinfo += 1;

    return (wsbi);
}
/***************************************************************/
int parse_binding_operation_attrs_soap(struct wsglob * wsg,
            struct xmltree * xtree_wsbo,
            const char * soap_name_space,
            struct wsboperation * wsbo) {

    int wstat = 0;
    int atix;

    atix = 0;
    while (!wstat && atix < xtree_wsbo->nattrs) {
        if (!strcmp(xtree_wsbo->attrnam[atix], WSSH_TYP_SOAP_SOAP_ACTION)) {
            wsbo->wsbo_soap_soapAction = Strdup(xtree_wsbo->attrval[atix]);

        } else if (!strcmp(xtree_wsbo->attrnam[atix], WSSH_TYP_SOAP_STYPE)) {
            wsbo->wsbo_soap_style = Strdup(xtree_wsbo->attrval[atix]);

        }
        atix++;
    }

    if (!wstat && xtree_wsbo->nsubel) {
            wstat = ws_set_error(wsg, WSE_NOCONTENTEXPECTED,
                    "%s <%s>", xtree_wsbo->elname,
                    xtree_wsbo->subel[0]->elname);
    }

    return (wstat);
}
/***************************************************************/
int parse_binding_operation_wsbopinfo_soap_body_attrs(struct wsglob * wsg,
            struct xmltree * xtree_wsbiso,
            const char * name_space,
            const char * soap_name_space,
            struct wssoapop * wsbiso) {

    int wstat = 0;
    int atix;

    atix = 0;
    while (!wstat && atix < xtree_wsbiso->nattrs) {
        if (!strcmp(xtree_wsbiso->attrnam[atix], WSSH_TYP_USE)) {
            wsbiso->wsbiso_soap_body_use = Strdup(xtree_wsbiso->attrval[atix]);

        }
        atix++;
    }

    if (!wstat && xtree_wsbiso->nsubel) {
            wstat = ws_set_error(wsg, WSE_NOCONTENTEXPECTED,
                    "%s <%s>", xtree_wsbiso->elname,
                    xtree_wsbiso->subel[0]->elname);
    }

    return (wstat);
}
/***************************************************************/
int parse_binding_operation_wsbopinfo_soap_header_attrs(struct wsglob * wsg,
            struct xmltree * xtree_wsbiso,
            const char * name_space,
            const char * soap_name_space,
            struct wssoapop * wsbiso) {

    int wstat = 0;
    int atix;

    atix = 0;
    while (!wstat && atix < xtree_wsbiso->nattrs) {
        if (!strcmp(xtree_wsbiso->attrnam[atix], WSSH_TYP_MESSAGE)) {
            wsbiso->wsbiso_soap_header_message = Strdup(xtree_wsbiso->attrval[atix]);

        } else if (!strcmp(xtree_wsbiso->attrnam[atix], WSSH_TYP_PART)) {
            wsbiso->wsbiso_soap_header_part = Strdup(xtree_wsbiso->attrval[atix]);

        } else if (!strcmp(xtree_wsbiso->attrnam[atix], WSSH_TYP_USE)) {
            wsbiso->wsbiso_soap_header_use = Strdup(xtree_wsbiso->attrval[atix]);

        }
        atix++;
    }

    if (!wstat && xtree_wsbiso->nsubel) {
            wstat = ws_set_error(wsg, WSE_NOCONTENTEXPECTED,
                    "%s <%s>", xtree_wsbiso->elname,
                    xtree_wsbiso->subel[0]->elname);
    }

    return (wstat);
}
/***************************************************************/
int parse_binding_operation_wsbopinfo_soap_fault_attrs(struct wsglob * wsg,
            struct xmltree * xtree_wsbiso,
            const char * name_space,
            const char * soap_name_space,
            struct wssoapop * wsbiso) {

    int wstat = 0;
    int atix;

    atix = 0;
    while (!wstat && atix < xtree_wsbiso->nattrs) {
        if (!strcmp(xtree_wsbiso->attrnam[atix], WSSH_TYP_NAME)) {
            wsbiso->wsbiso_soap_fault_name = Strdup(xtree_wsbiso->attrval[atix]);

        } else if (!strcmp(xtree_wsbiso->attrnam[atix], WSSH_TYP_USE)) {
            wsbiso->wsbiso_soap_fault_use = Strdup(xtree_wsbiso->attrval[atix]);

        }
        atix++;
    }

    if (!wstat && xtree_wsbiso->nsubel) {
            wstat = ws_set_error(wsg, WSE_NOCONTENTEXPECTED,
                    "%s <%s>", xtree_wsbiso->elname,
                    xtree_wsbiso->subel[0]->elname);
    }

    return (wstat);
}
/***************************************************************/
int parse_binding_operation_wsbopinfo_soap(struct wsglob * wsg,
            struct xmltree * xtree_wsbi,
            const char * name_space,
            const char * soap_name_space,
            struct wsbopinfo * wsbi) {

    int wstat = 0;
    int trix;
    struct wssoapop * wsbiso;

    trix = 0;
    while (!wstat && trix < xtree_wsbi->nsubel) {
        /* soap name space */
        if (xml_full_name_equals(xtree_wsbi->subel[trix]->elname,
                    soap_name_space, WSSH_BODY)) {
            wsbiso = add_new_wssoapop(wsbi, WSBISO_SOAP_TYP_BODY);
            wstat = parse_binding_operation_wsbopinfo_soap_body_attrs(wsg,
                        xtree_wsbi->subel[trix],
                        name_space, soap_name_space, wsbiso);

        } else if (xml_full_name_equals(xtree_wsbi->subel[trix]->elname,
                    soap_name_space, WSSH_HEADER)) {
            wsbiso = add_new_wssoapop(wsbi, WSBISO_SOAP_TYP_HEADER);
            wstat = parse_binding_operation_wsbopinfo_soap_header_attrs(wsg,
                        xtree_wsbi->subel[trix],
                        name_space, soap_name_space, wsbiso);

        } else if (xml_full_name_equals(xtree_wsbi->subel[trix]->elname,
                    soap_name_space, WSSH_FAULT)) {
            wsbiso = add_new_wssoapop(wsbi, WSBISO_SOAP_TYP_FAULT);
            wstat = parse_binding_operation_wsbopinfo_soap_fault_attrs(wsg,
                        xtree_wsbi->subel[trix],
                        name_space, soap_name_space, wsbiso);

        } else {
            wstat = ws_set_error(wsg, WSE_SOAPOPCONTENT,
                        "%s", xtree_wsbi->subel[trix]->elname);
        }
        trix++;
    }

    return (wstat);
}
/***************************************************************/
int parse_binding_operation_content_soap(struct wsglob * wsg,
            struct xmltree * xtree_wsbo,
            const char * name_space,
            const char * soap_name_space,
            struct wsboperation * wsbo) {

    int wstat = 0;
    int trix;
    struct wsbopinfo * wsbi;

    trix = 0;
    while (!wstat && trix < xtree_wsbo->nsubel) {
        /* soap name space */
        if (xml_full_name_equals(xtree_wsbo->subel[trix]->elname,
                    soap_name_space, WSSH_OPERATION)) {
            wstat = parse_binding_operation_attrs_soap(wsg,
                    xtree_wsbo->subel[trix], soap_name_space, wsbo);

        /* wsdl name space */
        } else if (xml_full_name_equals(xtree_wsbo->subel[trix]->elname,
                    name_space, WSSH_INPUT)) {
            wsbi = add_new_wsbopinfo(wsbo, WSYO_BTYP_INPUT);
            wstat = parse_binding_operation_wsbopinfo_soap(wsg,
                        xtree_wsbo->subel[trix],
                        name_space, soap_name_space, wsbi);

        } else if (xml_full_name_equals(xtree_wsbo->subel[trix]->elname,
                    name_space, WSSH_OUTPUT)) {
            wsbi = add_new_wsbopinfo(wsbo, WSYO_BTYP_OUTPUT);
            wstat = parse_binding_operation_wsbopinfo_soap(wsg,
                        xtree_wsbo->subel[trix],
                        name_space, soap_name_space, wsbi);

        } else if (xml_full_name_equals(xtree_wsbo->subel[trix]->elname,
                    name_space, WSSH_FAULT)) {
            wsbi = add_new_wsbopinfo(wsbo, WSYO_BTYP_FAULT);
            wstat = parse_binding_operation_wsbopinfo_soap(wsg,
                        xtree_wsbo->subel[trix],
                        name_space, soap_name_space, wsbi);

        } else {
            wstat = ws_set_error(wsg, WSE_SERVICECONTENT,
                        "%s", xtree_wsbo->subel[trix]->elname);
        }
        trix++;
    }

    return (wstat);
}
/***************************************************************/
int parse_binding_operation_soap(struct wsglob * wsg,
            struct xmltree * xtree_wsbo,
            const char * name_space,
            const char * soap_name_space,
            struct wsboperation * wsbo) {

    int wstat = 0;
    int atix;

    atix = 0;
    while (!wstat && atix < xtree_wsbo->nattrs) {
        if (!strcmp(xtree_wsbo->attrnam[atix], WSSH_TYP_NAME)) {
            wsbo->wsbo_name = Strdup(xtree_wsbo->attrval[atix]);

        }
        atix++;
    }

    if (!wstat && xtree_wsbo->nsubel) {
        wstat = parse_binding_operation_content_soap(wsg,
                xtree_wsbo, name_space, soap_name_space, wsbo);
    }

    return (wstat);
}
/***************************************************************/
int parse_binding_attrs_soap(struct wsglob * wsg,
            struct xmltree * xtree_wsb,
            const char * name_space,
            struct wsbinding * wsb) {

    int wstat = 0;
    int atix;

    atix = 0;
    while (!wstat && atix < xtree_wsb->nattrs) {
        if (!strcmp(xtree_wsb->attrnam[atix], "transport")) {
            wsb->wsb_soap_transport = Strdup(xtree_wsb->attrval[atix]);

        }
        atix++;
    }

    if (!wstat && xtree_wsb->nsubel) {
            wstat = ws_set_error(wsg, WSE_NOCONTENTEXPECTED,
                    "%s <%s>", xtree_wsb->elname,
                    xtree_wsb->subel[0]->elname);
    }

    return (wstat);
}
/***************************************************************/
int parse_binding_content(struct wsglob * wsg,
                    struct xmltree * xtree_wsb,
                    const char * name_space,
                    struct wsbinding * wsb) {

    int wstat = 0;
    int trix;
    struct wsboperation * wsbo;
    char binding_name_space[XML_NAMESPACE_SZ];

    binding_name_space[0] = '\0';
    trix = 0;
    while (!wstat && trix < xtree_wsb->nsubel) {
        if (xml_item_name_equals(xtree_wsb->subel[trix]->elname, WSDL_BINDING)) {
            xml_get_name_space(xtree_wsb->subel[trix]->elname,
                binding_name_space, XML_NAMESPACE_SZ);
            wsb->wsb_binding_type = wsd_get_binding_type(wsb->wsb_wsdef_ref,
                                        binding_name_space);
            wstat = parse_binding_attrs_soap(wsg,
                        xtree_wsb->subel[trix],
                        binding_name_space,
                        wsb);

        } else if (xml_full_name_equals(xtree_wsb->subel[trix]->elname,
                    name_space, WSSH_OPERATION)) {
            if (wsb->wsb_binding_type == WSB_BINDING_TYPE_SOAP   ||
                wsb->wsb_binding_type == WSB_BINDING_TYPE_SOAP12) {
                wsbo = new_wsboperation(wsb, wsb->wsb_binding_type);
                wstat = parse_binding_operation_soap(wsg,
                            xtree_wsb->subel[trix],
                            name_space,
                            binding_name_space,
                            wsbo);
                if (!wstat) {
                    wstat = add_wsboperation_to_wsbinding(wsg, wsb, wsbo);
                }
            }

        } else {
            wstat = ws_set_error(wsg, WSE_BINDINGCONTENT,
                        "%s", xtree_wsb->subel[trix]->elname);
        }

        trix++;
    }

    return (wstat);
}
/***************************************************************/
int parse_wsdl_wsbinding(struct wsglob * wsg,
            struct xmltree * xtree_wsb,
            const char * name_space,
            struct wsbinding * wsb) {

/*
    <binding name="nmtoken"> *
        <part name="nmtoken" element="qname"? type="qname"?/> *
    </binding>
*/
    int wstat = 0;
    int atix;

    atix = 0;
    while (!wstat && atix < xtree_wsb->nattrs) {
        if (!strcmp(xtree_wsb->attrnam[atix], WSSH_TYP_NAME)) {
            wsb->wsb_name = Strdup(xtree_wsb->attrval[atix]);

        } else if (!strcmp(xtree_wsb->attrnam[atix], WSSH_TYP_TYPE)) {
            wsb->wsb_type = Strdup(xtree_wsb->attrval[atix]);

        } else {
            wstat = ws_set_error(wsg, WSE_BADBINDINGATTR,
                    "%s", xtree_wsb->attrnam[atix]);
        }
        atix++;
    }

    if (!wstat && xtree_wsb->nsubel) {
        wstat = parse_binding_content(wsg, xtree_wsb, name_space, wsb);
    }

    return (wstat);
}
/***************************************************************/
int add_wsport_to_wsservice(struct wsglob * wsg,
                            struct wsservice * wss,
                            struct wsport * wsp) {

/*
** Duplicates allowed, but not supported yet.
*/
    int wstat = 0;
    void ** vhand;
    int nlen;
    int is_dup;

    if (!wsp->wsp_name) {
        wstat = ws_set_error(wsg, WSE_PORTNAMEREQD, "");
        return (wstat);
    }

    nlen = strlen(wsp->wsp_name);
    if (!wss->wss_wsport_dbt) {
        wss->wss_wsport_dbt = dbtree_new();
        vhand = NULL;
    } else {
        vhand = dbtree_find(wss->wss_wsport_dbt,
                        wsp->wsp_name, nlen + 1);
    }

    if (!vhand) {
        is_dup = dbtree_insert(wss->wss_wsport_dbt,
                        wsp->wsp_name, nlen + 1, wsp);
    }

    return (wstat);
}
/***************************************************************/
int parse_port_content_attrs(struct wsglob * wsg,
                    struct xmltree * xtree_wsp,
                    const char * port_name_space,
                    struct wsport * wsp) {

    int wstat = 0;
    int atix;

    atix = 0;
    while (!wstat && atix < xtree_wsp->nattrs) {
        if (!strcmp(xtree_wsp->attrnam[atix], WSSH_TYP_LOCATION)) {
            wsp->wsp_location = Strdup(xtree_wsp->attrval[atix]);

        }
        atix++;
    }

    if (!wstat && xtree_wsp->nsubel) {
            wstat = ws_set_error(wsg, WSE_NOCONTENTEXPECTED,
                    "%s <%s>", xtree_wsp->elname,
                    xtree_wsp->subel[0]->elname);
    }

    return (wstat);
}
/***************************************************************/
int parse_port_content(struct wsglob * wsg,
                    struct xmltree * xtree_wsp,
                    const char * name_space,
                    struct wsport * wsp) {

    int wstat = 0;
    int trix;
    char port_name_space[XML_NAMESPACE_SZ];

    port_name_space[0] = '\0';
    trix = 0;
    while (!wstat && trix < xtree_wsp->nsubel) {
        if (xml_item_name_equals(xtree_wsp->subel[trix]->elname, WSDL_ADDRESS)) {
            xml_get_name_space(xtree_wsp->subel[trix]->elname,
                port_name_space, XML_NAMESPACE_SZ);

            wstat = parse_port_content_attrs(wsg,
                xtree_wsp->subel[trix], port_name_space, wsp);
            if (!wstat) {
                wsp->wsp_port_name_space = Strdup(port_name_space);
            }

        } else {
            wstat = ws_set_error(wsg, WSE_BINDINGCONTENT,
                        "%s", xtree_wsp->subel[trix]->elname);
        }

        trix++;
    }

    return (wstat);
}
/***************************************************************/
int parse_wsdl_wsport(struct wsglob * wsg,
            struct xmltree * xtree_wsp,
            const char * name_space,
            struct wsport * wsp) {

/*
    <port name="nmtoken"> *
        <part name="nmtoken" element="qname"? type="qname"?/> *
    </port>
*/
    int wstat = 0;
    int atix;

    atix = 0;
    while (!wstat && atix < xtree_wsp->nattrs) {
        if (!strcmp(xtree_wsp->attrnam[atix], WSSH_TYP_NAME)) {
            wsp->wsp_name = Strdup(xtree_wsp->attrval[atix]);

        } else if (!strcmp(xtree_wsp->attrnam[atix], WSSH_TYP_BINDING)) {
            wsp->wsp_binding = Strdup(xtree_wsp->attrval[atix]);

        } else {
            wstat = ws_set_error(wsg, WSE_BADPORTATTR,
                    "%s", xtree_wsp->attrnam[atix]);
        }
        atix++;
    }

    if (!wstat && xtree_wsp->nsubel) {
        wstat = parse_port_content(wsg, xtree_wsp, name_space, wsp);
    }

    return (wstat);
}
/***************************************************************/
int parse_service_content(struct wsglob * wsg,
                    struct xmltree * xtree_wss,
                    const char * name_space,
                    struct wsservice * wss) {

    int wstat = 0;
    int trix;
    struct wsport * wsp;

    trix = 0;
    while (!wstat && trix < xtree_wss->nsubel) {
        if (xml_full_name_equals(xtree_wss->subel[trix]->elname,
            name_space, WSDL_PORT)) {
            wsp = new_wsport(wss);
            wstat = parse_wsdl_wsport(wsg, xtree_wss->subel[trix],
                name_space, wsp);
            if (!wstat) {
                wstat = add_wsport_to_wsservice(wsg, wss, wsp);
            }

        } else if (xml_full_name_equals(xtree_wss->subel[trix]->elname,
            name_space, WSSH_DOCUMENTATION)) {
            if (xtree_wss->subel[trix]->eldata) {
                wss->wss_cdata = Strdup(xtree_wss->subel[trix]->eldata);
            }

        } else {
            wstat = ws_set_error(wsg, WSE_PORTCONTENT,
                        "%s", xtree_wss->subel[trix]->elname);
        }
        trix++;
    }

    return (wstat);
}
/***************************************************************/
int parse_wsdl_wsservice(struct wsglob * wsg,
            struct xmltree * xtree_wss,
            const char * name_space,
            struct wsservice * wss) {

/*
    <service name="nmtoken"> *
    </service>
*/
    int wstat = 0;
    int atix;

    atix = 0;
    while (!wstat && atix < xtree_wss->nattrs) {
        if (!strcmp(xtree_wss->attrnam[atix], WSSH_TYP_NAME)) {
            wss->wss_name = Strdup(xtree_wss->attrval[atix]);

        } else {
            wstat = ws_set_error(wsg, WSE_BADSERVICEATTR,
                    "%s", xtree_wss->attrnam[atix]);
        }
        atix++;
    }

    if (!wstat && xtree_wss->nsubel) {
        wstat = parse_service_content(wsg, xtree_wss, name_space, wss);
    }

    return (wstat);
}
/***************************************************************/
int parse_part_content(struct wsglob * wsg,
                    struct xmltree * xtree_wsmp,
                    const char * name_space,
                    struct wspart * wsmp) {

    int wstat = 0;
    int trix;

    trix = 0;
    while (!wstat && trix < xtree_wsmp->nsubel) {
            wstat = ws_set_error(wsg, WSE_PARTCONTENT,
                        "%s", xtree_wsmp->subel[trix]->elname);

        trix++;
    }

    return (wstat);
}
/***************************************************************/
int parse_wsdl_msg_part(struct wsglob * wsg,
                    struct xmltree * xtree_wsmp,
                    const char * name_space,
                    struct wspart * wsmp) {
/*
        <part name="nmtoken" element="qname"? type="qname"?/> *
*/

    int wstat = 0;
    int atix;

    atix = 0;
    while (!wstat && atix < xtree_wsmp->nattrs) {
        if (!strcmp(xtree_wsmp->attrnam[atix], WSSH_MSG_ELEMENT)) {
            wsmp->wsmp_element = Strdup(xtree_wsmp->attrval[atix]);

        } else if (!strcmp(xtree_wsmp->attrnam[atix], WSSH_MSG_NAME)) {
            wsmp->wsmp_name = Strdup(xtree_wsmp->attrval[atix]);

        } else if (!strcmp(xtree_wsmp->attrnam[atix], WSSH_MSG_TYPE)) {
            wsmp->wsmp_type = Strdup(xtree_wsmp->attrval[atix]);

        } else {
            wstat = ws_set_error(wsg, WSE_BADPARTATTR,
                    "%s", xtree_wsmp->attrnam[atix]);
        }
        atix++;
    }

    if (!wstat && xtree_wsmp->nsubel) {
        wstat = parse_part_content(wsg, xtree_wsmp, name_space, wsmp);
    }

    return (wstat);
}
/***************************************************************/
int parse_message_content(struct wsglob * wsg,
                    struct xmltree * xtree_wsm,
                    const char * name_space,
                    struct wsmessage * wsm) {

    int wstat = 0;
    int trix;
    struct wspart * wsmp;

    trix = 0;
    while (!wstat && trix < xtree_wsm->nsubel) {
        if (xml_full_name_equals(xtree_wsm->subel[trix]->elname,
            name_space, WSSH_PART)) {
            wsmp = add_new_wspart(wsm);
            wstat = parse_wsdl_msg_part(wsg, xtree_wsm->subel[trix],
                    name_space, wsmp);

        } else {
            wstat = ws_set_error(wsg, WSE_MESSAGECONTENT,
                        "%s", xtree_wsm->subel[trix]->elname);
        }
        trix++;
    }

    return (wstat);
}
/***************************************************************/
int parse_wsdl_wsmessage(struct wsglob * wsg,
            struct xmltree * xtree_wsm,
            const char * name_space,
            struct wsmessage * wsm) {

/*
    <message name="nmtoken"> *
        <part name="nmtoken" element="qname"? type="qname"?/> *
    </message>
*/
    int wstat = 0;
    int atix;

    atix = 0;
    while (!wstat && atix < xtree_wsm->nattrs) {
        if (!strcmp(xtree_wsm->attrnam[atix], WSSH_TYP_NAME)) {
            wsm->wsm_name = Strdup(xtree_wsm->attrval[atix]);

        } else {
            wstat = ws_set_error(wsg, WSE_BADMESSAGEATTR,
                    "%s", xtree_wsm->attrnam[atix]);
        }
        atix++;
    }

    if (!wstat && xtree_wsm->nsubel) {
        wstat = parse_message_content(wsg, xtree_wsm, name_space, wsm);
    }

    return (wstat);
}
/***************************************************************/
int add_wsmessage_to_wsdef(struct wsglob * wsg,
                            struct wsdef * wsd,
                            struct wsmessage * wsm) {

/*
** Not sure how to handle duplicates.
*/
    int wstat = 0;
    void ** vhand;
    int nlen;
    int is_dup;

    if (!wsm->wsm_name) {
        wstat = ws_set_error(wsg, WSE_MESSAGENAMEREQD, "");
        return (wstat);
    }
    wsm->wsm_qname = calc_qname(wsd->wsd_nslist,
        wsd->wsd_tns, wsm->wsm_name);

    nlen = strlen(wsm->wsm_qname);
    if (!wsd->wsd_wsmessage_dbt) {
        wsd->wsd_wsmessage_dbt = dbtree_new();
        vhand = NULL;
    } else {
        vhand = dbtree_find(wsd->wsd_wsmessage_dbt,
                        wsm->wsm_qname, nlen + 1);
    }

    if (!vhand) {
        is_dup = dbtree_insert(wsd->wsd_wsmessage_dbt,
                        wsm->wsm_qname, nlen + 1, wsm);
    }

    return (wstat);
}
/***************************************************************/
int add_wsporttype_to_wsdef(struct wsglob * wsg,
                            struct wsdef * wsd,
                            struct wsporttype * wsy) {

/*
** Not sure how to handle duplicates.
*/
    int wstat = 0;
    void ** vhand;
    int nlen;
    int is_dup;

    if (!wsy->wsy_name) {
        wstat = ws_set_error(wsg, WSE_PORTTYPENAMEREQD, "");
        return (wstat);
    }
    wsy->wsy_qname = calc_qname(wsd->wsd_nslist,
        wsd->wsd_tns, wsy->wsy_name);

    nlen = strlen(wsy->wsy_qname);
    if (!wsd->wsd_wsporttype_dbt) {
        wsd->wsd_wsporttype_dbt = dbtree_new();
        vhand = NULL;
    } else {
        vhand = dbtree_find(wsd->wsd_wsporttype_dbt,
                        wsy->wsy_qname, nlen + 1);
    }

    if (!vhand) {
        is_dup = dbtree_insert(wsd->wsd_wsporttype_dbt,
                        wsy->wsy_qname, nlen + 1, wsy);
    }

    return (wstat);
}
/***************************************************************/
#if 0
int add_wsport_to_wsdef(struct wsglob * wsg,
                            struct wsdef * wsd,
                            struct wsport * wsp) {

/*
** Not sure how to handle duplicates.
*/
    int wstat = 0;
    void ** vhand;
    int nlen;
    int is_dup;

    if (!wsp->wsp_name) {
        wstat = ws_set_error(wsg, WSE_MESSAGENAMEREQD, "");
        return (wstat);
    }

    nlen = strlen(wsp->wsp_name);
    if (!wsd->wsd_wsport_dbt) {
        wsd->wsd_wsport_dbt = dbtree_new();
        vhand = NULL;
    } else {
        vhand = dbtree_find(wsd->wsd_wsport_dbt,
                        wsp->wsp_name, nlen + 1);
    }

    if (!vhand) {
        is_dup = dbtree_insert(wsd->wsd_wsport_dbt,
                        wsp->wsp_name, nlen + 1, wsp);
    }

    return (wstat);
}
#endif
/***************************************************************/
int add_wsbinding_to_wsdef(struct wsglob * wsg,
                            struct wsdef * wsd,
                            struct wsbinding * wsb) {

/*
** Not sure how to handle duplicates.
*/
    int wstat = 0;
    void ** vhand;
    int nlen;
    int is_dup;

    if (!wsb->wsb_name) {
        wstat = ws_set_error(wsg, WSE_BINDINGNAMEREQD, "");
        return (wstat);
    }
    wsb->wsb_qname = calc_qname(wsd->wsd_nslist,
        wsd->wsd_tns, wsb->wsb_name);

    nlen = strlen(wsb->wsb_qname);
    if (!wsd->wsd_wsbinding_dbt) {
        wsd->wsd_wsbinding_dbt = dbtree_new();
        vhand = NULL;
    } else {
        vhand = dbtree_find(wsd->wsd_wsbinding_dbt,
                        wsb->wsb_qname, nlen + 1);
    }

    if (!vhand) {
        is_dup = dbtree_insert(wsd->wsd_wsbinding_dbt,
                        wsb->wsb_qname, nlen + 1, wsb);
    }

    return (wstat);
}
/***************************************************************/
int add_wsservice_to_wsdef(struct wsglob * wsg,
                            struct wsdef * wsd,
                            struct wsservice * wss) {

/*
** Not sure how to handle duplicates.
*/
    int wstat = 0;
    void ** vhand;
    int nlen;
    int is_dup;

    if (!wss->wss_name) {
        wstat = ws_set_error(wsg, WSE_MESSAGENAMEREQD, "");
        return (wstat);
    }

    nlen = strlen(wss->wss_name);
    if (!wsd->wsd_wsservice_dbt) {
        wsd->wsd_wsservice_dbt = dbtree_new();
        vhand = NULL;
    } else {
        vhand = dbtree_find(wsd->wsd_wsservice_dbt,
                        wss->wss_name, nlen + 1);
    }

    if (!vhand) {
        is_dup = dbtree_insert(wsd->wsd_wsservice_dbt,
                        wss->wss_name, nlen + 1, wss);
    }

    return (wstat);
}
/***************************************************************/
int parse_wsdl_definitions_content(struct wsglob * wsg,
            struct xmltree * xtree_wsd,
            const char * name_space,
            struct wsdef * wsd) {

    int wstat = 0;
    int trix;
    struct wsmessage * wsm;
    struct wsporttype * wsy;
    struct wsbinding * wsb;
    struct wsservice * wss;

    xml_get_name_space(xtree_wsd->elname, wsd->wsd_name_space, XML_NAMESPACE_SZ);

    trix = 0;

    while (!wstat && trix < xtree_wsd->nsubel) {
        if (xml_item_name_equals(xtree_wsd->subel[trix]->elname, WSDL_TYPES)) {
            if (wsd->wsd_wstypes) {
                wstat = ws_set_error(wsg, WSE_MULTIPLETYPES,
                            "%s", xtree_wsd->subel[trix]->elname);
            } else {
                wsd->wsd_wstypes = new_wstypes(wsd);
                wstat = parse_wsdl_wstypes(wsg, wsd,
                            xtree_wsd->subel[trix], wsd->wsd_wstypes);
            }

        } else if (xml_item_name_equals(xtree_wsd->subel[trix]->elname, WSDL_MESSAGE)) {
            wsm = new_wsmessage();
            wstat = parse_wsdl_wsmessage(wsg, xtree_wsd->subel[trix],
                    wsd->wsd_name_space, wsm);
            if (!wstat) {
                wstat = add_wsmessage_to_wsdef(wsg, wsd, wsm);
            }

        } else if (xml_item_name_equals(xtree_wsd->subel[trix]->elname, WSDL_PORT_TYPE)) {
            wsy = new_wsporttype();
            wstat = parse_wsdl_wsporttype(wsg, xtree_wsd->subel[trix],
                    wsd->wsd_name_space, wsy);
            if (!wstat) {
                wstat = add_wsporttype_to_wsdef(wsg, wsd, wsy);
            }

        } else if (xml_item_name_equals(xtree_wsd->subel[trix]->elname, WSDL_BINDING)) {
            wsb = new_wsbinding(wsd);
            wstat = parse_wsdl_wsbinding(wsg, xtree_wsd->subel[trix],
                    wsd->wsd_name_space, wsb);
            if (!wstat && wsb->wsb_binding_type) {
                wstat = add_wsbinding_to_wsdef(wsg, wsd, wsb);
            } else {
                free_wsbinding(wsb);
            }

        } else if (xml_item_name_equals(xtree_wsd->subel[trix]->elname, WSDL_SERVICE)) {
            wss = new_wsservice(wsd);
            wstat = parse_wsdl_wsservice(wsg, xtree_wsd->subel[trix],
                    wsd->wsd_name_space, wss);
            if (!wstat) {
                wstat = add_wsservice_to_wsdef(wsg, wsd, wss);
            }

        } else if (xml_item_name_equals(xtree_wsd->subel[trix]->elname, WSDL_DOCUMENTATION)) {

            /* documentation, ignore */

        } else if (!wstat && trix < xtree_wsd->nsubel &&
                   !xml_name_space_equals(xtree_wsd->subel[trix]->elname,
                                          name_space)) {

            /* different name space, ignore */

        } else {
            wstat = ws_set_error(wsg, WSE_DEFINITIONCONTENT,
                        "%s", xtree_wsd->subel[trix]->elname);
        }
        trix++;
    }

    return (wstat);
}
/***************************************************************/
int parse_wsdl_definitions(struct wsglob * wsg,
            struct xmltree * xtree_wsd,
            struct wsdef * wsd) {
/*
<definitions>

<types>
   data type definitions........
</types>

<message>
   definition of the data being communicated....
</message>

<portType>
   set of operations......
</portType>

<port>
   port ...
</port>

<binding>
   protocol and data format specification....
</binding>

<service>
   service specification....
</service>

</definitions>
*/
    int wstat = 0;
    int atix;
    char * tns;

    xml_get_name_space(xtree_wsd->elname, wsd->wsd_name_space, XML_NAMESPACE_SZ);

    atix = 0;
    while (!wstat && atix < xtree_wsd->nattrs) {
        if (!memcmp(xtree_wsd->attrnam[atix], "xmlns:", 6)) {
            nsl_insert_name_space(wsd->wsd_nslist,
                xtree_wsd->attrnam[atix] + 6, 
                xtree_wsd->attrval[atix]);

        } else if (!strcmp(xtree_wsd->attrnam[atix], WSSH_TYP_XMLNS)) {
            nsl_insert_name_space(wsd->wsd_nslist,
                "", 
                xtree_wsd->attrval[atix]);

        } else if (!strcmp(xtree_wsd->attrnam[atix], WSSH_TYP_NAME)) {
            wsd->wsd_name = Strdup(xtree_wsd->attrval[atix]);

        } else if (!strcmp(xtree_wsd->attrnam[atix],
                            WSSH_TYP_TARGETNAMESPACE)) {
            wsd->wsd_targetNamespace = Strdup(xtree_wsd->attrval[atix]);
        }

        atix++;
    }
#if TEST_NAME_SPACES
    nsl_test_name_spaces(wsd->wsd_nslist);
#endif

    if (!wstat && xtree_wsd->nsubel) {
        tns = NULL;
        if (wsd->wsd_targetNamespace) {
            tns = nsl_find_name_space_uri(wsd->wsd_nslist,
                                          wsd->wsd_targetNamespace, 1);
        }
        if (tns) wsd->wsd_tns = Strdup(tns);

        wstat = parse_wsdl_definitions_content(wsg, xtree_wsd,
            wsd->wsd_name_space, wsd);
    }

    return (wstat);
}
/***************************************************************/
/* End of WSDL parsing functions                               */
/***************************************************************/
int parse_wsdl(struct hgenv * hge,
            struct xmltree * xtree,
            struct wsglob * wsg) {
/*
** See : http://www.w3.org/TR/wsdl
** Also: http://www.w3schools.com/xml/xml_wsdl.asp
** Also: http://www.tutorialspoint.com/wsdl/wsdl_example.htm
*/
    int hgstat = 0;
    int wstat;
    const char * emsg;
    struct xmltree * xtree_wsd;

    xtree_wsd = parse_wsdl_find_definitions(xtree);
    if (!xtree_wsd) {
        hgstat = hg_set_error(hge, HGE_ENOTWSDL,
            "XML file %s is not an WSDL file.",
            hgGetWsdlFile(hge));
    } else {
        wsg->wsg_wsd = new_wsdef();
        wstat = parse_wsdl_definitions(wsg, xtree_wsd, wsg->wsg_wsd);
        if (wstat) {
            emsg = ws_get_error_msg(wsg, wstat);
            hgstat = hg_set_error(hge, HGE_EPARSE,
                "XML file %s had parsing error: %s",
                hgGetWsdlFile(hge), emsg);
            ws_clear_error(wsg, wstat);
        }
    }

    return (hgstat);
}
/***************************************************************/
int read_xml_wsdl_file(struct hgenv * hge,
        const char * wsdlfname,
        const char * cfgroot,
        struct wsglob * wsg) {

    int estat = 0;
    struct xmltree * xtree;
    char errmsg[GLB_ERRMSG_SIZE];

    estat = xmlp_get_xmltree_from_file(wsdlfname, &xtree, errmsg, sizeof(errmsg));
    if (estat) {
        estat = hg_set_error(hge, HGE_EREADCONFIG,
            "XML error reading WSDL file: %s", errmsg);
    } else if (!xtree) {
        estat = hg_set_error(hge, HGE_WDSLEMPTY,
            "XML file %s is empty.", wsdlfname);
    } else {
        hgSetWsdlFile(hge, wsdlfname);
        /* print_xmltree(xtree, 0); */

        estat = parse_wsdl(hge, xtree, wsg);
        free_xmltree(xtree);
    }

    return (estat);
}
/***************************************************************/
/* Begin WSDL processing functions                             */
/***************************************************************/
struct wsspinfo * new_wsspinfo(void) {

    struct wsspinfo * wsspi;

    wsspi = New(struct wsspinfo, 1);
    wsspi->wsspi_name       = NULL;
    wsspi->wsspi_element    = NULL;
    wsspi->wsspi_type       = NULL;

    return (wsspi);
}
/***************************************************************/
struct wssoperation * new_wssoperation(const char * opname,
    struct wsdef * wsd,
/*  struct wsschema * wsts, */
    struct wsservice * wss,
    struct wsport * wsp,
    struct wsbinding * wsb,
    struct wsboperation * wsbo,
    struct wspoperation * wsyo) {

    struct wssoperation * wssop;

    wssop = New(struct wssoperation, 1);
    wssop->wssop_opname         = Strdup(opname);

    wssop->wssop_wsd_ref        = wsd;
/*  wssop->wssop_wsts_ref       = wsts; */
    wssop->wssop_wss_ref        = wss;
    wssop->wssop_wsp_ref        = wsp;
    wssop->wssop_wsb_ref        = wsb;
    wssop->wssop_wsbo_ref       = wsbo;
    wssop->wssop_wsyo_ref       = wsyo;

    wssop->wssop_in_nwsspinfo   = 0;
    wssop->wssop_in_awsspinfo   = NULL;
    wssop->wssop_out_nwsspinfo  = 0;
    wssop->wssop_out_awsspinfo  = NULL;
    wssop->wssop_f_nwsspinfo    = 0;
    wssop->wssop_f_awsspinfo    = NULL;

    return (wssop);
}
/***************************************************************/
struct wssopinfo * new_wssopinfo(void) {

    struct wssopinfo * wssopi;

    wssopi = New(struct wssopinfo, 1);
    wssopi->wssopi_full_opname      = NULL;
    wssopi->wssopi_mwssoperation    = 0;
    wssopi->wssopi_nwssoperation    = 0;
    wssopi->wssopi_awssoperation    = NULL;

    return (wssopi);
}
/***************************************************************/
struct wssoplist * new_wssoplist(int binding_types) {

    struct wssoplist * wssopl;

    wssopl = New(struct wssoplist, 1);
    wssopl->wssopl_binding_types    = binding_types;
    wssopl->wssopl_wssopinfo_dbt    = NULL;

    return (wssopl);
}
/***************************************************************/
void free_wsspinfo(struct wsspinfo * wsspi) {

    Free(wsspi->wsspi_name);
    Free(wsspi->wsspi_element);
    Free(wsspi->wsspi_type);

    Free(wsspi);
}
/***************************************************************/
void free_wssoperation(struct wssoperation * wssop) {

    int ii;

    for (ii = 0; ii < wssop->wssop_in_nwsspinfo; ii++) {
        free_wsspinfo(wssop->wssop_in_awsspinfo[ii]);
    }
    Free(wssop->wssop_in_awsspinfo);

    for (ii = 0; ii < wssop->wssop_out_nwsspinfo; ii++) {
        free_wsspinfo(wssop->wssop_out_awsspinfo[ii]);
    }
    Free(wssop->wssop_out_awsspinfo);

    for (ii = 0; ii < wssop->wssop_f_nwsspinfo; ii++) {
        free_wsspinfo(wssop->wssop_f_awsspinfo[ii]);
    }
    Free(wssop->wssop_f_awsspinfo);

    Free(wssop->wssop_opname);

    Free(wssop);
}
/***************************************************************/
void free_wssopinfo(struct wssopinfo * wssopi) {

    int ii;

    for (ii = 0; ii < wssopi->wssopi_nwssoperation; ii++) {
        free_wssoperation(wssopi->wssopi_awssoperation[ii]);
    }
    Free(wssopi->wssopi_awssoperation);
    Free(wssopi->wssopi_full_opname);

    Free(wssopi);
}
/***************************************************************/
void freev_wssopinfo(void * vwssopi) {

    free_wssopinfo((struct wssopinfo *)vwssopi);
}
/***************************************************************/
void free_wssoplist(struct wssoplist * wssopl) {

    if (wssopl->wssopl_wssopinfo_dbt)
        dbtree_free(wssopl->wssopl_wssopinfo_dbt, freev_wssopinfo);

    Free(wssopl);
}
/***************************************************************/
void wssopl_add_operation(struct wssoplist * wssopl,
                struct wssoperation * wssop) {

    struct wssopinfo * wssopi;
    void ** vhand;
    int is_dup;
    char * full_opname;

    full_opname = Smprintf("%s.%s.%s",
            wssop->wssop_wss_ref->wss_name,
            wssop->wssop_wsp_ref->wsp_name,
            wssop->wssop_wsyo_ref->wsyo_name);

    wssopi = NULL;
    if (!wssopl->wssopl_wssopinfo_dbt) {
        wssopl->wssopl_wssopinfo_dbt = dbtree_new();
    } else {
        vhand = dbtree_find(wssopl->wssopl_wssopinfo_dbt,
                (char*)full_opname,
                IStrlen(full_opname) + 1);
        if (vhand) wssopi = *((struct wssopinfo **)vhand);
    }

    if (wssopi) {
        Free(full_opname);
    } else {
        wssopi = new_wssopinfo();
        wssopi->wssopi_full_opname = full_opname;
        is_dup = dbtree_insert(wssopl->wssopl_wssopinfo_dbt,
                    (char*)full_opname,
                    IStrlen(full_opname) + 1, wssopi);
    }

    if (wssopi->wssopi_nwssoperation >= wssopi->wssopi_mwssoperation) {
        if (!wssopi->wssopi_mwssoperation) wssopi->wssopi_mwssoperation = 8;
        else {
            wssopi->wssopi_mwssoperation *= 2;
        }
        wssopi->wssopi_awssoperation = 
            Realloc(wssopi->wssopi_awssoperation, char*, wssopi->wssopi_mwssoperation);
    }
    wssopi->wssopi_awssoperation[wssopi->wssopi_nwssoperation] = wssop;
    wssopi->wssopi_nwssoperation += 1;
}
/***************************************************************/
struct wsmessage * wsdl_find_wsmessage(struct wsdef * wsd,
                        const char * qname)
{
    void ** vhand;
    struct wsmessage * wsm;

    wsm = NULL;
    vhand = wsdl_find_qname(wsd->wsd_nslist,
               wsd->wsd_wsmessage_dbt, qname);
    if (vhand) wsm = *((struct wsmessage **)vhand);

    return (wsm);
}
/***************************************************************/
int wsdl_get_wsmessage_parm(struct wsglob * wsg,
    struct wsdef * wsd,
    struct wsmessage * wsm,
    struct wspart * wsmp,
    struct wsspinfo * wsspi)
{
    int wstat = 0;

    if (wsm->wsm_name) wsspi->wsspi_name = Strdup(wsm->wsm_name);

    if (wsmp->wsmp_element) wsspi->wsspi_element = Strdup(wsmp->wsmp_element);
    if (wsmp->wsmp_type)    wsspi->wsspi_type    = Strdup(wsmp->wsmp_type);

    return (wstat);
}
/***************************************************************/
void wssopl_add_wsspinfo(int * p_nwsspinfo,
                 struct wsspinfo *** p_awsspinfo,
                 struct wsspinfo * wsspi) {

    (*p_awsspinfo) =
        Realloc((*p_awsspinfo), struct wsspinfo*, (*p_nwsspinfo)+1);
    (*p_awsspinfo)[*p_nwsspinfo] = wsspi;
    (*p_nwsspinfo) += 1;
}
/***************************************************************/
int wsdl_get_wsboperation_parms(struct wsglob * wsg,
    struct wsdef * wsd,
    struct wsservice * wss,
    struct wsport * wsp,
    struct wsbinding * wsb,
    struct wsboperation * wsbo,
    struct wspoperation * wsyo,
    struct wssoperation * wssop)
{
    int wstat = 0;
    int ii;
    int jj;
    struct wspopinfo * wsyi;
    struct wsmessage * wsm;
    struct wsspinfo * wsspi;

    for (ii = 0; ii < wsyo->wsyo_nwspopinfo; ii++) {
        wsyi = wsyo->wsyo_awspopinfo[ii];
        if (wsyi->wsyi_message) {

            wsm = wsdl_find_wsmessage(wsd, wsyi->wsyi_message);

            if (wsm) {
                for (jj = 0; jj < wsm->wsm_nwspart; jj++) {
                    wsspi = new_wsspinfo();

                    wstat = wsdl_get_wsmessage_parm(wsg,
                        wsd, wsm, wsm->wsm_awspart[jj], wsspi);

                    switch (wsyi->wsyi_ptyp) {
                        case WSYO_PTYP_INPUT:
                            wssopl_add_wsspinfo(
                                &(wssop->wssop_in_nwsspinfo),
                                &(wssop->wssop_in_awsspinfo),
                                wsspi);
                            break;
                        case WSYO_PTYP_OUTPUT:
                            wssopl_add_wsspinfo(
                                &(wssop->wssop_out_nwsspinfo),
                                &(wssop->wssop_out_awsspinfo),
                                wsspi);
                            break;
                        case WSYO_PTYP_FAULT:
                            wssopl_add_wsspinfo(
                                &(wssop->wssop_f_nwsspinfo),
                                &(wssop->wssop_f_awsspinfo),
                                wsspi);
                            break;
                        default:
                            free_wsspinfo(wsspi);
                            break;
                    }
                }
            }
        }
    }

    return (wstat);
}
/***************************************************************/
int wsdl_get_wsboperation_wssoplist(struct wsglob * wsg,
    struct wsdef * wsd,
    struct wsservice * wss,
    struct wsport * wsp,
    struct wsbinding * wsb,
    struct wsboperation * wsbo,
    struct wspoperation * wsyo,
    struct wssoplist * wssopl)
{
    int wstat = 0;
    struct wssoperation * wssop;
/*  struct wsschema * wsts; */

/*  wsts = NULL; */
/*  if (wsd->wsd_wstypes->wst_nwsschema >= 1) { */
/*      wsts = wsd->wsd_wstypes->wst_awsschema[0]; */
/*  } */
/*   */
/*  if (wsts) {        */
/*      wssop = new_wssoperation(wsyo->wsyo_name, wsd, wsts, wss, wsp, wsb, wsbo, wsyo); */
        wssop = new_wssoperation(wsyo->wsyo_name, wsd, wss, wsp, wsb, wsbo, wsyo);

        wstat = wsdl_get_wsboperation_parms(wsg,
            wsd, wss, wsp, wsb, wsbo, wsyo, wssop);

        wssopl_add_operation(wssopl, wssop);
/*  } */

    return (wstat);
}
/***************************************************************/
int binding_operation_matches_porttype_operation(struct wsglob * wsg,
    struct wsdef * wsd,
    struct wsboperation * wsbo,
    struct wspoperation * wsyo)
{
    return (1);
}
/***************************************************************/
struct wsporttype * wsdl_find_wsporttype(struct wsdef * wsd,
                        const char * qname)
{

    void ** vhand;
    struct wsporttype * wsy;

    wsy = NULL;
    vhand = wsdl_find_qname(wsd->wsd_nslist,
               wsd->wsd_wsporttype_dbt, qname);
    if (vhand)  wsy = *((struct wsporttype **)vhand);

    return (wsy);
}
/***************************************************************/
int wsdl_get_wsbinding_wssoplist(struct wsglob * wsg,
    struct wsdef * wsd,
    struct wsservice * wss,
    struct wsport * wsp,
    struct wsbinding * wsb,
    struct wssoplist * wssopl)
{
    int wstat = 0;
    struct wsboplist * wsbl;
    void ** vhand;
    void ** vhand_wsboplist;
    int ii;
    int jj;
    struct wsboperation * wsbo;
    struct wsporttype * wsy;
    struct wsyoplist * wsyl;
    struct wspoperation * wsyo;
    void * vdbtc_wsboplist;

    vdbtc_wsboplist = dbtree_rewind(wsb->wsb_wsboplist_dbt, DBTREE_READ_FORWARD);
    do {
        vhand_wsboplist = dbtree_next(vdbtc_wsboplist);
        if (vhand_wsboplist) {
            wsbl = *((struct wsboplist **)vhand_wsboplist);

            wsy = wsdl_find_wsporttype(wsd, wsb->wsb_type);

            if (wsy) {
                for (ii = 0; ii < wsbl->wsbl_nwsboperation; ii++) {
                    wsbo = wsbl->wsbl_awsboperation[ii];

                    vhand = dbtree_find(wsy->wsy_wsyoplist_dbt,
                               wsbo->wsbo_name, IStrlen(wsbo->wsbo_name) + 1);
                    if (vhand) {
                        wsyl = *((struct wsyoplist **)vhand);

                        for (jj = 0; jj < wsyl->wsyl_nwspoperation; jj++) {
                            wsyo = wsyl->wsyl_awspoperation[jj];

                            if (binding_operation_matches_porttype_operation(wsg,
                                    wsd, wsbo, wsyo)) {
                                wstat = wsdl_get_wsboperation_wssoplist(wsg,
                                    wsd, wss, wsp, wsb, wsbo, wsyo, wssopl);
                            }
                        }
                    }
                }
            }
        }
    } while (vhand_wsboplist);
    dbtree_close(vdbtc_wsboplist);

    return (wstat);
}
/***************************************************************/
struct wsbinding * wsdl_find_wsbinding(struct wsdef * wsd,
                        const char * qname)
{

    void ** vhand;
    struct wsbinding * wsb;

    wsb = NULL;
    vhand = wsdl_find_qname(wsd->wsd_nslist,
                            wsd->wsd_wsbinding_dbt, qname);
    if (vhand) wsb = *((struct wsbinding **)vhand);

    return (wsb);
}
/***************************************************************/
int wsdl_get_wsport_wssoplist(struct wsglob * wsg,
    struct wsdef * wsd,
    struct wsservice * wss,
    struct wsport * wsp,
    struct wssoplist * wssopl,
    int binding_types)
{
    int wstat = 0;
    struct wsbinding * wsb;

    if (wsp->wsp_binding) {

        wsb = wsdl_find_wsbinding(wsd, wsp->wsp_binding);

        if (wsb) {
            if (wsb->wsb_type && (wsb->wsb_binding_type & binding_types)) {
                wstat = wsdl_get_wsbinding_wssoplist(wsg, wsd, wss, wsp, wsb,
                    wssopl);
            }
        }
    }

    return (wstat);
}
/***************************************************************/
int wsdl_get_wsdef_wssoplist(struct wsglob * wsg,
    struct wsdef * wsd,
    struct wssoplist * wssopl,
    int binding_types)
{
    int wstat = 0;
    void * vdbtc_wsdef;
    void ** vhand_wsdef;
    struct wsservice * wss;
    void * vdbtc_wsport;
    void ** vhand_wsport;
    struct wsport * wsp;

    vdbtc_wsdef = dbtree_rewind(wsd->wsd_wsservice_dbt, DBTREE_READ_FORWARD);
    do {
        vhand_wsdef = dbtree_next(vdbtc_wsdef);
        if (vhand_wsdef) {
            wss = *((struct wsservice **)vhand_wsdef);
            vdbtc_wsport = dbtree_rewind(wss->wss_wsport_dbt, DBTREE_READ_FORWARD);
            do {
                vhand_wsport = dbtree_next(vdbtc_wsport);
                if (vhand_wsport) {
                    wsp = *((struct wsport **)vhand_wsport);
                    wstat = wsdl_get_wsport_wssoplist(wsg, wsd, wss, wsp,
                        wssopl, binding_types);
                }
            } while (vhand_wsport);
            dbtree_close(vdbtc_wsport);
        }
    } while (vhand_wsdef);
    dbtree_close(vdbtc_wsdef);

    return (wstat);
}
/***************************************************************/
void append_chars(char ** tgtbuf,
                    int * tgtlen,
                    int * tgtmax,
                    char * srcbuf) {

    int srclen;

    srclen = IStrlen(srcbuf);

    if ((*tgtlen) + srclen >= (*tgtmax)) {
        (*tgtmax) = (*tgtlen) + srclen + 32;
        (*tgtbuf) = Realloc((*tgtbuf), char, (*tgtmax));
    }

    memcpy((*tgtbuf) + (*tgtlen), srcbuf, srclen + 1);
    (*tgtlen) += srclen;
}
/***************************************************************/
char * wsdl_get_parm_string(struct wsglob * wsg,
    struct wsdef * wsd,
    int nwsspinfo,
    struct wsspinfo ** awsspinfo) {

    char * pbuf;
    int    pbuflen;
    int    pbufmax;
    int ii;

    pbuf    = NULL;
    pbuflen = 0;
    pbufmax = 0;
    for (ii = 0; ii < nwsspinfo; ii++) {
        if (pbuf) {
            append_chars(&pbuf, &pbuflen, &pbufmax, ",");
        }
        if (awsspinfo[ii]->wsspi_name) {
            append_chars(&pbuf, &pbuflen, &pbufmax, awsspinfo[ii]->wsspi_name);
        }
    }

    if (!pbuf) pbuf = Strdup("");

    return (pbuf);
}
/***************************************************************/
char * wsdl_get_wssoperation_string(struct wsglob * wsg,
    struct wsdef * wsd,
    const char * full_opname,
    struct wssoperation * wssop) {

    char * opbuf;
    char * inpbuf;
    char * outpbuf;
    char * fpbuf;
    int    opbuflen;
    int    opbufmax;

    inpbuf = wsdl_get_parm_string(wsg, wsd,
                wssop->wssop_in_nwsspinfo, wssop->wssop_in_awsspinfo);

    outpbuf = wsdl_get_parm_string(wsg, wsd,
                wssop->wssop_out_nwsspinfo, wssop->wssop_out_awsspinfo);

    fpbuf = wsdl_get_parm_string(wsg, wsd,
                wssop->wssop_f_nwsspinfo, wssop->wssop_f_awsspinfo);

    opbuflen = IStrlen(full_opname);
    opbufmax = opbuflen + 32;
    opbuf    = New(char, opbufmax);
    memcpy(opbuf, full_opname, opbuflen);
    opbuf[opbuflen++] = '(';
    append_chars(&opbuf, &opbuflen, &opbufmax, inpbuf);
    opbuf[opbuflen++] = ')';

    if (outpbuf[0]) {
        append_chars(&opbuf, &opbuflen, &opbufmax, " -> ");
        append_chars(&opbuf, &opbuflen, &opbufmax, outpbuf);
    }

    if (fpbuf[0]) {
        append_chars(&opbuf, &opbuflen, &opbufmax, " FAULT ");
        append_chars(&opbuf, &opbuflen, &opbufmax, fpbuf);
    }

    Free(inpbuf);
    Free(outpbuf);
    Free(fpbuf);

    return (opbuf);
}
/***************************************************************/
char ** wsdl_copy_wssopl_to_service_list(struct wsglob * wsg,
    struct wsdef * wsd,
    struct wssoplist * wssopl) {

    int ii;
    char * opbuf;
    int nops;
    int mops;
    void * vdbtc;
    void ** vhand;
    struct wssopinfo * wssopi;
    char ** service_list;

    nops = 0;
    mops = 0;
    service_list = NULL;

    if (!wssopl->wssopl_wssopinfo_dbt) return NULL;

    vdbtc = dbtree_rewind(wssopl->wssopl_wssopinfo_dbt, DBTREE_READ_FORWARD);
    do {
        vhand = dbtree_next(vdbtc);
        if (vhand) {
            wssopi = *((struct wssopinfo **)vhand);

            for (ii = 0; ii < wssopi->wssopi_nwssoperation; ii++) {
                if (nops >= mops) {
                    if (mops) mops *= 2;
                    else      mops = 7;
                    service_list =
                        Realloc(service_list, char*, mops + 1);
                }
                opbuf = wsdl_get_wssoperation_string(wsg, wsd,
                            wssopi->wssopi_full_opname,
                            wssopi->wssopi_awssoperation[ii]);
                service_list[nops] = opbuf;
                nops++;
            }
        }
    } while (vhand);
    dbtree_close(vdbtc);

    if (nops) service_list[nops] = NULL;

    return (service_list);
}
/***************************************************************/
int wsdl_get_operations(struct wsglob * wsg, int binding_types)
{
    int wstat = 0;

    if (wsg->wsg_wssopl) {
        if (wsg->wsg_wssopl->wssopl_binding_types != binding_types) {
            free_wssoplist(wsg->wsg_wssopl);
            wsg->wsg_wssopl = NULL;
        }
    }

    if (!wsg->wsg_wssopl) {
        wsg->wsg_wssopl = new_wssoplist(binding_types);

        wstat = wsdl_get_wsdef_wssoplist(wsg, wsg->wsg_wsd,
            wsg->wsg_wssopl, binding_types);
    }

    return (wstat);
}
/***************************************************************/
char ** wsdl_get_service_list(struct wsglob * wsg)
{
    char ** service_list;

    /* copy wsg->wsg_wssopl to p_service_list */
    service_list = wsdl_copy_wssopl_to_service_list(wsg, wsg->wsg_wsd,
            wsg->wsg_wssopl);

    return (service_list);
}
/***************************************************************/
/* End WSDL processing functions                               */
/***************************************************************/
