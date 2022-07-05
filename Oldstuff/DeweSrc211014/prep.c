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
#include "jstok.h"
#include "dbtreeh.h"
#include "snprfh.h"
#include "prep.h"
#include "var.h"
#include "jsexp.h"

/***************************************************************/
int jprep_skip_generic_statement(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok,
                    int skip_flags)
{
/*
** 02/26/2021
**
** See also: jrun_exec_generic_inactive_stmt()
*/
    int jstat = 0;
    int done;
    int braces;

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
            if (!braces) {
                done = 1;
                if (!jstat) jstat = jrun_next_token(jx, pjtok);
            }
        }
    }

    return (jstat);
}
/***************************************************************/
static int jprep_if_stmt(
                    struct jrunexec * jx,
                    struct jvarvalue_function * jvvf,
                    struct jtoken ** pjtok,
                    int skip_flags)
{
/*
** 02/26/2021
*/
    int jstat = 0;

    if ((*pjtok)->jtok_kw != JSPU_OPEN_PAREN) {
        jstat = jrun_set_error(jx, JSERROBJ_SCRIPT_SYNTAX_ERROR,
            "Expecting open parenthesis. Found \'%s\'", (*pjtok)->jtok_text);
    } else {
        jstat = jrun_next_token(jx, pjtok);
        if (!jstat) {
            jstat = jexp_evaluate_inactive(jx, pjtok);
            if (!jstat) {
                if ((*pjtok)->jtok_kw != JSPU_CLOSE_PAREN) {
                    jstat = jrun_set_error(jx, JSERROBJ_SCRIPT_SYNTAX_ERROR,
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
static int jprep_function_stmt(
                    struct jrunexec * jx,
                    struct jvarvalue_function * jvvf_parent,
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
    jstat = jvar_parse_function(jx, pjtok, &fjvv);
    if (!jstat) {
        jcxflags = JCX_FLAG_FUNCTION;
        jstat = jvar_new_local_function(jx, jvvf_parent, &fjvv, jcxflags);
    }
    if (jstat) jvar_free_jvarvalue_data(&fjvv);

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
                    jstat = jexp_eval_assignment_inactive(jx, pjtok);
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
int jprep_preprocess_block(
                    struct jrunexec * jx,
                    struct jvarvalue_function * jvvf,
                    struct jtoken ** pjtok,
                    int skip_flags)
{
/*
** 02/14/2021
**
** See also: jrun_exec_generic_inactive_stmt()
**
*/
    int jstat = 0;
    int done;
    int braces;

    done = 0;
    braces = 0;
    while (!jstat && !done) {
    
        switch ((*pjtok)->jtok_kw) {
            case JSPU_CLOSE_BRACE:
                braces--;
                if (!braces && !(skip_flags & SKIP_FLAG_UNTIL_EOF)) {
                    done = 1;
                }
                jstat = jrun_next_token(jx, pjtok);
                break;

            case JSPU_OPEN_BRACE:
                braces++;
                jstat = jrun_next_token(jx, pjtok);
                break;

            case JSKW_FUNCTION:
                jstat = jrun_next_token(jx, pjtok);
                if (!jstat) jstat = jprep_function_stmt(jx, jvvf, pjtok, skip_flags);
                break;

            case JSKW_IF:
                jstat = jrun_next_token(jx, pjtok);
                if (!jstat) jstat = jprep_if_stmt(jx, jvvf, pjtok, skip_flags);
                break;

            case JSKW_VAR:
                jstat = jrun_next_token(jx, pjtok);
                if (!jstat) jstat = jprep_var_stmt(jx, jvvf, pjtok, skip_flags);
                break;

            default:
                jstat = jprep_skip_generic_statement(jx, pjtok, skip_flags);
                break;
        }
    }

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
    jvvf->jvvf_flags |= JVVF_FLAG_RUN_TO_EOF;
    INCFUNCREFS(jvvf);

    /* jrun_get_current_jxc(jx, &(jvvf->jvvf_begin_jxc)); */
    jrun_copy_current_jxc(jx, &(jvvf->jvvf_begin_jxc));

    jstat = jrun_next_token(jx, &jtok);
    if (jstat) return (jstat);

    jstat = jprep_preprocess_block(jx, jvvf, &jtok, SKIP_FLAG_HOIST | SKIP_FLAG_UNTIL_EOF);
    if (jstat == JERR_EOF && (jvvf->jvvf_flags & JVVF_FLAG_RUN_TO_EOF)) {
        jstat = 0;
    }

    if (!jstat) {
        (*pjvvf) = jvvf;
    } else {
        jvar_free_jvarvalue_function(jvvf);
    }

    return (jstat);
}
/***************************************************************/
