#ifndef WHSTRH_INCLUDED
#define WHSTRH_INCLUDED
/***************************************************************/
/* json                                                        */
/***************************************************************/

#define VTAB_VAL_NONE           0
#define VTAB_VAL_OBJECT         1
#define VTAB_VAL_ARRAY          2
#define VTAB_VAL_STRING         3
#define VTAB_VAL_NUMBER         4
#define VTAB_VAL_TRUE           5
#define VTAB_VAL_FALSE          6
#define VTAB_VAL_NULL           7
#define VTAB_VAL_BOOLEAN        8
#define VTAB_VAL_EMPTY          9   /* 9/30/2016 */
#define VTAB_VAL_ERROR          (-1)

#define JSONE_FOPEN      (-2201)
#define JSONE_FREAD      (-2202)
#define JSONE_FCLOSE     (-2203)
#define JSONE_FNEW       (-2204)
#define JSONE_PROCESS    (-2205)

typedef wchar_t WJSCHAR;
typedef char JSCHAR;
#define JSONCMAXSZ           4 /* Maximum number of bytes in raw char */

#define JSON_PROLOG_SEARCH      1   /* if not json, search for '=' */
#define JSON_PROLOG_NONE_OK     2   /* no json is OK */

#define JSON_VALUE_TRUE         "true"
#define JSON_VALLEN_TRUE        4
#define JSON_VALUE_FALSE        "false"
#define JSON_VALLEN_FALSE       5
#define JSON_VALUE_NULL         "null"
#define JSON_VALLEN_NULL        4

#define JSONSTRCMP strcmp

/***************************************************************/

struct jsonarray {   /* ja */
    int     ja_nvals;
    struct jsonvalue ** ja_vals;
};

struct jsonobject {   /* jo */
    struct dbtree * jo_dbt; /* dbtree of struct jsonvalue * */
};

struct jsonstring {   /* jb */
    JSCHAR *     jb_buf;
    int          jb_bytes;
};
struct jsonnumber { /* jn */
    JSCHAR *     jn_buf;
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

#define PJT_FLAG_INDENT     1
#define PJT_FLAG_LF         2
#define PJT_FLAG_PRETTY     4

/***************************************************************/
int jsonp_get_jsontree(const char * jsonfname,
struct jsontree ** pjt,
    char * errmsg,
    int    errmsgsz);
struct jsontree * dup_jsontree(struct jsontree * jt);
int print_jsonvalue(FILE * outfile, struct jsonvalue * jv, int pjtflags, int jtdepth);
int print_jsontree(FILE * outfile, struct jsontree * jt, int pjtflags);

void free_jsonstring(struct jsonstring * jb);
void free_jsontree(struct jsontree * jt);

struct jsontree * jsonp_new_jsontree(void);
struct jsonvalue * jsonp_new_jsonvalue(int vtyp);
struct jsonobject * jsonp_new_jsonobject(void);
struct jsonarray * jsonp_new_jsonarray(void);

int jsonobject_decode_base64(struct jsonobject * jo, const char * b64field);

struct jsonarray * jsonobject_get_jsonarray(struct jsonobject * jo, const char * fldname);
struct jsonstring * jsonobject_get_jsonstring(struct jsonobject * jo, const char * fldname);
struct jsonobject * jsonobject_get_jsonobject(struct jsonobject * jo, const char * fldname);
int jsonobject_get_integer(struct jsonobject * jo, const char * fldname, int * num);
int jsonobject_get_double(struct jsonobject * jo, const char * fldname, double * dval);
char * jsonobject_get_string(struct jsonobject * jo, const char * fldname);

struct jsonstring * jsonarray_get_jsonstring(struct jsonarray * ja, int jaindex);
struct jsonnumber * jsonarray_get_jsonnumber(struct jsonarray * ja, int jaindex);
struct jsonobject * jsonarray_get_jsonobject(struct jsonarray * ja, int jaindex);
struct jsonarray * jsonarray_get_jsonarray(struct jsonarray * ja, int jaindex);
int jsonarray_get_integer(struct jsonarray * ja, int jaindex, int * num);
int jsonarray_get_double(struct jsonarray * ja, int jaindex, double * dval);
char * jsonarray_get_string(struct jsonarray * ja, int jaindex);

void jsonobject_put_int(struct jsonobject * jo, const char * fldnam, int ival);
void jsonobject_put_double(struct jsonobject * jo, const char * fldnam, double dval);
void jsonobject_put_string(struct jsonobject * jo, const char * fldnam, const char * strval);
void jsonobject_put_jv(struct jsonobject * jo, const char * fldnam, struct jsonvalue * jv);

void jsonarray_put_int(struct jsonarray * ja, int ival);
void jsonarray_put_double(struct jsonarray * ja, double dval);
void jsonarray_put_str(struct jsonarray * ja, const char * strval);
void jsonarray_put_jv(struct jsonarray * ja, struct jsonvalue * jv);

/***************************************************************/

/***************************************************************/
#endif
