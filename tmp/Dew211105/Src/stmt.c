/***************************************************************/
/*
**  jsenv.c --  JavaScript Environment/main
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
#include "jsexp.h"
#include "dbtreeh.h"
#include "snprfh.h"
#include "internal.h"
#include "var.h"
#include "prep.h"
#include "stmt.h"
#include "print.h"

#define DEBUG_XSTACK            0

/***************************************************************/
static void printf_xstate(int xstate)
{
    char xbuf[32];

    jpr_xstack_char(xbuf, xstate);
    printf("%s", xbuf);
}
/***************************************************************/
static void jrun_print_xstack(struct jrunexec * jx)
{
/*
** See also: jpr_exec_debug_show_runstack()
*/
    int sp;

    printf("[");
    for (sp = 1; sp < jx->jx_njs; sp++) {
        if (sp > 1) printf(",");

        printf_xstate(jx->jx_js[sp]->jxs_xstate);
    }
    printf("]\n");
}
/***************************************************************/
static int jrun_process_use_string(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok,
                    int * pskip_eval)
{
/*
** 01/12/2021
*/
    int jstat = 0;
    int len;
    char * dbgstr;

    (*pskip_eval) = 0;
    if ((*pjtok)->jtok_tlen > 5 && !strncmp((*pjtok)->jtok_text + 1, "use ", 4)) {
        if ((*pjtok)->jtok_tlen == 12 && !strncmp((*pjtok)->jtok_text + 5, "strict", 6)) {
            jstat = jrun_set_strict_mode(jx);
            (*pskip_eval) = 1;
        }
#if IS_DEBUG
    } else if ((*pjtok)->jtok_tlen > 5 && !strncmp((*pjtok)->jtok_text + 1, "debug ", 5)) {
        len = IStrlen((*pjtok)->jtok_text);
        dbgstr = New(char, len - 1);
        memcpy(dbgstr, (*pjtok)->jtok_text + 1, len - 2);
        dbgstr[len - 2] = '\0';
        //dbgstr = Strdup((*pjtok)->jtok_text);
        jstat = jrun_debug_chars(jx, dbgstr);
        Free(dbgstr);
        (*pskip_eval) = 1;
#endif
    }

    return (jstat);
}
/***************************************************************/
static int jrun_exec_implicit_stmt(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok)
{
    int jstat = 0;
    int skip_eval = 0;
    struct jvarvalue jvv;

    if ((*pjtok)->jtok_typ == JSW_STRING) {
        jstat = jrun_process_use_string(jx, pjtok, &skip_eval);
    }
    if (skip_eval) {
        jstat = jrun_next_token(jx, pjtok);
    } else  {
        jstat = jexp_evaluate(jx, pjtok, &jvv);
        //jvar_free_jvarvalue_data(&jvv);   -- Removed 07/21/21
    }

    if (!jstat) {
        if ((*pjtok)->jtok_kw != JSPU_SEMICOLON) {
                    jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERROBJ_SCRIPT_SYNTAX_ERROR,
                "Expecting semicolon. Found \'%s\'", (*pjtok)->jtok_text);
        }
    }

    return (jstat);
}
/***************************************************************/
static int jrun_exec_if_stmt(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok)
{
/*
** 01/17/2021
** Omar
    svstat = svare_evaluate(svx, svt, &svv);
    if (!svstat) {
        if (svv.svv_dtype != SVV_DTYPE_BOOL) {
            svstat = svare_set_error(svx, SVARERR_EXPECT_BOOLEAN,
                "Expecting boolean value for if.");
        } else {
            if (svv.svv_val.svv_val_bool) {
                svstat = svare_push_xstat(svx, XSTAT_TRUE_IF);
            } else {
                svstat = svare_push_xstat(svx, XSTAT_FALSE_IF);
            }
        }
    }
*/
    int jstat = 0;
    int istrue;
    struct jvarvalue jvv;

    if ((*pjtok)->jtok_kw != JSPU_OPEN_PAREN) {
        jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERROBJ_SCRIPT_SYNTAX_ERROR,
            "Expecting open parenthesis. Found \'%s\'", (*pjtok)->jtok_text);
    } else {
        jstat = jrun_next_token(jx, pjtok);
        if (!jstat) {
            jstat = jexp_evaluate(jx, pjtok, &jvv);
            if (!jstat) {
                jstat = jexp_istrue(jx, &jvv, &istrue);
                if (!jstat) {
                    if (istrue) {
                        jstat = jrun_push_xstat(jx, XSTAT_TRUE_IF);
                    } else {
                        jstat = jrun_push_xstat(jx, XSTAT_FALSE_IF);
                    }
                    if ((*pjtok)->jtok_kw != JSPU_CLOSE_PAREN) {
                        jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERROBJ_SCRIPT_SYNTAX_ERROR,
                            "Expecting close parenthesis. Found \'%s\'", (*pjtok)->jtok_text);
                    } else {
                        jstat = jrun_next_token(jx, pjtok);
                    }
                }
            }
        }
    }

    return (jstat);
}
/***************************************************************/
static int jrun_exec_inactive_if(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok)
{
/*
** 01/17/2021
** Omar
    svstat = svare_evaluate_inactive(svx, svt);
    if (!svstat) {
        svstat = svare_push_xstat(svx, XSTAT_INACTIVE_IF);
    }
*/
    int jstat = 0;

    if ((*pjtok)->jtok_kw != JSPU_OPEN_PAREN) {
        jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERROBJ_SCRIPT_SYNTAX_ERROR,
            "Expecting open parenthesis. Found \'%s\'", (*pjtok)->jtok_text);
    } else {
        jstat = jrun_next_token(jx, pjtok);
        if (!jstat) {
            jstat = jexp_evaluate_inactive(jx, pjtok);
            if (!jstat) jstat = jrun_push_xstat(jx, XSTAT_INACTIVE_IF);
            if (!jstat)  {
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
static int jrun_exec_try_stmt(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok)
{
/*
** 11/3/2021
*/
    int jstat = 0;

    jstat = jrun_pop_push_xstat(jx, XSTAT_TRY);

    return (jstat);
}
/***************************************************************/
static int jrun_exec_catch_stmt(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok)
{
/*
** 11/3/2021
*/
    int jstat = 0;

    return (jstat);
}
/***************************************************************/
static int jrun_exec_finally_stmt(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok)
{
/*
** 11/3/2021
*/
    int jstat = 0;

    return (jstat);
}
/***************************************************************/
static int jrun_exec_while_stmt(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok)
{
/*
** 02/01/2021
** Omar
    svstat = svare_evaluate(svx, svt, &svv);
    if (!svstat) {
        if (svv.svv_dtype != SVV_DTYPE_BOOL) {
            svstat = svare_set_error(svx, SVARERR_EXPECT_BOOLEAN,
                "Expecting boolean value for while.");
        } else {
            if (svv.svv_val.svv_val_bool) {
                svstat = svare_push_xstat(svx, XSTAT_TRUE_WHILE);
            } else {
                svstat = svare_push_xstat(svx, XSTAT_INACTIVE_WHILE);
            }
        }
    }
*/
    int jstat = 0;
    int istrue;
    struct jvarvalue jvv;

    if ((*pjtok)->jtok_kw != JSPU_OPEN_PAREN) {
        jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERROBJ_SCRIPT_SYNTAX_ERROR,
            "Expecting open parenthesis. Found \'%s\'", (*pjtok)->jtok_text);
    } else {
        jstat = jrun_next_token(jx, pjtok);
        if (!jstat) {
            jstat = jexp_evaluate(jx, pjtok, &jvv);
            if (!jstat) {
                jstat = jexp_istrue(jx, &jvv, &istrue);
                if (!jstat) {
                    if (istrue) {
                        jstat = jrun_push_xstat(jx, XSTAT_TRUE_WHILE);
                    } else {
                        jstat = jrun_push_xstat(jx, XSTAT_INACTIVE_WHILE);
                    }
                    if ((*pjtok)->jtok_kw != JSPU_CLOSE_PAREN) {
                        jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERROBJ_SCRIPT_SYNTAX_ERROR,
                            "Expecting close parenthesis. Found \'%s\'", (*pjtok)->jtok_text);
                    } else {
                        jstat = jrun_next_token(jx, pjtok);
                    }
                }
            }
        }
    }

    return (jstat);
}
/***************************************************************/
static int jrun_exec_inactive_while(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok)
{
/*
** 02/01/2021
** Omar
    svstat = svare_evaluate_inactive(svx, svt);
    if (svstat) svstat = svare_clear_error(svx, svstat);
    if (!svstat) {
        svstat = svare_push_xstat(svx, XSTAT_INACTIVE_WHILE);
    }
*/
    int jstat = 0;

    if ((*pjtok)->jtok_kw != JSPU_OPEN_PAREN) {
        jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERROBJ_SCRIPT_SYNTAX_ERROR,
            "Expecting open parenthesis. Found \'%s\'", (*pjtok)->jtok_text);
    } else {
        jstat = jrun_next_token(jx, pjtok);
        if (!jstat) {
            jstat = jexp_evaluate_inactive(jx, pjtok);
            if (!jstat) jstat = jrun_push_xstat(jx, XSTAT_INACTIVE_WHILE);
            if (!jstat)  {
                if ((*pjtok)->jtok_kw != JSPU_CLOSE_PAREN) {
                    jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERROBJ_SCRIPT_SYNTAX_ERROR,
                        "Expecting close parenthesis. Found \'%s\'", (*pjtok)->jtok_text);
                }
            }
        }
    }

    return (jstat);
}
/***************************************************************/
static int jrun_exec_generic_inactive_stmt(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok)
{
/*
**
**
** See also: jprep_skip_generic_statement()
*/
    int jstat = 0;
    int done;
    int braces;
/*
** Omar
        int braces;

        while (XSTAT_IS_COMPLETE(svx)) {
            svstat = svare_pop_xstat(svx, 0);
        }

        done = 0;
        braces = 0;
        while (!svstat && !done) {
            svstat = svare_get_token(svx, svt);
            if (SVAR_BLOCK_BEGIN_CHAR(svt->svt_text[0])) {
                braces++;
            } else if (SVAR_BLOCK_END_CHAR(svt->svt_text[0])) {
                braces--;
                if (!braces) done = 1;
            } else if (SVAR_STMT_TERM_CHAR(svt->svt_text[0])) {
                if (!braces) done = 1;
            }
        }

        if (!svstat) {
            svstat = svare_update_xstat(svx);
        }

        if (SVAR_STMT_END_CHAR(svt->svt_text[0])) {
            svstat = svare_get_token(svx, svt);
        }
*/

    while (XSTAT_IS_COMPLETE(jx)) { jstat = jrun_pop_xstat(jx); }

    done = 0;
    braces = 0;
    while (!jstat && !done) {
        jstat = jrun_next_token(jx, pjtok);
        if ((*pjtok)->jtok_kw == JSPU_OPEN_BRACE) {
            braces++;
        } else if ((*pjtok)->jtok_kw == JSPU_CLOSE_BRACE) {
            braces--;
            if (!braces) done = 1;
        } else if ((*pjtok)->jtok_kw == JSPU_SEMICOLON) {
            //jrun_unget_token(jx, pjtok);
            if (!braces) {
                done = 1;
                jstat = jrun_update_xstat(jx);
                if (!jstat) jstat = jrun_next_token(jx, pjtok);
            }
        }
    }

    return (jstat);
}
/***************************************************************/
static int jrun_exec_inactive_stmt(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok)
{
/*
**
*/
    int jstat = 0;

    switch ((*pjtok)->jtok_kw) {
#if IS_DEBUG
        case JSPU_AT_AT:
            jstat = JERR_EXIT;
            break;
#endif

        case JSKW_ELSE:
/*
** Omar
        svstat = svare_get_token(svx, svt);
        if (svx->svx_svs[svx->svx_nsvs - 1]->svs_xstate == XSTAT_FALSE_IF) {
            svstat = svare_pop_push_xstat(svx, XSTAT_FALSE_IF, XSTAT_TRUE_ELSE);
        } else if (svx->svx_svs[svx->svx_nsvs - 1]->svs_xstate == XSTAT_INACTIVE_IF) {
            svstat = svare_pop_push_xstat(svx, XSTAT_INACTIVE_IF, XSTAT_INACTIVE_ELSE);
        }
*/
            jstat = jrun_next_token(jx, pjtok);
            if (jx->jx_js[jx->jx_njs - 1]->jxs_xstate == XSTAT_FALSE_IF) {
                jstat = jrun_pop_push_xstat(jx, XSTAT_TRUE_ELSE);
            } else if (jx->jx_js[jx->jx_njs - 1]->jxs_xstate == XSTAT_INACTIVE_IF) {
                jstat = jrun_pop_push_xstat(jx, XSTAT_INACTIVE_ELSE);
            }
            break;

        case JSKW_IF:
/*
** Omar
        svstat = svare_update_xstat(svx);
        if (!svstat) svstat = svare_get_token(svx, svt);
        if (!svstat) {
            svstat = svare_exec_inactive_if(svx, svt);
        }
*/
            jstat = jrun_update_xstat(jx);
            if (!jstat) jstat = jrun_next_token(jx, pjtok);
            if (!jstat) jstat = jrun_exec_inactive_if(jx, pjtok);
            break;

        case JSKW_WHILE:
/*
** Omar
        svstat = svare_update_xstat(svx);
        if (!svstat) svstat = svare_get_token(svx, svt);
        if (!svstat) {
            svstat = svare_exec_inactive_while(svx, svt);
        }
*/
            jstat = jrun_update_xstat(jx);
            if (!jstat) jstat = jrun_next_token(jx, pjtok);
            if (!jstat) jstat = jrun_exec_inactive_while(jx, pjtok);
            break;

        case JSPU_SEMICOLON:
/*
** Omar
    } else if (!strcmp(svt->svt_text, ";")) {  
        while (XSTAT_IS_COMPLETE(svx)) {       
            svstat = svare_pop_xstat(svx, 0);  
        }                                      
        if (!svstat) {                         
            svstat = svare_update_xstat(svx);  
        }                                      
        if (!svstat) {                         
            svstat = svare_get_token(svx, svt);
        }                                      
*/
            while (XSTAT_IS_COMPLETE(jx)) { jstat = jrun_pop_xstat(jx); }
            if (!jstat) jstat = jrun_update_xstat(jx);
            if (!jstat) jstat = jrun_next_token(jx, pjtok);
            break;

        default:
            /* jstat = jrun_exec_generic_inactive_stmt(jx, pjtok); */
            jstat = jexp_evaluate_inactive(jx, pjtok);
            break;
    }

    return (jstat);
}
/***************************************************************/
static int jrun_exec_newvar_stmt(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok,
                    int jcxflags)
{
/*
** 09/21/2021 - let/const statement for new vars
**
** Syntax: let/const varname1 [= value1] [, varname2 [= value2] ... [, varnameN [= valueN]]];
*/

    int jstat = 0;
    int done;
    int vix;
    int injcxflags;
    struct jcontext * jcx;
    struct jvarrec * jvar;
    int * pvix;
    struct jfuncstate * jxfs;
    struct jvarvalue * njvv;
    struct jtoken next_jtok;
    struct jvarvalue   vjvv;

    jcx = XSTAT_JCX(jx);
    if (!jcx) {
        if (XSTAT_XSTATE(jx) != XSTAT_ACTIVE_BLOCK) {
            jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_MUST_BE_BLOCK,
                "Variable \'%s\' must be declared in block.", (*pjtok)->jtok_text);
        } else {
            jvar = jvar_new_jvarrec();
            jcx = jvar_new_jcontext(jx, jvar, jvar_get_head_jcontext(jx));
            jvar_set_head_jcontext(jx, jcx);
            XSTAT_JCX(jx) = jcx;
        }
    }

    done = 0;
    while (!jstat && !done) {
        pvix = jvar_find_in_jvarrec(jcx->jcx_jvar, (*pjtok)->jtok_text);
        if (!pvix) {
            if (XSTAT_FLAGS(jx) & JXS_FLAGS_BEGIN_FUNC) {
                /* Check if variable was also var */
                jxfs = jvar_get_current_jfuncstate(jx);
                if (jxfs) {
                    pvix = jvar_find_in_jvarrec(jxfs->jfs_jcx->jcx_jvar, (*pjtok)->jtok_text);
                    if (pvix) {
                        jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_DUPLICATE_VARIABLE,
                            "Variable \'%s\' is duplicate of var.", (*pjtok)->jtok_text);
                    }
                }
            }
            jstat = jvar_validate_varname(jx, (*pjtok));
            if (!jstat) {
                vix = jvar_insert_into_jvarrec(jcx->jcx_jvar, (*pjtok)->jtok_text);
                njvv = jvar_get_jvv_with_expand(jx, jcx, vix, &jcxflags);
            }
        } else {
            njvv = jvar_get_jvv_from_vix(jx, jcx, *pvix, &injcxflags);
        }

        if (!jstat) {
            jrun_peek_token(jx, &next_jtok);
            if (next_jtok.jtok_kw == JSPU_EQ) {
                if (!jstat) jstat = jrun_next_token(jx, pjtok); /* '=' */
                if (!jstat) jstat = jrun_next_token(jx, pjtok);
                if (!jstat) jstat = jexp_eval_value(jx, pjtok, &vjvv);
                if (!jstat) {
                    jstat = jvar_assign_jvarvalue(jx, njvv, &vjvv);
                    if (!jstat) {
                        /* jvar_set_jvvflags(jvv, jvvflags); */
                        jvar_free_jvarvalue_data(&vjvv);
                    }
                }
            } else if (jcxflags & JCX_FLAG_CONST) {
                jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_EXPECT_EQUALS,
                    "Const statement requires value for \'%s\'", (*pjtok)->jtok_text);
            }
        }
        if (!jstat) {
            if ((*pjtok)->jtok_kw == JSPU_SEMICOLON) {
                jstat = jrun_next_token(jx, pjtok);
                done = 1;
            } else if ((*pjtok)->jtok_kw == JSPU_COMMA) {
                jstat = jrun_next_token(jx, pjtok);
            } else {
                jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERROBJ_SCRIPT_SYNTAX_ERROR,
                    "Unexpected token \'%s\'", (*pjtok)->jtok_text);
            }
        }
    }

    return (jstat);
}
/***************************************************************/
static int jrun_exec_const_stmt(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok)
{
/*
** 09/23/2021 - const statement for new vars
**
** Syntax: const varname1 [= value1] [, varname2 [= value2] ... [, varnameN [= valueN]]];
*/

    int jstat;

    jstat = jrun_exec_newvar_stmt(jx, pjtok, JCX_FLAG_CONST);

    return (jstat);
}
/***************************************************************/
static int jrun_exec_let_stmt(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok)
{
/*
** 09/21/2021 - let statement for new vars
**
** Syntax: let varname1 [= value1] [, varname2 [= value2] ... [, varnameN [= valueN]]];
*/

    int jstat;

    jstat = jrun_exec_newvar_stmt(jx, pjtok, 0);

    return (jstat);
}
/***************************************************************/
static int jrun_exec_var_stmt(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok)
{
/*
** 03/02/2021 - var statement for previously hoisted vars
**
** Syntax: var varname1 [= value1] [, varname2 [= value2] ... [, varnameN [= valueN]]];
*/
    int jstat = 0;
    int done;
    int jcxflags = 0;
    struct jvarvalue * jvv;
    struct jtoken next_jtok;
    struct jvarvalue   vjvv;
    struct jcontext * var_jcx;

    done = 0;
    while (!jstat && !done) {
        jvv = jvar_find_variable(jx, (*pjtok), &var_jcx, &jcxflags);
        if (!jvv) {
            jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_INTERNAL_ERROR,
                "Cannot find hoisted variable \'%s\'", (*pjtok)->jtok_text);
        } else {
            jrun_peek_token(jx, &next_jtok);
            if (next_jtok.jtok_kw == JSPU_EQ) {
                if (!jvv) {
                    jvv = jvar_new_jvarvalue();
                    jstat = jvar_insert_new_global_variable(jx, (*pjtok)->jtok_text, jvv, 0);
                }
                if (!jstat) jstat = jrun_next_token(jx, pjtok); /* '=' */
                if (!jstat) jstat = jrun_next_token(jx, pjtok);
                if (!jstat) jstat = jexp_eval_value(jx, pjtok, &vjvv);
                if (!jstat) {
                    jstat = jvar_assign_jvarvalue(jx, jvv, &vjvv);
                    if (!jstat) jvar_free_jvarvalue_data(&vjvv);
                }
            } else {    /* No '=' */
                if (jvv) {
                    jstat = jrun_next_token(jx, pjtok);
                } else {
                    jvv = jvar_new_jvarvalue();
                    jstat = jvar_insert_new_global_variable(jx, (*pjtok)->jtok_text, jvv, 0);
                    if (!jstat) jstat = jrun_next_token(jx, pjtok);
                }
            }
            if (!jstat) {
                if ((*pjtok)->jtok_kw == JSPU_SEMICOLON) {
                    jstat = jrun_next_token(jx, pjtok);
                    done = 1;
                } else if ((*pjtok)->jtok_kw == JSPU_COMMA) {
                    jstat = jrun_next_token(jx, pjtok);
                } else {
                    jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERROBJ_SCRIPT_SYNTAX_ERROR,
                        "Unexpected token \'%s\'", (*pjtok)->jtok_text);
                }
            }
        }
    }

    return (jstat);
}
/***************************************************************/
static int jrun_exec_active_stmt(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok)
{
/*
**
*/
    int jstat = 0;
    int is_active;
    int do_update;
    struct jxcursor current_jxc;

    do_update = 1;
    if ((*pjtok)->jtok_typ == JSW_KW) do_update = !((*pjtok)->jtok_flags & JSKWI_NO_UPDATE);
    
    switch ((*pjtok)->jtok_kw) {
#if IS_DEBUG
        case JSPU_AT_AT:
            jstat = JERR_EXIT;
            break;
#endif

        case JSKW_CATCH:
            jstat = jrun_next_token(jx, pjtok);
            if (!jstat) jstat = jrun_exec_catch_stmt(jx, pjtok);
            break;

        case JSKW_CONST:
            jstat = jrun_next_token(jx, pjtok);
            if (!jstat) jstat = jrun_exec_const_stmt(jx, pjtok);
            break;

        case JSKW_ELSE:
/*
** Omar
        if (!XSTAT_IS_COMPLETE(svx)) {                                                    
            svstat = svare_set_error(svx, SVARERR_EXP_IF,                                 
                "else expects previous statement to be if. Found: \"%s\"", svt->svt_text);
        } else {                                                                          
            svstat = svare_update_xstat(svx);
            if (!svstat) svstat = svare_get_token(svx, svt);
        }
*/
            if (!XSTAT_IS_COMPLETE(jx)) {                                                    
                jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERROBJ_SCRIPT_SYNTAX_ERROR,
                    "Unexpected token \'%s\'", (*pjtok)->jtok_text);
            } else {                                                                          
                jstat = jrun_update_xstat(jx);
                if (!jstat) jstat = jrun_next_token(jx, pjtok);
            }
            break;

        case JSKW_FINALLY:
            jstat = jrun_next_token(jx, pjtok);
            if (!jstat) jstat = jrun_exec_finally_stmt(jx, pjtok);
            break;

        case JSKW_FUNCTION:
            jstat = jrun_next_token(jx, pjtok);
            if (!jstat) jstat = jrun_exec_function_stmt(jx, pjtok);
            break;

        case JSKW_IF:
/*
** Omar
        while (XSTAT_IS_COMPLETE(svx)) { svstat = svare_pop_xstat(svx, 0); }
        svstat = svare_update_xstat(svx);
        is_active = XSTAT_IS_ACTIVE(svx);                   
        if (!svstat) svstat = svare_get_token(svx, svt);
        if (!svstat) {
            if (!is_active) {                               
                svstat = svare_exec_inactive_if(svx, svt);  
            } else {
                svstat = svare_exec_if(svx, svt);
            }
        }

*/
            while (XSTAT_IS_COMPLETE(jx)) { jstat = jrun_pop_xstat(jx); }
            if (!jstat) jstat = jrun_update_xstat(jx);
            is_active = XSTAT_IS_ACTIVE(jx);
            if (!jstat) jstat = jrun_next_token(jx, pjtok);
            if (!jstat) {
                if (!is_active) {                               
                    jstat = jrun_exec_inactive_if(jx, pjtok);
                } else {
                    jstat = jrun_exec_if_stmt(jx, pjtok);
                }
            }
            break;

        case JSKW_LET:
            jstat = jrun_next_token(jx, pjtok);
            if (!jstat) jstat = jrun_exec_let_stmt(jx, pjtok);
            break;

        case JSKW_RETURN:
            jstat = jrun_next_token(jx, pjtok);
            if (!jstat) jstat = jrun_exec_return_stmt(jx, pjtok);
            break;

        case JSKW_TRY:
            jstat = jrun_next_token(jx, pjtok);
            if (!jstat) jstat = jrun_exec_try_stmt(jx, pjtok);
            break;

        case JSKW_WHILE:
/*
** Omar
        while (XSTAT_IS_COMPLETE(svx)) { svstat = svare_pop_xstat(svx, 0); }
        svare_get_current_svc(svx, &current_svc);
        svstat = svare_update_xstat(svx);
        if (!svstat) svstat = svare_get_token(svx, svt);
        if (!svstat) {
            svstat = svare_exec_while(svx, svt);
            if (!svstat && svx->svx_svs[svx->svx_nsvs - 1]->svs_xstate == XSTAT_TRUE_WHILE) {
                svare_set_xstat_svc(svx, &current_svc);
            }

        }
*/
            while (XSTAT_IS_COMPLETE(jx)) { jstat = jrun_pop_xstat(jx); }
            if (!jstat) jstat = jrun_update_xstat(jx);
            if (!jstat) {
                jrun_get_current_jxc(jx, &current_jxc);
                jstat = jrun_next_token(jx, pjtok);
            }
            if (!jstat) jstat = jrun_exec_while_stmt(jx, pjtok);
            if (!jstat && jx->jx_js[jx->jx_njs - 1]->jxs_xstate == XSTAT_TRUE_WHILE) {
                jrun_set_xstat_jxc(jx, &current_jxc);
            }
            break;

        case JSKW_VAR:
            jstat = jrun_next_token(jx, pjtok);
            if (!jstat) jstat = jrun_exec_var_stmt(jx, pjtok);
            break;

        case JSPU_SEMICOLON:
            jstat = jrun_next_token(jx, pjtok);
            break;

        default:
/*
** Omar
        while (XSTAT_IS_COMPLETE(svx)) { svstat = svare_pop_xstat(svx, 0); }
        is_active = XSTAT_IS_ACTIVE(svx);

        if (!is_active) {                               
            svstat = svare_exec_inactive_stmt(svx, svt);
        } else {                                        
*/
            while (XSTAT_IS_COMPLETE(jx)) { jstat = jrun_pop_xstat(jx); }
            is_active = XSTAT_IS_ACTIVE(jx);
            if (!is_active) {                               
                jstat = jrun_exec_inactive_stmt(jx, pjtok);
            } else {                                        
                jstat = jrun_exec_implicit_stmt(jx, pjtok);
                if (!jstat && do_update) jstat = jrun_update_xstat(jx);
                if (!jstat) jstat = jrun_next_token(jx, pjtok);
            }
            /* jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_UNSUPPORTED_STMT,     */
            /*     "Unsupported statement: %s", (*pjtok)->jtok_text); */
            break;
    }

    return (jstat);
}
/***************************************************************/
static void jrun_error_recovery(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok,
                    int *pjstat)
{
/*
** 11/04/2021
*/
    int jstat = 0;
    int done = 0;

    while (!jstat && !done) {
        switch ((*pjtok)->jtok_kw) {
            case JSPU_OPEN_BRACE:
                jstat = jrun_push_xstat(jx, XSTAT_INACTIVE_BLOCK);
                if (!jstat) jstat = jrun_next_token(jx, pjtok);
                break;

            case JSPU_CLOSE_BRACE:
                jstat = jrun_pop_xstat(jx);
                if (!jstat) jstat = jrun_next_token(jx, pjtok);
                break;

            case JSKW_CATCH:
                done = 1;
                break;

            case JSKW_FINALLY:
                done = 1;
                break;

            default:
                jstat = jrun_exec_inactive_stmt(jx, pjtok);
                break;
        }
    }
}
/***************************************************************/
int jrun_exec_block(struct jrunexec * jx, struct jtoken ** pjtok)
{
/*
**
*/
    int jstat = 0;
    int is_active;
    //struct jcontext * jcx;

    while (!jstat) {
        is_active = XSTAT_IS_ACTIVE(jx);
        if ((*pjtok)->jtok_kw == JSPU_OPEN_BRACE) {
#if DEBUG_XSTACK & 4
            printf("================================================================\n");
            printf("4 jrun_exec_block() %-8s toke=%-6s, stack=",
                   (is_active?"active":"inactive"), (*pjtok)->jtok_text);
            jrun_print_xstack(jx);
#endif
            while (XSTAT_IS_COMPLETE(jx)) { jstat = jrun_pop_xstat(jx); }

            if (is_active) {
                if (XSTAT_XSTATE(jx) == XSTAT_FUNCTION) {
                    jstat = jrun_push_xstat(jx, XSTAT_ACTIVE_BLOCK);
                    XSTAT_FLAGS(jx) |= JXS_FLAGS_BEGIN_FUNC;
                } else {
                    jstat = jrun_push_xstat(jx, XSTAT_ACTIVE_BLOCK);
                }
            } else {
                jstat = jrun_push_xstat(jx, XSTAT_INACTIVE_BLOCK);
            }
            if (!jstat) jstat = jrun_next_token(jx, pjtok);
        } else if ((*pjtok)->jtok_kw == JSPU_CLOSE_BRACE) {
#if DEBUG_XSTACK & 4
            printf("================================================================\n");
            printf("4 jrun_exec_block() %-8s toke=%-6s, stack=",
                   (is_active?"active":"inactive"), (*pjtok)->jtok_text);
            jrun_print_xstack(jx);
#endif
            //jcx = XSTAT_JCX(jx);
            //if (jcx) {
            //    jvar_set_head_jcontext(jx, jcx->jcx_outer_jcx);
            //    jvar_free_jcontext(jcx);
            //}

            while (XSTAT_IS_COMPLETE(jx)) { jstat = jrun_pop_xstat(jx); }

            jstat = jrun_pop_xstat(jx);

            if (XSTAT_XSTATE(jx) == XSTAT_FUNCTION) {
                //jstat = jrun_push_xstat(jx, XSTAT_RETURN);
                jstat = JERR_RETURN;
            } else {
                //  if (jx->jx_js[jx->jx_njs - 1]->jxs_xstate == XSTAT_DEFINE_FUNCTION) {
                //      if (!jstat) jstat = jrun_pop_save_tokens(jx);
                //  }

                if (!jstat) jstat = jrun_update_xstat(jx);
                if (!jstat) jstat = jrun_next_token(jx, pjtok);
            }
        } else {
#if DEBUG_XSTACK & 8
            printf("================================================================\n");
            printf("8 jrun_exec_block() %-8s toke=%-6s, stack=",
                   (is_active?"active":"inactive"), (*pjtok)->jtok_text);
            jrun_print_xstack(jx);
#endif
            if (is_active) {
                jstat = jrun_exec_active_stmt(jx, pjtok);
            } else {
                jstat = jrun_exec_inactive_stmt(jx, pjtok);
            }
            if (jstat > 0) {
                jrun_error_recovery(jx, pjtok, &jstat);
            }
        }
    }

    return (jstat);
}
/***************************************************************/
