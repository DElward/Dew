#ifndef JSENVH_INCLUDED
#define JSENVH_INCLUDED
/***************************************************************/
/* jsenv.h                                                     */
/***************************************************************/

#define JENFLAG_NODEJS   1

struct jsenv { /* jen_ */
	int                 jen_flags;
};

struct jsvalue { /* jv_ */
    int                 jv_typ;
};

struct jsvar { /* jsv_ */
    char              * jsv_name;
    struct jsvalue    * jsv_val;
};

struct jscontext { /* jcx_ */
    struct dbtree     * jcx_dbt;    /* tree of struct jsvar */
};
/***************************************************************/

struct jsenv * new_jsenv(int jenflags);
void free_jsenv(struct jsenv * jenv);

/***************************************************************/
#endif /* JSENVH_INCLUDED */
