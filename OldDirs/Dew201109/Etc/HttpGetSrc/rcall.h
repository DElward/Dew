#ifndef RCALLH_INCLUDED
#define RCALLH_INCLUDED
/***************************************************************/
/*
**  rcall.h -- wsdl functions
*/
/***************************************************************/

struct rcall { /* rc */
    int                 rc_binding_type;
    char *              rc_location;
    char *              rc_soap_action;
    char *              rc_host;
    char *              rc_response;
    int                 rc_resp_len;
    int                 rc_resp_max;
    struct xmltree *    rc_content_xt;
};
/***************************************************************/

struct rcall * new_rcall(int binding_type);
void free_rcall(struct rcall * rc);
void rcall_set_location(struct rcall * rc, const char * location);
void rcall_set_soap_action(struct rcall * rc, const char * soap_action);
void rcall_set_host(struct rcall * rc, const char * host);

int rcall_call(struct wsglob * wsg,
            struct dynl_info * dynl,
            struct rcall * rc,
            struct xmltree ** p_soap_result);

/***************************************************************/
#endif /* RCALLH_INCLUDED */
/***************************************************************/
