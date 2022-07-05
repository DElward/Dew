#ifndef XMLH_INCLUDED
#define XMLH_INCLUDED
/*
**  XMLH -- Extern declarations for XML parser
**
*/
/***************************************************************/
#if (SHRT_MAX == 32767)
typedef unsigned short int XMLC;
#define XMLCSZ              2 /* Number of bytes in a translated char */
#define XMLCMAXSZ           4 /* Maximum number of bytes in raw char */
#else
#error No int16 value
#endif

#define XMLCEOF             ((XMLC)EOF)
#define XMLCOVFL            254

typedef long (*XML_FREAD)   (void * vp, void * buffer, long buflen);
typedef void * (*XML_FOPEN) (void * vp, char * fname, char * fmode);
typedef int (*XML_FCLOSE)   (void * vp);
typedef int  (*XML_ELEMENT) (void * vp,
                             XMLC *  elNam,
                             int     emptyFlag,
                             int     nAttrs,
                             XMLC ** attrNames,
                             int   * attrLens,
                             XMLC ** attrVals);
typedef int  (*XML_CONTENT) (void * vp, int contLen, XMLC * contVal);
typedef int  (*XML_ENDTAG)  (void * vp, XMLC * elNam);

struct xmls_file * Xml_new(XML_FOPEN  p_xf_fopen,
                           XML_FREAD  p_xf_fread,
                           XML_FCLOSE p_xf_fclose,
                           void * vfp,
                           int vFlags);
int Xml_err(struct xmls_file * xf);
char * Xml_err_msg(struct xmls_file * xf);
void Xml_close(struct xmls_file * xf);
int Xml_process_xml(struct xmls_file * xf,
                            void * vp,
                            XML_ELEMENT p_xf_element,
                            XML_CONTENT p_xf_content,
                            XML_ENDTAG  p_xf_endtag);
int Xml_has_dtd(struct xmls_file * xf);
void Xml_imply_dtd(struct xmls_file * xf, long nitems);

/** schema functions **/
char ** Xmls_get_element(struct xmls_file * xf, char * el_name, int colattr);

/** XMLC functions **/
int xmlclen(XMLC * xtok);
int xmlccmp(XMLC * xtok1, XMLC * xtok2);
void xmlctostr(char * tgt, XMLC * src, int tgtlen);
void strtoxmlc(XMLC * tgt, char * src, int tgtlen);
void mvchartoxmlc(XMLC * tgt, char * src, int srclen);
void mvxmlctochar(char * tgt, XMLC * src, int srclen);

/** debug functions **/
void Xml_show_doctype(struct xmls_file * xf, FILE * fref);
void Xml_show_stats(struct xmls_file * xf, FILE * fref);

#define XMLC_BYTES(nc) ((nc) * XMLCSZ)

#define XMLRESULT_OK    0
#define XMLRESULT_EOF   (-1)
#define XMLRESULT_ERR   (-2)
#define XMLRESULT_INVAL (-3)

#define XMLVFLAG_VALIDATE   1
#define XMLVFLAG_MASK       (XMLVFLAG_VALIDATE)

#define XML_PCDATA_FIELD        "__DATA"
#define XML_EMPTY_FIELD         "__EMPTY"
/***************************************************************/
/* xmlp_ xml parsing functions                                 */

#define XMLE_FOPEN      1
#define XMLE_FNEW       2
#define XMLE_PROCESS    3

struct xmltree {
    char *  elname;
    int     nattrs;
    char ** attrnam;
    char ** attrval;
    char  * eldata;
    int     nsubel;
    struct xmltree ** subel;
    struct xmltree  * parent;
};
int xmlp_get_xmltree_from_file(const char * xmlfname,
                     struct xmltree ** xtree,
                     char * errmsg,
                     int    errmsgsz);
int xmlp_get_xmltree_from_string(const char * xml_string,
                     struct xmltree ** pxtree,
                     char * errmsg,
                     int    errmsgsz);
void print_xmltree(FILE * outf, struct xmltree * xtree, int depth);
void free_xmltree(struct xmltree * xtree);
int write_xmltree(char * fname, struct xmltree * xtree);
struct xmltree * xmlp_insert_subel(struct xmltree * xtree,
            char * subelName,
            char * subelVal,
            int    atFront);
void xmlp_delete_subel(struct xmltree * xtree, int subelIx);
void xmlp_replace_subel_value(struct xmltree * xtree,
            int subelIx,
            char * subelVal);
void xmlp_remove_subel(struct xmltree * xtree, int subelIx);
void xmlp_insert_attr(struct xmltree * xtree,
            const char * attrName,
            const char * attrVal,
            int    atFront);
struct xmltree * xmlp_new_element(char * elnam, struct xmltree * parent);
void xmlp_insert_subel_tree(struct xmltree * xtree,
            struct xmltree * subel,
            int    atFront);

#define XTTS_NL         1
#define XTTS_INDENT     2
#define XTTS_XML_HEADER 4

char * xmltree_to_string(struct xmltree * xtree, int xtts_flags);

/***************************************************************/
#endif /* XMLH_INCLUDED */
