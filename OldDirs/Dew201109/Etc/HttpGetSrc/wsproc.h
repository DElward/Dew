#ifndef WSPROCH_INCLUDED
#define WSPROCH_INCLUDED
/***************************************************************/
/*
**  wsproc.h -- wsdl functions
*/
/***************************************************************/

#define XML_NAMESPACE_SZ    32
typedef char tWCHAR;

/***************************************************************/
/* WSDL parsing structures                                     */
/***************************************************************/
/*
** See: http://www.w3.org/TR/xmlschema11-1/
*/
/*
** List of structures from:
**      http://www.w3schools.com/schema/schema_elements_ref.asp
**  
**
**  Element Explanation 
**
**  wstwa_
**  all             Specifies that the child elements can appear in any order.
**                  Each child element can occur 0 or 1 time 
**
**  wstwe_ todo
**  alternative     NOT DOCUMENTED in www.w3schools.com
**
**  wsta_
**  annotation      Specifies the top-level element for schema comments 
**
**  wsty_
**  anyAttribute    Enables the author to extend the XML document with
**                  attributes not specified by the schema
**
**  wsti_
**  appinfo         Specifies information to be used by the application
**                  (must go inside annotation) 
**
**  wstws_ todo
**  assert          NOT DOCUMENTED in www.w3schools.com
**
**  wstb_
**  attribute       Defines an attribute 
**
**  wstwg_ todo
**  attributeGroup  Defines an attribute group to be used in complex type
**                  definitions 
**
**  wstwc_
**  choice          Allows only one of the elements contained in the <choice>
**                  declaration to be present within the containing element 
**
**  wstk_
**  complexContent  Defines extensions or restrictions on a complex type that
**                  contains mixed content or elements only 
**
**  wstx_
**  complexType     Defines a complex type element 
**
**  wstd_
**  documentation   Defines text comments in a schema (must go inside
**                  annotation) 
**
**  wste_
**  element         Defines an element
**
**  wstu_
**  enumeration     NOT DOCUMENTED in www.w3schools.com
**
**  wstwx_ todo
**  example         NOT DOCUMENTED in www.w3schools.com
**
**  wstn_
**  extension       Extends an existing simpleType or complexType element 
**
**  wstwf_ todo
**  field           Specifies an XPath expression that specifies the value used
**                  to define an identity constraint 
**
**  wstg_ todo
**  group           Defines a group of elements to be used in complex type
**                  definitions 
**
**  wstwm_
**  import          Adds multiple schemas with different target namespace to a
**                  document 
**
**  wstwn_ todo
**  include         Adds multiple schemas with the same target namespace to a
**                  document 
**
**  wstwk_ todo
**  key             Specifies an attribute or element value as a key (unique,
**                  non-nullable, and always present) within the containing
**                  element in an instance document 
**
**  wstwr_ todo
**  keyref          Specifies that an attribute or element value correspond to
**                  those of the specified key or unique element 
**
**  wstl_
**  list            Defines a simple type element as a list of values 
**
**  wstwt_ todo
**  notation        Describes the format of non-XML data within an XML document 
**
**  wstwo_ todo
**  override        NOT DOCUMENTED in www.w3schools.com
**
**  wstwd_ todo
**  redefine        Redefines simple and complex types, groups, and attribute
**                  groups from an external schema 
**
**  wstr_
**  restriction     Defines restrictions on a simpleType, simpleContent, or a
**                  complexContent 
**
**  wsts_
**  schema          Defines the root element of a schema 
**
**  wstwl_ todo
**  selector        Specifies an XPath expression that selects a set of elements
**                  for an identity constraint 
**
**  wstq_
**  sequence        Specifies that the child elements must appear in a sequence.
**                  Each child element can occur from 0 to any number of times 
**
**  wstp_
**  simpleContent   Contains extensions or restrictions on a text-only complex
**                  type or on a simple type as content and contains no elements 
**
**  wsst_
**  simpleType      Defines a simple type and specifies the constraints and
**                  information about the values of attributes or text-only
**                  elements 
**
**  wstv_
**  union           Defines a simple type as a collection (union) of values from
**                  specified simple data types 
**
**  wstwu_ todo
**  unique          Defines that an element or an attribute value must be unique
**                  within the scope 
**
**  wstz_
**  any             Enables the author to extend the XML document with elements
**                  not specified by the schema 
*/
/***************************************************************/
/*
3.15.1 The Annotation Schema Component

<annotation
  id = ID
  {any attributes with non-schema namespace . . .}>
  Content: (appinfo | documentation)*
</annotation>
*/
struct wsannotation {   /* wsta_    annotation */
    char *                  wsta_id;
    int                     wsta_ncontent;
    struct wscontent **     wsta_acontent;
};
/***************************************************************/
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
struct wsattribute {   /* wstb_    attribute */
    char *                  wstb_default;
    char *                  wstb_fixed;
    char *                  wstb_form;
    char *                  wstb_id;
    char *                  wstb_name;
    char *                  wstb_ref;
    char *                  wstb_targetNamespace;
    char *                  wstb_type;
    char *                  wstb_use;
    char *                  wstb_inheritable;
    int                     wstb_ncontent;
    struct wscontent **     wstb_acontent;
};
/***************************************************************/
struct wscontent {   /* wstc_    generic content type */
    int                     wstc_content_from;
    int                     wstc_content_type;
    char *                  wstc_qname;
    union {
        void             *          wstcu_void;
        struct wselement *          wstcu_wselement;
        struct wscomplextype *      wstcu_wscomplextype;
        struct wssequence *         wstcu_wssequence;
        struct wsanyattribute *     wstcu_wsanyattribute;
        struct wsany *              wstcu_wsany;
        struct wssimpletype *       wstcu_wssimpletype;
        struct wsenumeration *      wstcu_wsenumeration;
        struct wsrestriction *      wstcu_wsrestriction;
        struct wscomplexcontent *   wstcu_wscomplexcontent;
        struct wsextension *        wstcu_wsextension;
        struct wsannotation *       wstcu_wsannotation;
        struct wssimplecontent *    wstcu_wssimplecontent;
        struct wsattribute *        wstcu_wsattribute;
        struct wslist *             wstcu_wslist;
        struct wsunion *            wstcu_wsunion;
        struct wsappinfo *          wstcu_wsappinfo;
        struct wsdocumentation *    wstcu_wsdocumentation;
        struct wsall *              wstcu_wsall;
        struct wschoice *           wstcu_wschoice;
        struct wsgroup *            wstcu_wsgroup;
    }                       wstc_itm;
};
/***************************************************************/
/*
<documentation
  source = anyURI
  xml:lang = language
  {any attributes with non-schema namespace . . .}>
  Content: ({any})*
</documentation>
*/
struct wsdocumentation {   /* wstd_    documentation */
    char *                  wstd_source;
    char *                  wstd_xml_lang;
    char *                  wstd_cdata;
};
/***************************************************************/
/*
3.3.2 XML Representation of Element Declaration Schema Components

<element
  abstract = boolean : false
  block = (#all | List of (extension | restriction | substitution)) 
  default = string
  final = (#all | List of (extension | restriction)) 
  fixed = string
  form = (qualified | unqualified)
  id = ID
  maxOccurs = (nonNegativeInteger | unbounded)  : 1
  minOccurs = nonNegativeInteger : 1
  name = NCName
  nillable = boolean : false
  ref = QName
  substitutionGroup = List of QName
  targetNamespace = anyURI
  type = QName
  {any attributes with non-schema namespace . . .}>
  Content: (annotation?, ((simpleType | complexType)?, alternative*,
                            (unique | key | keyref)*))
</element>
*/
struct wselement {   /* wste_    element */
    char *                  wste_abstract;
    char *                  wste_block;
    char *                  wste_default;
    char *                  wste_final;
    char *                  wste_fixed;
    char *                  wste_form;
    char *                  wste_id;
    char *                  wste_maxOccurs;
    char *                  wste_minOccurs;
    char *                  wste_name;
    char *                  wste_nillable;
    char *                  wste_ref;
    char *                  wste_substitutionGroup;
    char *                  wste_targetNamespace;
    char *                  wste_type;
    int                     wste_ncontent;
    struct wscontent **     wste_acontent;
};
/***************************************************************/
/*
<import
  id = ID
  namespace = anyURI
  schemaLocation = anyURI
  {any attributes with non-schema namespace . . .}>
  Content: (annotation?)
</import>
*/
struct wsimport {   /* wstf_    import */
    char *                  wstf_id;
    char *                  wstf_namespace;
    char *                  wstf_schemaLocation;
    int                     wstf_ncontent;
    struct wscontent **     wstf_acontent;
};
/***************************************************************/
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
struct wsgroup {   /* wstg_    group */
    char *                  wstg_id;
    char *                  wstg_maxOccurs;
    char *                  wstg_minOccurs;
    char *                  wstg_name;
    char *                  wstg_ref;
    int                     wstg_ncontent;
    struct wscontent **     wstg_acontent;
};
/***************************************************************/
/*
<appinfo
  source = anyURI
  {any attributes with non-schema namespace . . .}>
  Content: ({any})*
</appinfo>
*/
struct wsappinfo {   /* wsti_    appinfo */
    char *                  wsti_source;
    char *                  wsti_cdata;
};
/***************************************************************/
/*
<complexContent
  id = ID
  mixed = boolean
  {any attributes with non-schema namespace . . .}>
  Content: (annotation?, (restriction | extension))
</complexContent>
*/
struct wscomplexcontent {   /* wstk_    complex content */
    char *                  wstk_id;
    char *                  wstk_mixed;
    int                     wstk_ncontent;
    struct wscontent **     wstk_acontent;
};
/***************************************************************/
/*
<list
  id = ID
  itemType = QName
  {any attributes with non-schema namespace . . .}>
  Content: (annotation?, simpleType?)
</list>
*/
struct wslist {   /* wstl_    list */
    char *                  wstl_id;
    char *                  wstl_itemType;
    int                     wstl_ncontent;
    struct wscontent **     wstl_acontent;
};
/***************************************************************/
/*
<extension
  base = QName
  id = ID
  {any attributes with non-schema namespace . . .}>
  Content: (annotation?, openContent?,
      ((group | all | choice | sequence)?,
      ((attribute | attributeGroup)*, anyAttribute?), assert*))
</extension>
*/
struct wsextension {   /* wstn_    extension */
    char *                  wstn_base;
    char *                  wstn_id;
    int                     wstn_ncontent;
    struct wscontent **     wstn_acontent;
};
/***************************************************************/
/*
<simpleContent
  id = ID
  {any attributes with non-schema namespace . . .}>
  Content: (annotation?, (restriction | extension))
</simpleContent>
*/
struct wssimplecontent {   /* wstp_    simple content */
    char *                  wstp_id;
    int                     wstp_ncontent;
    struct wscontent **     wstp_acontent;
};
/***************************************************************/
/*
<sequence
  id = ID
  maxOccurs = (nonNegativeInteger | unbounded)  : 1
  minOccurs = nonNegativeInteger : 1
  {any attributes with non-schema namespace . . .}>
  Content: (annotation?, (element | group | choice | sequence | any)*)
</sequence>
*/
struct wssequence {   /* wstq_    sequence */
    char *                  wstq_id;
    char *                  wstq_maxOccurs;
    char *                  wstq_minOccurs;
    int                     wstq_ncontent;
    struct wscontent **     wstq_acontent;
};
/***************************************************************/
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
struct wsrestriction {   /* wstr_    restriction */
    char *                  wstr_base;
    char *                  wstr_id;
    int                     wstr_ncontent;
    struct wscontent **     wstr_acontent;
};
/***************************************************************/
struct wsschematype {   /* wsst_    schema type */
    char *                  wsst_sch_itm_name;
    struct wscontent *      wsst_sch_content;
};
/***************************************************************/
/*
<schema
  attributeFormDefault = (qualified | unqualified) : unqualified
  blockDefault = (#all | List of (extension | restriction | substitution))  : ''
  defaultAttributes = QName
  xpathDefaultNamespace = (anyURI |
            (##defaultNamespace | ##targetNamespace | ##local))  : ##local
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
struct wsschema {   /* wsts_    schema */
    struct wstypes *        wsts_wstypes_ref; /* parent */
    char *                  wsts_attributeFormDefault;
    char *                  wsts_blockDefault;
    char *                  wsts_defaultAttributes;
    char *                  wsts_xpathDefaultNamespace;
    char *                  wsts_elementFormDefault;
    char *                  wsts_finalDefault;
    char *                  wsts_id;
    char *                  wsts_targetNamespace;
    char *                  wsts_version;
    char *                  wsts_xml_lang;
    char *                  wsts_tns;
    struct nslist *         wsts_nslist;

    char                    wsts_name_space[XML_NAMESPACE_SZ];
    struct dbtree *         wsts_schematype_dbt; /* dbtree of wsschematype * */
};
/***************************************************************/
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
struct wssimpletype {   /* wstt_    simple type */
    char *                  wstt_final;
    char *                  wstt_id;
    char *                  wstt_name;
    int                     wstt_ncontent;
    struct wscontent **     wstt_acontent;
};
/***************************************************************/
/*
4.3.5.2 XML Representation of enumeration Schema Components

<enumeration
  id = ID
  value = anySimpleType
  {any attributes with non-schema namespace . . .}>
  Content: (annotation?)
</enumeration>
*/
struct wsenumeration {   /* wstu_    enumeration */
    char *                  wstu_id;
    char *                  wstu_value;
};
/***************************************************************/
/*
<union
  id = ID
  memberTypes = List of QName
  {any attributes with non-schema namespace . . .}>
  Content: (annotation?, simpleType*)
</union>
*/
struct wsunion {   /* wstv_    union */
    char *                  wstv_id;
    char *                  wstv_memberTypes;
    int                     wstv_ncontent;
    struct wscontent **     wstv_acontent;
};
/***************************************************************/
/*
<all
  id = ID
  maxOccurs = (0 | 1) : 1
  minOccurs = (0 | 1) : 1
  {any attributes with non-schema namespace . . .}>
  Content: (annotation?, (element | any | group)*)
</all>
*/
struct wsall {   /* wstwa_    all */
    char *                  wstwa_id;
    char *                  wstwa_maxOccurs;
    char *                  wstwa_minOccurs;
    int                     wstwa_ncontent;
    struct wscontent **     wstwa_acontent;
};
/***************************************************************/
/*
<choice
  id = ID
  maxOccurs = (nonNegativeInteger | unbounded)  : 1
  minOccurs = nonNegativeInteger : 1
  {any attributes with non-schema namespace . . .}>
  Content: (annotation?, (element | group | choice | sequence | any)*)
</choice>
*/
struct wschoice {   /* wstwc_    choice */
    char *                  wstwc_id;
    char *                  wstwc_maxOccurs;
    char *                  wstwc_minOccurs;
    int                     wstwc_ncontent;
    struct wscontent **     wstwc_acontent;
};
/***************************************************************/
/*
<redefine
  id = ID
  schemaLocation = anyURI
  {any attributes with non-schema namespace . . .}>
  Content: (annotation | (simpleType | complexType | group | attributeGroup))*
</redefine>
*/
struct wsredefine {   /* wstwd_    redefine */
    int                     wstwd_00_toto;
    int                     wstwd_ncontent;
    struct wscontent **     wstwd_acontent;
};
/***************************************************************/
/*
<alternative
  id = ID
  test = an XPath expression
  type = QName
  xpathDefaultNamespace = (anyURI | (##defaultNamespace | ##targetNamespace | ##local)) 
  {any attributes with non-schema namespace . . .}>
  Content: (annotation?, (simpleType | complexType)?)
</alternative>
*/
struct wsalternative {   /* wstwe_    alternative */
    int                     wstwe_ncontent;
    struct wscontent **     wstwe_acontent;
};
/***************************************************************/
/*
<field
  id = ID
  xpath = a subset of XPath expression, see below
  xpathDefaultNamespace = (anyURI | (##defaultNamespace | ##targetNamespace | ##local)) 
  {any attributes with non-schema namespace . . .}>
  Content: (annotation?)
</field>
*/
struct wsfield {   /* wstwf_    field */
    int                     wstwf_00_toto;
    int                     wstwf_ncontent;
    struct wscontent **     wstwf_acontent;
};
/***************************************************************/
/*
<attributeGroup
  id = ID
  ref = QName
  {any attributes with non-schema namespace . . .}>
  Content: (annotation?)
</attributeGroup>
*/
struct wsattributegroup {   /* wstwg_    attributegroup */
    int                     wstwg_ncontent;
    struct wscontent **     wstwg_acontent;
};
/***************************************************************/
/*
<key
  id = ID
  name = NCName
  ref = QName
  {any attributes with non-schema namespace . . .}>
  Content: (annotation?, (selector, field+)?)
</key>
*/
struct wskey {   /* wstwk_    key */
    int                     wstwk_00_toto;
    int                     wstwk_ncontent;
    struct wscontent **     wstwk_acontent;
};
/***************************************************************/
/*
<selector
  id = ID
  xpath = a subset of XPath expression, see below
  xpathDefaultNamespace = (anyURI | (##defaultNamespace | ##targetNamespace | ##local)) 
  {any attributes with non-schema namespace . . .}>
  Content: (annotation?)
</selector>
*/
struct wsselector {   /* wstwl_    selector */
    int                     wstwl_00_toto;
    int                     wstwl_ncontent;
    struct wscontent **     wstwl_acontent;
};
/***************************************************************/
/*
<include
  id = ID
  schemaLocation = anyURI
  {any attributes with non-schema namespace . . .}>
  Content: (annotation?)
</include>
*/
struct wsinclude {   /* wstwn_    include */
    int wstwn_00_toto;
};
/***************************************************************/
/*
<override
  id = ID
  schemaLocation = anyURI
  {any attributes with non-schema namespace . . .}>
  Content: (annotation | (simpleType | complexType | group | attributeGroup |
             element | attribute | notation))*
</override>
*/
struct wsoverride {   /* wstwo_    override */
    int                     wstwo_00_toto;
    int                     wstwo_ncontent;
    struct wscontent **     wstwo_acontent;
};
/***************************************************************/
/*
<keyref
  id = ID
  name = NCName
  ref = QName
  refer = QName
  {any attributes with non-schema namespace . . .}>
  Content: (annotation?, (selector, field+)?)
</keyref>
*/
struct wskeyref {   /* wstwr_    keyref */
    int                     wstwr_00_toto;
    int                     wstwr_ncontent;
    struct wscontent **     wstwr_acontent;
};
/***************************************************************/
/*
<assert
  id = ID
  test = an XPath expression
  xpathDefaultNamespace = (anyURI | (##defaultNamespace | ##targetNamespace | ##local)) 
  {any attributes with non-schema namespace . . .}>
  Content: (annotation?)
</assert>
*/
struct wsassert {   /* wstws_    assert */
    int                     wstws_ncontent;
    struct wscontent **     wstws_acontent;
};
/***************************************************************/
/*
<notation
  id = ID
  name = NCName
  public = token
  system = anyURI
  {any attributes with non-schema namespace . . .}>
  Content: (annotation?)
</notation>
*/
struct wsnotation {   /* wstwt_    notation */
    int                     wstwt_00_toto;
    int                     wstwt_ncontent;
    struct wscontent **     wstwt_acontent;
};
/***************************************************************/
/*
<unique
  id = ID
  name = NCName
  ref = QName
  {any attributes with non-schema namespace . . .}>
  Content: (annotation?, (selector, field+)?)
</unique>
*/
struct wsunique {   /* wstwu_    unique */
    int                     wstwu_00_toto;
    int                     wstwu_ncontent;
    struct wscontent **     wstwu_acontent;
};
/***************************************************************/
/*
<example
  count = integer
  size = (large | medium | small) : medium>
  Content: (all | any*)
</example>
*/
struct wsexample {   /* wstwx_    example */
    int                     wstwx_ncontent;
    struct wscontent **     wstwx_acontent;
};
/***************************************************************/
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
  Content: (annotation?, (simpleContent | complexContent |
        (openContent?, (group | all | choice | sequence)?,
        ((attribute | attributeGroup)*, anyAttribute?), assert*)))
</complexType>
*/
struct wscomplextype {   /* wstx_    complex type */
    char *                  wstx_abstract;
    char *                  wstx_block;
    char *                  wstx_final;
    char *                  wstx_id;
    char *                  wstx_mixed;
    char *                  wstx_name;
    char *                  wstx_defaultAttributesApply;
    int                     wstx_ncontent;
    struct wscontent **     wstx_acontent;
};
/***************************************************************/
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
struct wsanyattribute {   /* wsty_    anyAttribute */
    char *                  wsty_id;
    char *                  wsty_namespace;
    char *                  wsty_notNamespace;
    char *                  wsty_notQName;
    char *                  wsty_processContents;
    int                     wsty_ncontent;
    struct wscontent **     wsty_acontent;
};
/***************************************************************/
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
struct wsany {   /* wstz_    any */
    char *                  wstz_id;
    char *                  wstz_maxOccurs;
    char *                  wstz_minOccurs;
    char *                  wstz_namespace;
    char *                  wstz_notNamespace;
    char *                  wstz_notQName;
    char *                  wstz_processContents;
    int                     wstz_ncontent;
    struct wscontent **     wstz_acontent;
};
/***************************************************************/
/* End of XML schema structs                                   */
/***************************************************************/
struct wstypes {     /* wst_     type */
    struct wsdef *          wst_wsdef_ref; /* parent */
    int                     wst_num_wsschema;
/*
    int                     wst_nwsschema;
    struct wsschema **      wst_awsschema;
*/
    struct dbtree *         wst_wsschema_dbt;  /* dbtree of wsschema * */
};
struct wspart {  /* wsmp_     part */
    char *                  wsmp_name;
    char *                  wsmp_element;
    char *                  wsmp_type;
};
struct wsmessage {  /* wsm_     message */
    char *                  wsm_name;
    char *                  wsm_qname;
    int                     wsm_nwspart;
    struct wspart **        wsm_awspart;
};
#define WSYO_PTYP_INPUT         1
#define WSYO_PTYP_OUTPUT        2
#define WSYO_PTYP_FAULT         3
#define WSYO_PTYP_DOCUMENTATION 4

#define WSYO_BTYP_INPUT         WSYO_PTYP_INPUT
#define WSYO_BTYP_OUTPUT        WSYO_PTYP_OUTPUT
#define WSYO_BTYP_FAULT         WSYO_PTYP_FAULT
#define WSYO_BTYP_DOCUMENTATION WSYO_PTYP_DOCUMENTATION

#define WSBISO_SOAP_TYP_BODY    10
#define WSBISO_SOAP_TYP_HEADER  11
#define WSBISO_SOAP_TYP_FAULT   12

struct wspopinfo {  /* wsyi_     port type operation information*/
    int                     wsyi_ptyp;
    char *                  wsyi_name;
    char *                  wsyi_message;
    char *                  wsyi_cdata;
};
struct wspoperation {  /* wsyo_     port type operation */
    char *                  wsyo_name;
    int                     wsyo_nwspopinfo;
    struct wspopinfo **     wsyo_awspopinfo;
};
struct wsyoplist {  /* wsyl_     binding operation list */
    int                     wsyl_nwspoperation;
    struct wspoperation **  wsyl_awspoperation;
};
struct wsporttype { /* wsy_     port type*/
    char *                  wsy_name;
    char *                  wsy_qname;
    struct dbtree *         wsy_wsyoplist_dbt;  /* dbtree of wsyoplist * */
};
struct wsinterface {    /* wsi_ interface -- replaces portType operation */
                        /* and  message in WSDL 2.0 */
    int wsi_00_toto;
};
struct wssoapop {  /* wsbiso_     binding soap operation information*/
    int      wsbiso_optyp;
    char *   wsbiso_soap_body_use;       /* WSBISO_SOAP_TYP_BODY   */
    char *   wsbiso_soap_header_message; /* WSBISO_SOAP_TYP_HEADER */
    char *   wsbiso_soap_header_part;    /* WSBISO_SOAP_TYP_HEADER */
    char *   wsbiso_soap_header_use;     /* WSBISO_SOAP_TYP_HEADER */
    char *   wsbiso_soap_fault_name;     /* WSBISO_SOAP_TYP_FAULT  */
    char *   wsbiso_soap_fault_use;      /* WSBISO_SOAP_TYP_FAULT  */
};
/*
   <soap:body parts="nmtokens"? use="literal|encoded"
              encodingStyle="uri-list"? namespace="uri"?>
   <soap:header message="qname" part="nmtoken" use="literal|encoded"
                encodingStyle="uri-list"? namespace="uri"?>*
     <soap:headerfault message="qname" part="nmtoken" use="literal|encoded"
                       encodingStyle="uri-list"? namespace="uri"?/>*
   <soap:header>         
*/
struct wsbopinfo {  /* wsbi_     binding operation information*/
    int                     wsbi_btyp;
    int                     wsbi_nwssoapop;
    struct wssoapop **       wsbi_awssoapop;
};
/*
        <operation .... >
           <soap:operation soapAction="uri"? style="rpc|document"?>?
        </operation>
*/
struct wsboperation {  /* wsbo_     binding operation */
    char *                  wsbo_name;
    struct wsbinding *      wsbo_wsbinding_ref;
    int                     wsbo_binding_type; /* soap 1.1 or soap 1.2 */
    char *                  wsbo_soap_soapAction;
    char *                  wsbo_soap_style;
    int                     wsbo_nwsbopinfo;
    struct wsbopinfo **     wsbo_awsbopinfo;
};
struct wsboplist {  /* wsbl_     binding operation list */
    int                     wsbl_nwsboperation;
    struct wsboperation **  wsbl_awsboperation;
};
/*
    <binding .... >
        <soap:binding transport="uri"? style="rpc|document"?>
    </binding>
*/
struct wsbinding {  /* wsbl_     binding */
    char *                  wsb_name;
    char *                  wsb_qname;
    struct wsdef *          wsb_wsdef_ref;  /* parent */
    char *                  wsb_type;
    int                     wsb_binding_type;
    char *                  wsb_soap_transport;
    struct dbtree *         wsb_wsboplist_dbt;  /* dbtree of wsboplist * */
};
struct wsport {     /* wsp_     port */
    struct wsservice *      wsp_wsservice_ref;  /* parent */
    char *                  wsp_name;
    char *                  wsp_binding;
    char *                  wsp_port_name_space;
    char *                  wsp_location;
};
struct wsservice {  /* wss_     service */
    char *                  wss_name;
    char *                  wss_cdata;
    struct wsdef *          wss_wsdef_ref;  /* parent */
    struct dbtree *         wss_wsport_dbt;  /* dbtree of wsport * */
};
struct wsdef {      /* wsd_     definition */
    struct wstypes *        wsd_wstypes;
    struct dbtree *         wsd_wsmessage_dbt;  /* dbtree of wsd_wsmessage * */
    struct dbtree *         wsd_wsporttype_dbt; /* dbtree of wsporttype * */
 /* struct dbtree *         wsd_wsinterface_dbt; * dbtree of wsinterface * */
    struct dbtree *         wsd_wsbinding_dbt;  /* dbtree of wsbinding * */
    struct dbtree *         wsd_wsservice_dbt;  /* dbtree of wsservice * */

    char *                  wsd_name;
    char *                  wsd_targetNamespace;
    char *                  wsd_tns;
    struct nslist *         wsd_nslist;
    char                    wsd_name_space[XML_NAMESPACE_SZ];
};
struct wserr {  /* wsx_ */
    int     wsx_errnum;
    char *  wsx_errmsg;
};
#define WS_ERR_IX_DELTA 3000
struct wsglob {      /* wsg_     global */
    int                     wsg_nerrs;
    struct wserr **         wsg_errs;
    struct wsdef *          wsg_wsd;
    struct wssoplist *      wsg_wssopl;
};
/***************************************************************/
struct wsspinfo { /* wsspi */
    char *              wsspi_name;
    char *              wsspi_element;
    char *              wsspi_type;
};
struct wssoperation { /* wssop */
    char *                  wssop_opname;
    struct wsdef *          wssop_wsd_ref;
/*  struct wsschema *       wssop_wsts_ref; */
    struct wsservice *      wssop_wss_ref;
    struct wsport *         wssop_wsp_ref;
    struct wsbinding *      wssop_wsb_ref;
    struct wsboperation *   wssop_wsbo_ref;
    struct wspoperation *   wssop_wsyo_ref;

    int                     wssop_in_nwsspinfo;
    struct wsspinfo **      wssop_in_awsspinfo;
    int                     wssop_out_nwsspinfo;
    struct wsspinfo **      wssop_out_awsspinfo;
    int                     wssop_f_nwsspinfo;
    struct wsspinfo **      wssop_f_awsspinfo;
};
struct wssopinfo { /* wssopi_ */
    char *                  wssopi_full_opname;
    struct wssoperation **  wssopi_awssoperation;
    int                     wssopi_nwssoperation;
    int                     wssopi_mwssoperation;
};
struct wssoplist { /* wssopl_ */
    int                     wssopl_binding_types;
    struct dbtree *         wssopl_wssopinfo_dbt; /* dbtree of wssopinfo * */
};
/***************************************************************/

#define WSST_ALL                1
#define WSST_ANNOTATION         (WSST_ALL                + 1)   /*  2 */
#define WSST_ANY                (WSST_ANNOTATION         + 1)   /*  3 */
#define WSST_ANY_ATTRIBUTE      (WSST_ANY                + 1)   /*  4 */
#define WSST_APPINFO            (WSST_ANY_ATTRIBUTE      + 1)   /*  5 */
#define WSST_ATTRIBUTE          (WSST_APPINFO            + 1)   /*  6 */
#define WSST_CHOICE             (WSST_ATTRIBUTE          + 1)   /*  7 */
#define WSST_COMPLEX_CONTENT    (WSST_CHOICE             + 1)   /*  8 */
#define WSST_COMPLEX_TYPE       (WSST_COMPLEX_CONTENT    + 1)   /*  9 */
#define WSST_DOCUMENTATION      (WSST_COMPLEX_TYPE       + 1)   /* 10 */
#define WSST_ELEMENT            (WSST_DOCUMENTATION      + 1)   /* 11 */
#define WSST_ENUMERATION        (WSST_ELEMENT            + 1)   /* 12 */
#define WSST_EXTENSION          (WSST_ENUMERATION        + 1)   /* 13 */
#define WSST_GROUP              (WSST_EXTENSION          + 1)   /* 14 */
#define WSST_LIST               (WSST_GROUP              + 1)   /* 15 */
#define WSST_RESTRICTION        (WSST_LIST               + 1)   /* 16 */
#define WSST_SCHEMA             (WSST_RESTRICTION        + 1)   /* 17 */
#define WSST_SEQUENCE           (WSST_SCHEMA             + 1)   /* 18 */
#define WSST_SIMPLE_CONTENT     (WSST_SEQUENCE           + 1)   /* 19 */
#define WSST_SIMPLE_TYPE        (WSST_SIMPLE_CONTENT     + 1)   /* 20 */
#define WSST_UNION              (WSST_SIMPLE_TYPE        + 1)   /* 21 */

/***************************************************************/

struct jsonarray {   /* ja */
    int     ja_nvals;
    struct jsonvalue ** ja_vals;
};

struct jsonobject {   /* jo */
    struct dbtree * jo_dbt; /* dbtree of struct jsonvalue * */
};

struct jsonstring {   /* jb */
    tWCHAR *     jb_buf;
    int          jb_bytes;
};
struct jsonnumber { /* jn */
    tWCHAR *     jn_buf;
    int jn_bytes;
};

struct jsonvalue {   /* jv */
    int     jv_vtyp;
    union {
        struct jsonobject * jv_jo;
        struct jsonarray  * jv_ja;
        struct jsonstring  * jv_jb;
        struct jsonnumber  * jv_jn;
        void  * jv_vp;
    } jv_val;
};

struct jsontree {   /* jt */
    struct jsonvalue  * jt_jv;
};

#define VTAB_VAL_NONE           0
#define VTAB_VAL_OBJECT         1
#define VTAB_VAL_ARRAY          2
#define VTAB_VAL_STRING         3
#define VTAB_VAL_NUMBER         4
#define VTAB_VAL_TRUE           5
#define VTAB_VAL_FALSE          6
#define VTAB_VAL_NULL           7
#define VTAB_VAL_BOOLEAN        8
#define VTAB_VAL_ERROR          (-1)

#define JSON_PROLOG_SEARCH      1   /* if not json, search for '=' */
#define JSON_PROLOG_NONE_OK     2   /* no json is OK */

/***************************************************************/

int wsdl_call_operation(struct wsglob * wsg,
            struct dynl_info * dynl,
            const char * full_opname,
            const char * line,
            int * linix,
            FILE * outfile);
/***************************************************************/
#endif /* WSPROCH_INCLUDED */
/***************************************************************/
