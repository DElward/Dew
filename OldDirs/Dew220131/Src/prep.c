/***************************************************************/
/*
**  prep.c -- Preprocessing functions
*/
/***************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdarg.h>

#include "dew.h"
#include "jsenv.h"
#include "error.h"
#include "jstok.h"
#include "dbtreeh.h"
#include "snprfh.h"
#include "prep.h"
#include "var.h"
#include "jsexp.h"

static int jprep_eval(
                    struct jrunexec * jx,
                    struct jexp_exp_rec * jxe,
                    struct jtoken ** pjtok,
                    int skip_flags);
static int jprep_eval_assignment(
                    struct jrunexec * jx,
                    struct jexp_exp_rec * jxe,
                    struct jtoken ** pjtok,
                    int skip_flags);
static int jprep_eval_list(
                    struct jrunexec * jx,
                    struct jexp_exp_rec * jxe,
                    struct jtoken ** pjtok,
                    int skip_flags);
int jrun_exec_generic_inactive_stmt(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok);
int jprep_preprocess_block(
                    struct jrunexec * jx,
                    struct jvarvalue_function * jvvf,
                    struct jtoken ** pjtok,
                    int skip_flags);

/***************************************************************/
static int jprep_if_stmt(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok,
                    int skip_flags)
{
/*
** 02/26/2021
*/
    int jstat = 0;

    if ((*pjtok)->jtok_kw != JSPU_OPEN_PAREN) {
        jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERROBJ_SCRIPT_SYNTAX_ERROR,
            "Expecting open parenthesis. Found \'%s\'", (*pjtok)->jtok_text);
    } else {
        jstat = jrun_next_token(jx, pjtok);
        if (!jstat) {
            jstat = jexp_evaluate_inactive(jx, pjtok, 0);
            if (!jstat) {
                if ((*pjtok)->jtok_kw != JSPU_CLOSE_PAREN) {
                    jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERROBJ_SCRIPT_SYNTAX_ERROR,
                        "Expecting close parenthesis. Found \'%s\'", (*pjtok)->jtok_text);
                } else {
                    jstat = jrun_next_token(jx, pjtok);
                }
            }
        }
    }

    return (jstat);
}
/***************************************************************/
static int jprep_while_stmt(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok,
                    int skip_flags)
{
/*
** 01/27/2022
*/
    int jstat = 0;

    if ((*pjtok)->jtok_kw != JSPU_OPEN_PAREN) {
        jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERROBJ_SCRIPT_SYNTAX_ERROR,
            "Expecting open parenthesis after while. Found \'%s\'", (*pjtok)->jtok_text);
    } else {
        jstat = jrun_next_token(jx, pjtok);
        if (!jstat) {
            jstat = jexp_evaluate_inactive(jx, pjtok, 0);
            if (!jstat) {
                if ((*pjtok)->jtok_kw != JSPU_CLOSE_PAREN) {
                    jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERROBJ_SCRIPT_SYNTAX_ERROR,
                        "Expecting close parenthesis after while. Found \'%s\'", (*pjtok)->jtok_text);
                } else {
                    jstat = jrun_next_token(jx, pjtok);
                }
            }
        }
    }

    return (jstat);
}
/***************************************************************/
static int jprep_function_stmt(
                    struct jrunexec * jx,
                    struct jtokeninfo * jti_func,
                    struct jtokeninfo * jti_brace,
                    struct jtoken ** pjtok,
                    int skip_flags)
{
/*
** 03/01/2021
*/
    int jstat = 0;
    struct jvarvalue fjvv;
    int jcxflags;

    jvar_init_jvarvalue(&fjvv);
    jstat = jvar_parse_function(jx, pjtok, &fjvv, skip_flags);
    if (!jstat) {
        jcxflags = JCX_FLAG_FUNCTION;
        jstat = jvar_new_local_function(jx, jti_brace, &fjvv, jcxflags, skip_flags);
    }
    if (jstat) jvar_free_jvarvalue_data(&fjvv);

    return (jstat);
}
/***************************************************************/
int jprep_parse_object_inactive(
                    struct jrunexec * jx,
                    struct jexp_exp_rec * jxe,
                    struct jtoken ** pjtok,
                    int skip_flags)
{
/*
** 12/17/2021
**
** See also: jexp_parse_object_inactive()
*/
    int jstat = 0;

    if ((*pjtok)->jtok_kw == JSPU_CLOSE_BRACE ) {
        jstat = jrun_next_token(jx, pjtok);
        return (jstat);
    }

    return (jstat);
}
/***************************************************************/
int jprep_parse_array_inactive(
                    struct jrunexec * jx,
                    struct jexp_exp_rec * jxe,
                    struct jtoken ** pjtok,
                    int skip_flags)
{
/*
** 12/17/2021
**
** See also: jexp_parse_array_inactive()
*/
    int jstat;

    if ((*pjtok)->jtok_kw == JSPU_CLOSE_BRACKET ) {
        jstat = jrun_next_token(jx, pjtok);
        return (jstat);
    }

    jstat = jprep_eval_assignment(jx, jxe, pjtok, skip_flags);
    while (!jstat &&
           (*pjtok)->jtok_kw == JSPU_COMMA ) {
        jstat = jrun_next_token(jx, pjtok);
        if (!jstat) {
            jstat = jprep_eval_assignment(jx, jxe, pjtok, skip_flags);
        }
    }

    if (!jstat && (*pjtok)->jtok_kw == JSPU_CLOSE_BRACKET ) {
        jstat = jrun_next_token(jx, pjtok);
    }

    return (jstat);
}
/***************************************************************/
static int jprep_eval_function_call(
                    struct jrunexec * jx,
                    struct jexp_exp_rec * jxe,
                    struct jtoken ** pjtok,
                    int skip_flags)
{
/*
** 12/17/2021
**
** See also: jexp_eval_function_call_inactive()
*/
    int jstat;

    jstat = jrun_next_token(jx, pjtok); /* ( */
    if (!jstat) {
        jstat = jrun_next_token(jx, pjtok); /* next token */
    }
    if (!jstat && (*pjtok)->jtok_kw != JSPU_CLOSE_PAREN) {
        jstat = jprep_eval_list(jx, jxe, pjtok, skip_flags);
    }
    if (!jstat) {
        if ((*pjtok)->jtok_kw != JSPU_CLOSE_PAREN ) {
            jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_EXP_CLOSE_PAREN,
                "Expecting close parenthesis after function call. Found: %s", (*pjtok)->jtok_text);
        } else {
            jstat = jrun_next_token(jx, pjtok); /* next token */
        }
    }

    return (jstat);
}
/***************************************************************/
#ifdef OLD_WAY
int jprep_function_expression(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok,
                    int skip_flags)
{
/*
** 01/27/2022
**
**  On Entry: (*pjtok)->jtok_text = "("
*/
    int jstat;

    jstat = jrun_next_token(jx, pjtok);
    if (!jstat) {
        jstat = jvar_parse_function_arguments_inactive(jx, pjtok);
    }
    if (!jstat) {
        jstat = jrun_exec_inactive_block(jx, pjtok); /* in: "{"	out:  */
    }

    return (jstat);
}
/***************************************************************/
int jprep_function_primary(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok,
                    int skip_flags)
{
/*
** 12/30/2021
*/
    int jstat = 0;

    if ((*pjtok)->jtok_kw == JSPU_STAR) {
        jstat = jrun_next_token(jx, pjtok);
        if (jstat) return (jstat);
    }

    if ((*pjtok)->jtok_typ == JSW_ID || (*pjtok)->jtok_typ == JSW_KW) {
        jstat = jrun_next_token(jx, pjtok);
        if (jstat) return (jstat);
    }

    jstat = jprep_function_expression(jx, pjtok, skip_flags);

    return (jstat);
}
#endif
int jprep_function_primary(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok,
                    int skip_flags)
{
/*
** 01/27/2022
*/
    int jstat = 0;
    struct jvarvalue fjvv;

    jvar_init_jvarvalue(&fjvv);
    jstat = jvar_parse_function(jx, pjtok, &fjvv, skip_flags);

    jvar_free_jvarvalue_data(&fjvv);

    return (jstat);
}
/***************************************************************/
static int jprep_eval_primary(
                    struct jrunexec * jx,
                    struct jexp_exp_rec * jxe,
                    struct jtoken ** pjtok,
                    int skip_flags)
{
/*
** 12/17/2021
**
** See also: jexp_eval_primary_inactive()
*/
    int jstat = 0;
    struct jtoken next_jtok;

    switch ((*pjtok)->jtok_typ) {
        case JSW_STRING:
            jstat = jrun_next_token(jx, pjtok);
            break;

        case JSW_NUMBER:
            jstat = jrun_next_token(jx, pjtok);
            break;

        case JSW_KW:
            if ((*pjtok)->jtok_kw == JSKW_FUNCTION) {
                jstat = jrun_next_token(jx, pjtok);
                if (!jstat) jstat = jprep_function_primary(jx, pjtok, skip_flags);
            } else {
                jstat = jrun_next_token(jx, pjtok);
            }
            break;

        case JSW_ID:
            jrun_peek_token(jx, &next_jtok);
            if (next_jtok.jtok_kw == JSPU_OPEN_PAREN) {
                jstat = jprep_eval_function_call(jx, jxe, pjtok, skip_flags);
            } else {
                jstat = jrun_next_token(jx, pjtok);
            }
            break;

        case JSW_PU:
            switch ((*pjtok)->jtok_kw) {
                case JSPU_OPEN_PAREN:
                    jstat = jrun_next_token(jx, pjtok);
                    break;

                case JSPU_OPEN_BRACKET:
                    jstat = jrun_next_token(jx, pjtok);
                    if (!jstat) jstat = jprep_parse_array_inactive(jx, jxe, pjtok, skip_flags);
                    break;

                case JSPU_OPEN_BRACE:
                    jstat = jrun_next_token(jx, pjtok);
                    if (!jstat) jstat = jprep_parse_object_inactive(jx, jxe, pjtok, skip_flags);
                    break;

                default:
                    jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_UNEXPECTED_PUNCTUATOR,
                        "Unexpected punctuator during prepare: %s", (*pjtok)->jtok_text);
                    break;
            }
            break;

        default:
            jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_INVALID_TOKEN_TYPE,
                "Invalid token type %d: %s", (*pjtok)->jtok_typ, (*pjtok)->jtok_text);
            break;
    }

    return (jstat);
}
/***************************************************************/
static int jprep_eval(
                    struct jrunexec * jx,
                    struct jexp_exp_rec * jxe,
                    struct jtoken ** pjtok,
                    int skip_flags)
{
/*
** 12/17/2021
*/
    int jstat;
    int parens;
    int done;
    int xlx;

    jstat       = 0;
    parens      = 0;
    done        = 0;

    while (!jstat && !done) {
        xlx = GET_XLX(*pjtok);
        while (!jstat && xlx >= 0 && (jxe[xlx].jxe_opflags & JXE_OPFLAG_UNARY_OPERATION)) {
            jstat = jrun_next_token(jx, pjtok);
            xlx = GET_XLX(*pjtok);
        }
        if (!jstat) {
            if ((*pjtok)->jtok_kw == JSPU_OPEN_PAREN) {
                jstat = jrun_next_token(jx, pjtok);
                parens++;
            } else {
                if ((*pjtok)->jtok_kw == JSPU_CLOSE_PAREN) {
                    jstat = jrun_next_token(jx, pjtok);
                    parens--;
                } else {
                    jstat = jprep_eval_primary(jx, jxe, pjtok, skip_flags);
                }
                if (!jstat) {
                    xlx = GET_XLX(*pjtok);
                    if (xlx >= 0 && (jxe[xlx].jxe_opflags & JXE_OPFLAG_BINARY_OPERATION)) {
                        jstat = jrun_next_token(jx, pjtok);
                    } else {
                        if (!parens) done = 1;
                    }
                }
            }
        }
    }

    return (jstat);
}
/***************************************************************/
static int jprep_eval_assignment(
                    struct jrunexec * jx,
                    struct jexp_exp_rec * jxe,
                    struct jtoken ** pjtok,
                    int skip_flags)
{
/*
** 12/17/2021
*/
    int jstat;

    jstat = jprep_eval(jx, jxe, pjtok, skip_flags);

    return (jstat);
}
/***************************************************************/
static int jprep_eval_list(
                    struct jrunexec * jx,
                    struct jexp_exp_rec * jxe,
                    struct jtoken ** pjtok,
                    int skip_flags)
{
/*
** 12/17/2021
*/
    int jstat;

    jstat = jprep_eval_assignment(jx, jxe, pjtok, skip_flags);
    while (!jstat &&
           (*pjtok)->jtok_kw == JSPU_COMMA ) {
        jstat = jrun_next_token(jx, pjtok);
        if (!jstat) {
            if ((*pjtok)->jtok_kw == JSPU_CLOSE_PAREN &&        /* 01/06/2022 */
                !jrun_get_stricter_mode(jx)) {
                /* ok - allows: console.log( "Good",); */
            } else {
                jstat = jprep_eval_assignment(jx, jxe, pjtok, skip_flags);
            }
        }
    }

    return (jstat);
}
/***************************************************************/
static int jprep_expression(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok,
                    int skip_flags)
{
/*
** 12/17/2021
*/
    int jstat = 0;
    struct jexp_exp_rec * jxe;

    jxe = jeval_get_exp_list();
    jstat = jprep_eval_list(jx, jxe, pjtok, skip_flags);

    return (jstat);
}
/***************************************************************/
static int jprep_var_stmt(
                    struct jrunexec * jx,
                    struct jvarvalue_function * jvvf,
                    struct jtoken ** pjtok,
                    int skip_flags)
{
/*
** 02/26/2021
**
** Syntax: var varname1 [= value1] [, varname2 [= value2] ... [, varnameN [= valueN]]];
*/
    int jstat = 0;
    int done;
    int vix;
    struct jexp_exp_rec * jxe;

    jxe = jeval_get_exp_list();
    done = 0;
    while (!jstat && !done) {
        if (!jvar_is_valid_varname(jx, (*pjtok))) {
            vix = jvar_find_local_variable_index(jx, jvvf, (*pjtok)->jtok_text);
            if (vix < 0 && (skip_flags & SKIP_FLAG_HOIST)) {
                vix = jvar_new_local_variable(jx, jvvf, (*pjtok)->jtok_text);
            }
            jstat = jrun_next_token(jx, pjtok); /* delimiter or '=' */
            if ((*pjtok)->jtok_kw == JSPU_EQ) {
                jstat = jrun_next_token(jx, pjtok);
                if (!jstat) {
                    jstat = jprep_eval_assignment(jx, jxe, pjtok, skip_flags);
                }
            }
            if (!jstat) {
                if ((*pjtok)->jtok_kw == JSPU_SEMICOLON) {
                    jstat = jrun_next_token(jx, pjtok);
                    done = 1;
                } else if ((*pjtok)->jtok_kw == JSPU_COMMA) {
                    jstat = jrun_next_token(jx, pjtok);
                } else {
//                    jstat = jrun_set_error(jx, JSERROBJ_SCRIPT_SYNTAX_ERROR,
//                        "Unexpected token in var statement: \'%s\'", (*pjtok)->jtok_text);
                    done = 1;
                }
            }
        }
    }

    return (jstat);
}
/***************************************************************/
static int jprep_newvar_stmt(
                    struct jrunexec * jx,
                    struct jtokeninfo * jti_brace,
                    struct jtoken ** pjtok,
                    int jcxflags,
                    int skip_flags)
{
/*
** 12/14/2021
**
** Syntax: const/let varname1 [= value1] [, varname2 [= value2] ... [, varnameN [= valueN]]];
*/
    int jstat = 0;
    int done;
    int vix;
    int * pvix;
    struct jexp_exp_rec * jxe;

    jxe = jeval_get_exp_list();
    done = 0;
    while (!jstat && !done) {
        if (!jvar_is_valid_varname(jx, (*pjtok))) {
            pvix = jvar_find_in_jvarrec(jti_brace->jti_val.jti_jvar, (*pjtok)->jtok_text);
            if (!pvix && (skip_flags & SKIP_FLAG_HOIST)) {
                vix = jvar_insert_into_jvarrec(jti_brace->jti_val.jti_jvar, (*pjtok)->jtok_text);
            }
            jstat = jrun_next_token(jx, pjtok); /* delimiter or '=' */
            if ((*pjtok)->jtok_kw == JSPU_EQ) {
                jstat = jrun_next_token(jx, pjtok);
                if (!jstat) {
                    jstat = jprep_eval_assignment(jx, jxe, pjtok, skip_flags);
                }
            }
            if (!jstat) {
                if ((*pjtok)->jtok_kw == JSPU_SEMICOLON) {
                    jstat = jrun_next_token(jx, pjtok);
                    done = 1;
                } else if ((*pjtok)->jtok_kw == JSPU_COMMA) {
                    jstat = jrun_next_token(jx, pjtok);
                } else {
//                    jstat = jrun_set_error(jx, JSERROBJ_SCRIPT_SYNTAX_ERROR,
//                        "Unexpected token in var statement: \'%s\'", (*pjtok)->jtok_text);
                    done = 1;
                }
            }
        }
    }

    return (jstat);
}
/***************************************************************/
static int jprep_const_stmt(
                    struct jrunexec * jx,
                    struct jtokeninfo * jti_brace,
                    struct jtoken ** pjtok,
                    int skip_flags)
{
/*
** 12/14/2021
**
** Syntax: const varname1 [= value1] [, varname2 [= value2] ... [, varnameN [= valueN]]];
*/
    int jstat = 0;

    jstat = jprep_newvar_stmt(jx, jti_brace, pjtok, JCX_FLAG_CONST, skip_flags);

    return (jstat);
}
/***************************************************************/
static int jprep_let_stmt(
                    struct jrunexec * jx,
                    struct jtokeninfo * jti_brace,
                    struct jtoken ** pjtok,
                    int skip_flags)
{
/*
** 12/14/2021
**
** Syntax: let varname1 [= value1] [, varname2 [= value2] ... [, varnameN [= valueN]]];
*/
    int jstat = 0;

    jstat = jprep_newvar_stmt(jx, jti_brace, pjtok, 0, skip_flags);

    return (jstat);
}
/***************************************************************/
struct jtokeninfo * jprep_ensure_jtokeninfo(
                    struct jrunexec * jx,
                    enum e_jti_type titype,
                    struct jtoken * jtok)
{
/*
** 02/14/2021
**
** See also: jrun_exec_generic_inactive_stmt()
**
*/
    struct jtokeninfo * jti;

    jti = jtok->jtok_jti;
    if (!jti) {
        jti = jtok_new_jtokeninfo(titype);
        switch (jti->jti_type) {
            case JTI_TYPE_BRACE:
                jti->jti_val.jti_jvar = jvar_new_jvarrec();
                INCVARRECREFS(jti->jti_val.jti_jvar);
                break;

            case JTI_TYPE_FUNCTION:
                jti->jti_val.jti_end_jxc = jrun_new_jxcursor();
                break;

            default:
                break;
        }
        jtok->jtok_jti = jti;
    }
#if IS_DEBUG
    else if (jti->jti_type != titype) {
        printf("** DEBUG: jtokeninfo type conflict. jti->jti_type=%d titype=%d\n", jti->jti_type, titype);
    }
#endif

    return (jti);
}
/***************************************************************/
int jprep_preprocess_block(
                    struct jrunexec * jx,
                    struct jvarvalue_function * jvvf,
                    struct jtoken ** pjtok,
                    int skip_flags)
{
/*
** 02/14/2021
**
** See also: jrun_exec_inactive_block()
**
*/
    int jstat = 0;
    int done;
    int braces;
    int brmax;
    struct jtokeninfo * jti_func;
    struct jtokeninfo * jti_brace;
    struct jtoken ** jtok_stack;

    done = 0;
    braces = 0;
    brmax = 0;
    jtok_stack = NULL;
    while (!jstat && !done) {
    
        switch ((*pjtok)->jtok_kw) {
            case JSPU_CLOSE_BRACE:
                if (braces) braces--;
                if (!braces) {
                    done = 1;
                }
                jstat = jrun_next_token(jx, pjtok);
                break;

            case JSPU_OPEN_BRACE:
                if (braces + 1 >= brmax) {
                    if (!brmax) brmax = 4;
                    else brmax *= 2;
                    jtok_stack = Realloc(jtok_stack, struct jtoken *, brmax);
                }
                jtok_stack[braces] = (*pjtok);
                braces++;
                jstat = jrun_next_token(jx, pjtok);
                break;

            case JSPU_SEMICOLON:
                jstat = jrun_next_token(jx, pjtok);
                break;

            /**** statements ****/

            case JSKW_CATCH:
                jstat = jrun_next_token(jx, pjtok);
                if (!jstat && (*pjtok)->jtok_kw == JSPU_OPEN_PAREN) {
                    jstat = jrun_next_token(jx, pjtok);
                    if (!jstat) {
                        jstat = jrun_next_token(jx, pjtok);
                        if (!jstat && (*pjtok)->jtok_kw == JSPU_CLOSE_PAREN) {
                            jstat = jrun_next_token(jx, pjtok);
                        }
                    }
                }
                break;

            case JSKW_CONST:
                jstat = jrun_next_token(jx, pjtok);
                if (!jstat && braces) {
                    jti_brace = jprep_ensure_jtokeninfo(jx, JTI_TYPE_BRACE, jtok_stack[braces - 1]);
                    jstat = jprep_const_stmt(jx, jti_brace, pjtok, skip_flags);
                }
                break;

            case JSKW_ELSE:
                jstat = jrun_next_token(jx, pjtok);
                break;

            case JSKW_FINALLY:
                jstat = jrun_next_token(jx, pjtok);
                break;

            case JSKW_FUNCTION:
                jti_func = jprep_ensure_jtokeninfo(jx, JTI_TYPE_FUNCTION, (*pjtok));
                jstat = jrun_next_token(jx, pjtok);
                if (!jstat && braces) {
                    jti_brace = jprep_ensure_jtokeninfo(jx, JTI_TYPE_BRACE, jtok_stack[braces - 1]);
                    jstat = jprep_function_stmt(jx, jti_func, jti_brace, pjtok, skip_flags);
                }
                break;

            case JSKW_IF:
                jstat = jrun_next_token(jx, pjtok);
                if (!jstat) jstat = jprep_if_stmt(jx, pjtok, skip_flags);
                break;

            case JSKW_LET:
                jstat = jrun_next_token(jx, pjtok);
                if (!jstat && braces) {
                    jti_brace = jprep_ensure_jtokeninfo(jx, JTI_TYPE_BRACE, jtok_stack[braces - 1]);
                    jstat = jprep_let_stmt(jx, jti_brace, pjtok, skip_flags);
                }
                break;

            case JSKW_RETURN:
                jstat = jrun_next_token(jx, pjtok);
                if (!jstat) {
                    if ((*pjtok)->jtok_kw == JSPU_SEMICOLON) {
                        jstat = jrun_next_token(jx, pjtok);
                    } else {
                        jstat = jprep_expression(jx, pjtok, skip_flags);
                    }
                }
                break;

            case JSKW_TRY:
                jstat = jrun_next_token(jx, pjtok);
                break;

            case JSKW_WHILE:
                jstat = jrun_next_token(jx, pjtok);
                if (!jstat) jstat = jprep_while_stmt(jx, pjtok, skip_flags);
                break;

            case JSKW_VAR:
                jstat = jrun_next_token(jx, pjtok);
                if (!jstat) jstat = jprep_var_stmt(jx, jvvf, pjtok, skip_flags);
                break;

            default:
                jstat = jprep_expression(jx, pjtok, skip_flags);
                break;
        }
    }

    Free(jtok_stack);

    return (jstat);
}
/***************************************************************/
int jprep_preprocess_main(
                    struct jrunexec * jx,
                    struct jvarvalue_function ** pjvvf)
{
/*
** 03/01/2022
*/
    int jstat = 0;
    struct jtoken * jtok;
    struct jvarvalue_function * jvvf;

    (*pjvvf) = NULL;

    jvvf = new_jvarvalue_function();
    jvvf->jvvf_func_name = Strdup(MAIN_FUNCTION_NAME);
    jx->jx_flags |= JX_FLAG_PREPROCESSING;
    jvvf->jvvf_flags |= JVVF_FLAG_RUN_TO_EOF;
    INCFUNCREFS(jvvf);

    /* jrun_get_current_jxc(jx, &(jvvf->jvvf_begin_jxc)); */
    jrun_copy_current_jxc(jx, &(jvvf->jvvf_begin_jxc));

    jstat = jrun_next_token(jx, &jtok);
    if (jstat) return (jstat);

    jstat = jprep_preprocess_block(jx, jvvf, &jtok, SKIP_FLAG_HOIST);
    if (jstat == JERR_EOF && (jvvf->jvvf_flags & JVVF_FLAG_RUN_TO_EOF)) {
        jstat = 0;
    }

    if (!jstat) {
        (*pjvvf) = jvvf;
    } else {
        jvar_free_jvarvalue_function(jvvf);
    }

    if (jx->jx_flags & JX_FLAG_PREPROCESSING) jx->jx_flags ^= JX_FLAG_PREPROCESSING;


    return (jstat);
}
/***************************************************************/
