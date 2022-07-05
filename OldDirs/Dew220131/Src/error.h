#ifndef ERRORH_INCLUDED
#define ERRORH_INCLUDED
/***************************************************************/
/* error.h                                                     */
/***************************************************************/

#define JERR_EOF                                (-1)
#define JERR_EOF_ERROR                          (-2)
#define JERR_STOPPED                            (-3)
#define JERR_EXIT                               (-4)
#define JERR_RETURN                             (-5)

#define JSERR_UNMATCHED_QUOTE                   (-9801)
#define JSERR_INVALID_ESCAPE                    (-9802)
#define JSERR_INVALID_ID                        (-9803)
#define JSERR_ID_RESERVED                       (-9804)
#define JSERR_ID_INVALID_ESCAPE                 (-9805)
#define JSERR_UNSUPPORTED_STMT                  (-9806)
#define JSERR_OPERATOR_TYPE_MISMATCH            (-9807)
#define JSERR_INVALID_TOKEN_TYPE                (-9808)
#define JSERR_INVALID_NUMBER                    (-9809)
#define JSERR_UNEXPECTED_SPECIAL                (-9810)
#define JSERR_EXP_CLOSE_PAREN                   (-9811)
#define JSERR_UNEXPECTED_KEYWORD                (-9812)
#define JSERR_UNDEFINED_VARIABLE                (-9813)
#define JSERR_EXP_INTERNAL_CLASS                (-9814)
#define JSERR_DUP_INTERNAL_METHOD               (-9815)
#define JSERR_DUP_INTERNAL_CLASS                (-9816)
#define JSERR_DUPLICATE_VARIABLE                (-9817)
#define JSERR_EXP_ID_AFTER_DOT                  (-9818)
#define JSERR_EXP_OBJECT_AFTER_DOT              (-9819)
#define JSERR_DUP_CLASS_MEMBER                  (-9820)
#define JSERR_UNDEFINED_CLASS_MEMBER            (-9821)
#define JSERR_EXP_METHOD                        (-9822)
#define JSERR_INTERNAL_ERROR                    (-9823)
#define JSERR_ILLEGAL_EXPONENTIATION            (-9824)
#define JSERR_UNSUPPORTED_OPERATION             (-9825)
#define JSERR_INVALID_VARNAME                   (-9826)
#define JSERR_INVALID_FUNCTION                  (-9827)
#define JSERR_INVALID_STORE                     (-9828)
#define JSERR_UNSUPPORTED_ASSIGN                (-9829)
#define JSERR_MUST_BE_BLOCK                     (-9830)
#define JSERR_EXPECT_EQUALS                     (-9831)
#define JSERR_UNEXPECTED_PUNCTUATOR             (-9832)
#define JSERR_EXP_CLOSE_BRACKET                 (-9833)
#define JSERR_INVALID_TYPE                      (-9834)
#define JSERR_CONVERT_TO_INT                    (-9835)
#define JSERR_NULL_OBJECT                       (-9836)
#define JSERR_EXP_CLOSE_BRACE                   (-9837)
#define JSERR_OBJ_TOKEN_TYPE                    (-9838)
#define JSERR_OBJ_EXP_COLON                     (-9839)
#define JSERR_INVALID_COMPARE                   (-9840)
#define JSERR_PROP_READ_ONLY                    (-9841)
#define JSERR_INVALID_ARRAY_SIZE                (-9842)
#define JSERR_EXP_OPEN_PAREN                    (-9843)
#define JSERR_MUST_BE_IN_TRY                    (-9844)
#define JSERR_THROW_STATEMENT                   (-9845)
#define JSERR_EXP_SEMICOLON                     (-9846)
#define JSERR_UNRECOGNIZED_STMT                 (-9847)
#define JSERR_MUST_BE_IN_BLOCK                  (-9848)

#define JSERR_RUNTIME_STACK_OVERFLOW            (-7801)
#define JSERR_RUNTIME_STACK_UNDERFLOW           (-7802)

/* JavaScript errors from: */
/* https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Error#Error_types */
#define JSERROBJ_SCRIPT_SYNTAX_ERROR            (-9901)    /* SyntaxError */
#define JSERROBJ_SCRIPT_REFERENCE_ERROR         (-9902)    /* ReferenceError */

#define JSERRMSG_SIZE       128

enum errTypes { /* errtyp_ */
    errtyp_Error         ,
    errtyp_EvalError     ,
    errtyp_InternalError ,  /* dew specific */
    errtyp_RangeError    ,
    errtyp_ReferenceError,
    errtyp_SyntaxError   ,
    errtyp_TypeError     ,
    errtyp_URIError      ,
    errtyp_NativeError   ,
    errtyp_UnimplementedError   ,  /* dew specific */
};

/***************************************************************/
/* int set_jserror(enum errTypes errtyp, int estat, const char * fmt, ...); */

int jrun_set_error(struct jrunexec * jx, enum errTypes errtyp, int jstat, char *fmt, ...);
void jrun_clear_all_errors(struct jrunexec * jx);
char * jrun_get_errmsg(struct jrunexec * jx,
    int emflags,
    int jstat,
    int *jerrnum);
int jrun_clear_error(struct jrunexec * jx, int injstat);
int jrun_convert_error_to_jvv_error(struct jrunexec * jx,
    int injstat,
    struct jvarvalue * ejvv);
int jint_Error_toString(
        struct jrunexec  * jx,
        const char       * func_name,
        void             * this_obj,
        struct jvarvalue * jvvlargs,
        struct jvarvalue * jvvrtn);

/***************************************************************/
#endif /* ERRORH_INCLUDED */
