#ifndef WDSLH_INCLUDED
#define WDSLH_INCLUDED
/***************************************************************/
/*
**  wsdl.h -- wsdl functions
*/
/***************************************************************/

#define ELSTRCMP strcmp

#define HGE_EHGEOPEN        (-6200)
#define HGE_EREADCONFIG     (-6201)
#define HGE_WDSLEMPTY       (-6202)
#define HGE_ENOTWSDL        (-6203)
#define HGE_EPARSE          (-6204)
#define HGE_EFOPEN          (-6205)
#define HGE_EREADIN         (-6206)
#define HGE_EBADOPT         (-6207)
#define HGE_EDUPOPEN        (-6208)
#define HGE_EXPEOL          (-6209)
#define HGE_BADCMD          (-6210)
#define HGE_NOWSDL          (-6211)
#define HGE_EWRITEOUT       (-6212)
#define HGE_EWSDLERR        (-6213)
#define HGE_EOPER           (-6214)
#define HGE_EPAREN          (-6215)

#define WSB_BINDING_TYPE_SOAP   1
#define WSB_BINDING_TYPE_SOAP12 2

#define WSDL_DOCUMENT       "document"
#define WSDL_DEFINITIONS    "definitions"

#define WSDL_SCHEMA         "schema"
#define WSDL_TYPES          "types"
#define WSDL_MESSAGE        "message"
#define WSDL_PORT           "port"
#define WSDL_PORT_TYPE      "portType"
#define WSDL_BINDING        "binding"
#define WSDL_ADDRESS        "address"
#define WSDL_INTERFACE      "interface"
#define WSDL_SERVICE        "service"
#define WSDL_DOCUMENTATION  "documentation"

#define WSSH_ALL                        "all"
#define WSSH_ANNOTATION                 "annotation"
#define WSSH_ANY                        "any"
#define WSSH_ANY_ATTRIBUTE              "anyAttribute"
#define WSSH_APPINFO                    "appinfo"
#define WSSH_ATTRIBUTE                  "attribute"
#define WSSH_BODY                       "body"
#define WSSH_CHOICE                     "choice"
#define WSSH_COMPLEX_CONTENT            "complexContent"
#define WSSH_COMPLEX_TYPE               "complexType"
#define WSSH_DOCUMENTATION              WSDL_DOCUMENTATION
#define WSSH_ELEMENT                    "element"
#define WSSH_ENUMERATION                "enumeration"
#define WSSH_EXTENSION                  "extension"
#define WSSH_FAULT                      "fault"
#define WSSH_GROUP                      "group"
#define WSSH_HEADER                     "header"
#define WSSH_IMPORT                     "import"
#define WSSH_INPUT                      "input"
#define WSSH_LIST                       "list"
#define WSSH_NAME                       "name"
#define WSSH_OPERATION                  "operation"
#define WSSH_OUTPUT                     "output"
#define WSSH_PART                       "part"
#define WSSH_RESTRICTION                "restriction"
#define WSSH_SEQUENCE                   "sequence"
#define WSSH_SIMPLE_CONTENT             "simpleContent"
#define WSSH_SIMPLE_TYPE                "simpleType"
#define WSSH_UNION                      "union"

#define WSSH_TYP_ABSTRACT               "abstract"
#define WSSH_TYP_ATTRIBUTEFORMDEFAULT   "attributeFormDefault"
#define WSSH_TYP_BASE                   "base"
#define WSSH_TYP_BINDING                "binding"
#define WSSH_TYP_BLOCK                  "block"
#define WSSH_TYP_BLOCKDEFAULT           "blockDefault"
#define WSSH_TYP_DEFAULT                "default"
#define WSSH_TYP_DEFAULTATTRIBUTES      "defaultAttributes"
#define WSSH_TYP_DEFAULTATTRIBUTESAPPLY "defaultAttributesApply"
#define WSSH_TYP_ELEMENTFORMDEFAULT     "elementFormDefault"
#define WSSH_TYP_FINAL                  "final"
#define WSSH_TYP_FINALDEFAULT           "finalDefault"
#define WSSH_TYP_FIXED                  "fixed"
#define WSSH_TYP_FORM                   "form"
#define WSSH_TYP_ID                     "id"
#define WSSH_TYP_ITEM_TYPE              "itemType"
#define WSSH_TYP_INHERITABLE            "inheritable"
#define WSSH_TYP_LOCATION               "location"
#define WSSH_TYP_MAXOCCURS              "maxOccurs"
#define WSSH_TYP_MEMBER_TYPES           "memberTypes"
#define WSSH_TYP_MESSAGE                "message"
#define WSSH_TYP_MINOCCURS              "minOccurs"
#define WSSH_TYP_MIXED                  "mixed"
#define WSSH_TYP_NAME                   "name"
#define WSSH_TYP_NAMESPACE              "namespace"
#define WSSH_TYP_NILLABLE               "nillable"
#define WSSH_TYP_NOTNAMESPACE           "notNamespace"
#define WSSH_TYP_NOTQNAME               "notQName"
#define WSSH_TYP_PART                   "part"
#define WSSH_TYP_PROCESSCONTENTS        "processContents"
#define WSSH_TYP_REF                    "ref"
#define WSSH_TYP_SCHEMALOCATION         "schemaLocation"
#define WSSH_TYP_SOAP_SOAP_ACTION       "soapAction"
#define WSSH_TYP_SOAP_STYPE             "style"
#define WSSH_TYP_SOURCE                 "source"
#define WSSH_TYP_SUBSTITUTIONGROUP      "substitutionGroup"
#define WSSH_TYP_TARGETNAMESPACE        "targetNamespace"
#define WSSH_TYP_TYPE                   "type"
#define WSSH_TYP_USE                    "use"
#define WSSH_TYP_VALUE                  "value"
#define WSSH_TYP_VERSION                "version"
#define WSSH_TYP_XML_LANG               "xml:lang"
#define WSSH_TYP_XMLNS                  "xmlns"
#define WSSH_TYP_XPATHDEFAULTNAMESPACE  "xpathDefaultNamespace"

#define WSSH_MSG_ELEMENT                "element"
#define WSSH_MSG_NAME                   "name"
#define WSSH_MSG_TYPE                   "type"

#define NSURI_XSD                   "http://www.w3.org/2001/XMLSchema"
#define NSURI_SOAP                  "http://schemas.xmlsoap.org/wsdl/soap/"
#define NSURI_SOAP                  "http://schemas.xmlsoap.org/wsdl/soap/"
#define NSURI_SOAP12                "http://schemas.xmlsoap.org/wsdl/soap12/"
#define NSURI_SOAP11_ENVELOPE       "http://schemas.xmlsoap.org/soap/envelope/"
/*#define NSURI_SOAP11_ENVELOPE     "http://www.w3.org/2001/12/soap-envelope"*/
#define NSURI_SOAP12_ENVELOPE       "http://www.w3.org/2003/05/soap-envelope"

#define SOAP_ENVELOPE               "Envelope"
#define SOAP_BODY                   "Body"
#define SOAP_NS                     "soap"
#define SOAP_CALL_NS                "scns"

#define SOAP_ENVFLAG_SOAP_NS        1
#define SOAP_ENVFLAG_SOAP12_NS      2

#define WSE_EXPSCHEMA               (-6401)
#define WSE_EXPELEMENT              (-6402)
#define WSE_EXPNAME                 (-6403)
#define WSE_DUPELEMENT              (-6404)
#define WSE_BADELEMENTATTR          (-6405)
#define WSE_ELEMENTCONTENT          (-6406)
#define WSE_BADCOMPLEXTYPEATTR      (-6407)
#define WSE_COMPLEXTYPECONTENT      (-6408)
#define WSE_BADSEQUENCEATTR         (-6409)
#define WSE_SEQUENCECONTENT         (-6410)
#define WSE_BADSIMPLETYPEATTR       (-6411)
#define WSE_SIMPLETYPECONTENT       (-6412)
#define WSE_BADENUMERATIONATTR      (-6413)
#define WSE_ENUMERATIONCONTENT      (-6414)
#define WSE_BADRESTRICTIONATTR      (-6415)
#define WSE_RESTRICTIONCONTENT      (-6416)
#define WSE_BADSCHEMAATTR           (-6417)
#define WSE_BADANYATTRIBUTEATTR     (-6418)
#define WSE_ANYATTRIBUTECONTENT     (-6419)
#define WSE_BADEXTENSIONATTR        (-6420)
#define WSE_EXTENSIONCONTENT        (-6421)
#define WSE_BADCOMPLEXCONTENTATTR   (-6422)
#define WSE_COMPLEXCONTENTCONTENT   (-6423)
#define WSE_BADSIMPLECONTENTATTR    (-6424)
#define WSE_SIMPLECONTENTCONTENT    (-6425)
#define WSE_BADATTRIBUTEATTR        (-6426)
#define WSE_ATTRIBUTECONTENT        (-6427)
#define WSE_DEFINITIONCONTENT       (-6428)
#define WSE_MULTIPLETYPES           (-6429)
#define WSE_MESSAGENAMEREQD         (-6430)
#define WSE_BADMESSAGEATTR          (-6431)
#define WSE_MESSAGECONTENT          (-6432)
#define WSE_BADPARTATTR             (-6433)
#define WSE_PARTCONTENT             (-6434)
#define WSE_PORTTYPENAMEREQD        (-6435)
#define WSE_OPERATIONNAMEREQD       (-6436)
#define WSE_BINDINGNAMEREQD         (-6437)
#define WSE_SERVICENAMEREQD         (-6438)
#define WSE_BADPORTTYPEATTR         (-6439)
#define WSE_PORTTYPECONTENT         (-6440)
#define WSE_BADPORTATTR             (-6441)
#define WSE_PORTCONTENT             (-6442)
#define WSE_BADBINDINGATTR          (-6443)
#define WSE_BINDINGCONTENT          (-6444)
#define WSE_BADSERVICEATTR          (-6445)
#define WSE_SERVICECONTENT          (-6446)
#define WSE_NOCONTENTEXPECTED       (-6447)
#define WSE_BADPORTTYPEELATTR       (-6448)
#define WSE_OPERATIONCONTENT        (-6449)
#define WSE_SOAPOPCONTENT           (-6450)
#define WSE_PORTNAMEREQD            (-6451)
#define WSE_LISTCONTENT             (-6452)
#define WSE_UNIONCONTENT            (-6453)
#define WSE_BADLISTATTR             (-6454)
#define WSE_BADUNIONATTR            (-6455)
#define WSE_SCHEMANAMESPACE         (-6456)
#define WSE_BADANNOTATIONATTR       (-6457)
#define WSE_ANNOTATIONCONTENT       (-6458)
#define WSE_BADAPPINFOATTR          (-6459)
#define WSE_APPINFOCONTENT          (-6460)
#define WSE_BADDOCUMENTATIONATTR    (-6461)
#define WSE_DOCUMENTATIONCONTENT    (-6462)
#define WSE_BADALLATTR              (-6463)
#define WSE_ALLCONTENT              (-6464)
#define WSE_BADCHOICEATTR           (-6465)
#define WSE_CHOICECONTENT           (-6466)
#define WSE_BADGROUPATTR            (-6467)
#define WSE_GROUPCONTENT            (-6468)
#define WSE_UNIMPLEMENTEDXLS        (-6469)
#define WSE_NOSUCHOPERATION         (-6470)
#define WSE_OPERATIONNOTUNIQUE      (-6471)
#define WSE_CALLPARAMETERERROR      (-6472)
#define WSE_UNSUPPORTEDBINDING      (-6473)
#define WSE_INSUFFICIENTPARMS       (-6474)
#define WSE_UNIMPLEMENTEDPARM       (-6475)
#define WSE_TOOMANYPARMS            (-6476)
#define WSE_FINDELEMENTTYPE         (-6477)
#define WSE_UNSUPPORTEDPARMTYPE     (-6478)
#define WSE_PARMCONVERSION          (-6479)
#define WSE_FINDNAMESPACE           (-6480)
#define WSE_XMLRESPONSE             (-6481)
#define WSE_EXPCLOSEPAREN           (-6482)
#define WSE_UNSUPPORTEDSEQYPE       (-6483)
#define WSE_NOSEQCONTENTNAME        (-6484)
#define WSE_FINDPARM                (-6485)

/***************************************************************/

int read_xml_wsdl_file(struct hgenv * hge,
        const char * wsdlfname,
        const char * cfgroot,
        struct wsglob * wsg);

struct wsglob * new_wsglob(void);
void free_wsglob(struct wsglob * wsg);
void ws_clear_all_errors(struct wsglob * wsg);
int ws_clear_error(struct wsglob * wsg, int eref);
const char * ws_get_error_msg(struct wsglob * wsg, int eref);
int wsdl_get_operations(struct wsglob * wsg, int binding_types);
char ** wsdl_get_service_list(struct wsglob * wsg);

void free_wssoplist(struct wssoplist * wssopl);
int ws_set_error(struct wsglob * wsg, int inwserr, char *fmt, ...);

struct wscontent * wsdl_find_wscontent(struct wstypes * wst,
                        const char * qname);

char * nsl_find_name_space(struct nslist * nsl,
                           const char * name_space,
                           int check_parent);
char * find_qname_uri(struct wstypes * wst,
            const char * qname);
char * xml_get_name(const char * element_name);
int get_num_wsschema(struct wstypes * wst);

/***************************************************************/
#endif /* WDSLH_INCLUDED */
/***************************************************************/
