/**************************************************************/
/* demicp.c                                                   */
/**************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdarg.h>
#include <errno.h>

#include "types.h"
#include "decas.h"
#include "demi.h"
#include "demicp.h"
#include "dbtreeh.h"
#include "util.h"

#define DEBUG_PRINT_CPU     1

#define DEMI_TEST_INPUT_RECORD  2
#define DEMI_TEST_OUTPUT_RECORD 1

/**************************************************************/
struct dstrec { /* dst_ */
    char * dst_pbuf;
    int dst_pbuflen;
    int dst_pbufmax;
    int dst_pllen;
    int dst_pflags;
    FILE * dst_file;
};
struct cpurec { /* cpu_ */
    int cpu_state;
    int cpu_pc;
    struct demiprog * cpu_demp;
    pint * cpu_stack;
    int cpu_tos;
    int cpu_stack_size;
    int cpu_flags;
    struct demimemory * cpu_tgtdemmem;
    uchar * cpu_srcptr;
    uchar * cpu_tgtptr;
    uchar * cpu_headsrcptr;
    int cpu_srclen;
    int cpu_tgtix;
    int cpu_dbg_pc;
#if DEMI_TESTING
    int cpu_nzones;
    int cpu_xzones;
    int * cpu_zonesz;
    uchar ** cpu_zoneptr;
#endif
};
#define DEMICPU_INITIAL_STACK    8
#define DEMICPU_STACK_INCREMENT  8

#define CPU_STATE_RUNNING           0
#define CPU_STATE_END0              1
#define CPU_STATE_END1              2
#define CPU_STATE_OVERRUN           3
#define CPU_STATE_INSTRUCTION       4
#define CPU_STATE_STACK_UNDERFLOW   5

#define CPU_ADDS(c,n) \
{ if ((c)->cpu_tos + (n) > (c)->cpu_stack_size) { \
(c)->cpu_stack_size = (c)->cpu_tos + (n) + DEMICPU_STACK_INCREMENT; \
(c)->cpu_stack = Realloc((c)->cpu_stack, pint, (c)->cpu_stack_size);}(c)->cpu_tos += (n);}
#define CPU_SUBS(c,n) \
{ (c)->cpu_tos -= (n); if ((c)->cpu_tos < 0) (c)->cpu_tos = CPU_STATE_STACK_UNDERFLOW; }

#define CPU_TOS(c) ((c)->cpu_stack[(c)->cpu_tos-1])
#define CPU_STACKB(c) ((c)->cpu_stack[(c)->cpu_tos-2])
#define CPU_STACKC(c) ((c)->cpu_stack[(c)->cpu_tos-3])

static void sdemi_init_dstrec(struct dstrec * dst, int pflags);
static void sdemi_free_dstrec_data(struct dstrec * dst);
static void sdemi_append_nl(struct dstrec * dst);

static int demi_string_demityp_data(
        struct demirec * demi,
        struct dstrec * dst,
        const uchar * udptr,
        struct demityp * demt,
        int pdepth);

#define DEMIPROG_CPFLAG_UNPACK      1

int demi_emit_copy_demityp(struct demirec * demi,
    struct demiprog * demp,
    struct demityp * demt,
    int cpflags);

#if DEMI_TEST_INPUT_RECORD == 1
struct demi_test_function_rec {
    int    dtf1_num1;
    card_t dtf1_card[4];
};
struct demi_test_function_ARGS {
    int    dtf0_num;
    struct demi_test_function_rec * dtf0_rec1;
    struct demi_test_function_rec * dtf0_rec2;
    struct demi_test_function_rec * dtf0_rec3;
    card_t dtf0_card[4];
};
#endif
#if DEMI_TEST_INPUT_RECORD == 2
struct demi_test_function_ARGS {
    int      dtf0_num1;
    card_t * dtf0_cardlist1;
    int      dtf0_num2;
};
#endif

#if DEMI_TEST_OUTPUT_RECORD == 1
struct demi_test_function_OUT {
    int opiota;
};
#endif
/**************************************************************/

/* 4-bit instructions with 10-bit operand */
/* 0 0 0 0 0 0 0 0 0 0 1 1 1 1 1 1 */
/* 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 */
/* 0 0 i i i i n n n n n n n n n n */
#define DEMINST_PUSHI           0x0400
#define DEMINST_PUSHI_HI        0x0800
//#define DEMINST_STORSRC_IX      0x0C00
//#define DEMINST_STORTGT_IX      0x1000
#define DEMINST_COPY_TO_IX      0x1400
#define DEMINST_COPY_TO_IX_HI   0x1800
#define DEMINST_COPY_TO_PTR     0x1C00
#define DEMINST_COPY_TO_PTR_HI  0x2000
#define DEMINST_ADDI            0x2400
#define DEMINST_ADDI_HI         0x2800
#define DEMINST_ADDTGTLENI      0x2C00
#define DEMINST_ADDTGTLENI_HI   0x3000

#define DEMINST_IBITS           10
#define DEMINST_IBITS_POW2      1024    /* 2^DEMINST_IBITS */
#define DEMINST_IBITS_MASK      (DEMINST_IBITS_POW2 - 1)    /* 1023 = 0x3FF */

#define DEMSINST_IBITS          6
#define DEMSINST_IBITS_POW2     64    /* 2^DEMINST_IBITS */
#define DEMSINST_IBITS_MASK     (DEMSINST_IBITS_POW2 - 1)    /* 63 = 0x3F */

/* 6-bit instructions with 2 4-bit operands */
/* 0 0 0 0 0 0 0 0 0 0 1 1 1 1 1 1 */
/* 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 */
/* 0 1 i i i i i i n n n n n n n n */
#define DEMINST_DEBUG          0x4100

/* Short 6-bit instructions */
/* 0 0 0 0 0 0 0 0 0 0 1 1 1 1 1 1 */
/* 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 */
/* 1 0 0 0 a a a a a a b b b b b b */
#define DEMSINST_MASK               0x8000
#define DEMSINST_NOOP               0x00
//#define DEMSINST_ZEROSRC_IX         0x01
#define DEMSINST_ZEROTGT_IX         0x02
#define DEMSINST_PUSHTGT_IX         0x03
#define DEMSINST_STORTGT_IX         0x04
#define DEMSINST_STORA_TGT_IX       0x05
#define DEMSINST_STORSRC_PTR        0x06
#define DEMSINST_STORTGT_PTR        0x07
#define DEMSINST_XCHGAB             0x08
#define DEMSINST_DUPA               0x09
#define DEMSINST_PUSHSRC_PTR        0x0A
#define DEMSINST_PUSHTGT_PTR        0x0B
#define DEMSINST_DEREFERENCE        0x0C
#define DEMSINST_ADD_VOID_STAR      0x0D
#define DEMSINST_END0               0x0E
#define DEMSINST_END1               0x0F
#define DEMSINST_PUSHAT_SRC_PTR     0x10
#define DEMSINST_STOR_IX_SRC_PTR    0x11
#define DEMSINST_STOR_IX_TGT_PTR    0x12
#define DEMSINST_ROTATE_CAB         0x13
#define DEMSINST_NEW                0x14
#define DEMSINST_STORAT_TGT_PTR     0x15
#define DEMSINST_ADDTGTLEN          0x16
#define DEMSINST_PUSH_STRLEN_SRC1   0x17
#define DEMSINST_PUSH_STRLEN_SRC2   0x18
#define DEMSINST_PUSH_STRLEN_SRC4   0x19
#define DEMSINST_PUSH_STRLEN_SRC8   0x1A
#define DEMSINST_STRCPY1            0x1B
#define DEMSINST_STRCPY2            0x1C
#define DEMSINST_STRCPY4            0x1D
#define DEMSINST_STRCPY8            0x1E
#define DEMSINST_ADD                0x1F
#define DEMSINST_SUB                0x20
#define DEMSINST_MEMCPY_SRC_TGTIX   0x21
/*      DEMSINST_LAST_SINST         0x3F */

/***************************************************************/
static void sdemi_append_f(struct dstrec * dst, const char * fmt, ...)
{
/*
**  01/16/2020
*/

	va_list args;
    int ii;
    char msgbuf[256];

	va_start(args, fmt);
	vsprintf(msgbuf, fmt, args);
	va_end(args);

    appendbuf_n(&(dst->dst_pbuf), &(dst->dst_pbuflen), &(dst->dst_pbufmax),
        msgbuf, (int)strlen(msgbuf));
    for (ii = 0;  msgbuf[ii]; ii++) {
        if (msgbuf[ii] == '\n') dst->dst_pllen = 0;
        else dst->dst_pllen += 1;
    }
}
/***************************************************************/
int sdemi_print_demiinst(struct demirec * demi,
    DINST inst,
    struct dstrec * dst)
{
/*
**  01/16/2020
*/
    int dstat;
    int iop;
    int iix;
    char fmt[32];


    dstat = 0;
    switch (inst >> 14) {
        case 0:
            strcpy(fmt, "%-12s %d");
            switch (inst >> DEMINST_IBITS) {
                case 0:
                    break;
                case (DEMINST_PUSHI >> DEMINST_IBITS) :
                    sdemi_append_f(dst, fmt, "PUSHI", inst & DEMINST_IBITS_MASK);
                    break;
                case (DEMINST_PUSHI_HI >> DEMINST_IBITS) :
                    sdemi_append_f(dst, fmt, "PUSHI_HI", inst & DEMINST_IBITS_MASK);
                    break;
                case (DEMINST_COPY_TO_IX >> DEMINST_IBITS) :
                    sdemi_append_f(dst, fmt, "COPY_TO_IX", inst & DEMINST_IBITS_MASK);
                    break;
                case (DEMINST_COPY_TO_IX_HI >> DEMINST_IBITS) :
                    sdemi_append_f(dst, fmt, "COPY_TO_IX_HI", inst & DEMINST_IBITS_MASK);
                    break;
                case (DEMINST_COPY_TO_PTR >> DEMINST_IBITS) :
                    sdemi_append_f(dst, fmt, "COPY_TO_PTR", inst & DEMINST_IBITS_MASK);
                    break;
                case (DEMINST_COPY_TO_PTR_HI >> DEMINST_IBITS) :
                    sdemi_append_f(dst, fmt, "COPY_TO_PTR_HI", inst & DEMINST_IBITS_MASK);
                    break;
                case (DEMINST_ADDI >> DEMINST_IBITS) :
                    sdemi_append_f(dst, fmt, "ADDI", inst & DEMINST_IBITS_MASK);
                    break;
                case (DEMINST_ADDI_HI >> DEMINST_IBITS) :
                    sdemi_append_f(dst, fmt, "ADDI_HI", inst & DEMINST_IBITS_MASK);
                    break;
                case (DEMINST_ADDTGTLENI >> DEMINST_IBITS) :
                    sdemi_append_f(dst, fmt, "ADDTGTLENI", inst & DEMINST_IBITS_MASK);
                    break;
                case (DEMINST_ADDTGTLENI_HI >> DEMINST_IBITS) :
                    sdemi_append_f(dst, fmt, "ADDTGTLENI_HI", inst & DEMINST_IBITS_MASK);
                    break;

                default:
                    dstat = demi_set_error(demi, DEMIERR_UNIMPLEMENTED_INST,
                            "Unimplemented instruction for category 0: %x", inst);
                    break;
            }
            break;

        case 1:
            strcpy(fmt, "%-12s %d %d");
            switch (inst >> 8) {
                case 0:
                    break;

                case (DEMINST_DEBUG >> 8) :
                    sdemi_append_f(dst, fmt, "DEBUG", (inst & 0xF0) >> 4, inst & 0x0F);
                    break;

                default:
                    dstat = demi_set_error(demi, DEMIERR_UNIMPLEMENTED_INST,
                        "Unimplemented instruction category: %d", inst >> 14);
                    break;
            }
            break;

        case 2:
            strcpy(fmt, "%-12s");
            for (iix = 0; iix <= 1; iix++) {
                if (!iix) iop = (inst >> DEMSINST_IBITS) & DEMSINST_IBITS_MASK;
                else      iop = inst & DEMSINST_IBITS_MASK;
                switch (iop) {
                    case DEMSINST_NOOP:
                        sdemi_append_f(dst, fmt, "NOOP");
                        break;
                    case DEMSINST_ZEROTGT_IX:
                        sdemi_append_f(dst, fmt, "ZEROTGT_IX");
                        break;
                    case DEMSINST_PUSHTGT_IX:
                        sdemi_append_f(dst, fmt, "PUSHTGT_IX");
                        break;
                    case DEMSINST_STORTGT_IX:
                        sdemi_append_f(dst, fmt, "STORTGT_IX");
                        break;
                    case DEMSINST_STORA_TGT_IX:
                        sdemi_append_f(dst, fmt, "STORA_TGT_IX");
                        break;
                    case DEMSINST_STORSRC_PTR:
                        sdemi_append_f(dst, fmt, "STORSRC_PTR");
                        break;
                    case DEMSINST_STORTGT_PTR:
                        sdemi_append_f(dst, fmt, "STORTGT_PTR");
                        break;
                    case DEMSINST_XCHGAB:
                        sdemi_append_f(dst, fmt, "XCHGAB");
                        break;
                    case DEMSINST_DUPA:
                        sdemi_append_f(dst, fmt, "DUPA");
                        break;
                    case DEMSINST_PUSHSRC_PTR:
                        sdemi_append_f(dst, fmt, "PUSHSRC_PTR");
                        break;
                    case DEMSINST_PUSHTGT_PTR:
                        sdemi_append_f(dst, fmt, "PUSHTGT_PTR");
                        break;
                    case DEMSINST_DEREFERENCE:
                        sdemi_append_f(dst, fmt, "DEREFERENCE");
                        break;
                    case DEMSINST_ADD_VOID_STAR:
                        sdemi_append_f(dst, fmt, "ADD_VOID_STAR");
                        break;
                    case DEMSINST_END0:
                        sdemi_append_f(dst, fmt, "END0");
                        break;
                    case DEMSINST_END1:
                        sdemi_append_f(dst, fmt, "END1");
                        break;
                    case DEMSINST_PUSHAT_SRC_PTR:
                        sdemi_append_f(dst, fmt, "PUSHAT_SRC_PTR");
                        break;
                    case DEMSINST_STOR_IX_SRC_PTR:
                        sdemi_append_f(dst, fmt, "STOR_IX_SRC_PTR");
                        break;
                    case DEMSINST_STOR_IX_TGT_PTR:
                        sdemi_append_f(dst, fmt, "STOR_IX_TGT_PTR");
                        break;
                    case DEMSINST_ROTATE_CAB:
                        sdemi_append_f(dst, fmt, "ROTATE_CAB");
                        break;
                    case DEMSINST_NEW:
                        sdemi_append_f(dst, fmt, "NEW");
                        break;
                    case DEMSINST_STORAT_TGT_PTR:
                        sdemi_append_f(dst, fmt, "STORAT_TGT_PTR");
                        break;
                    case DEMSINST_ADDTGTLEN:
                        sdemi_append_f(dst, fmt, "ADDTGTLEN");
                        break;
                    case DEMSINST_PUSH_STRLEN_SRC1:
                        sdemi_append_f(dst, fmt, "PUSH_STRLEN_SRC1");
                        break;
                    case DEMSINST_PUSH_STRLEN_SRC2:
                        sdemi_append_f(dst, fmt, "PUSH_STRLEN_SRC2");
                        break;
                    case DEMSINST_PUSH_STRLEN_SRC4:
                        sdemi_append_f(dst, fmt, "PUSH_STRLEN_SRC4");
                        break;
                    case DEMSINST_PUSH_STRLEN_SRC8:
                        sdemi_append_f(dst, fmt, "PUSH_STRLEN_SRC8");
                        break;
                    case DEMSINST_STRCPY1:
                        sdemi_append_f(dst, fmt, "STRCPY1");
                        break;
                    case DEMSINST_STRCPY2:
                        sdemi_append_f(dst, fmt, "STRCPY2");
                        break;
                    case DEMSINST_STRCPY4:
                        sdemi_append_f(dst, fmt, "STRCPY4");
                        break;
                    case DEMSINST_STRCPY8:
                        sdemi_append_f(dst, fmt, "STRCPY8");
                        break;
                    case DEMSINST_ADD:
                        sdemi_append_f(dst, fmt, "ADD");
                        break;
                    case DEMSINST_SUB:
                        sdemi_append_f(dst, fmt, "SUB");
                        break;
                    case DEMSINST_MEMCPY_SRC_TGTIX:
                        sdemi_append_f(dst, fmt, "MEMCPY_SRC_TGTIX");
                        break;

                    default:
                        dstat = demi_set_error(demi, DEMIERR_UNIMPLEMENTED_INST,
                            "Unimplemented instruction for category 2: %x", inst);
                        break;
                }
                if (!iix) sdemi_append_f(dst, "%s", " ; ");
            }
            break;

        case 3:
            dstat = demi_set_error(demi, DEMIERR_UNIMPLEMENTED_INST,
                "Unimplemented instruction category: %d", inst >> 14);
            break;

        default:
            dstat = demi_set_error(demi, DEMIERR_UNIMPLEMENTED_INST,
                "Unimplemented instruction category: %d", inst >> 14);
            break;
    }

    return (dstat);
}
/***************************************************************/
int demi_print_demiprog(struct demirec * demi,
    struct demiprog * demp,
    const char * tdesc,
    const char * tnam,
    int pflags)
{
/*
** 01/16/2020
*/
    int dstat;
    int apflags;
    int ii;
    struct dstrec dst;

    dstat = 0;
    apflags = pflags;
    if (!apflags) {
        apflags = DEMIDAT_FLAG_FMT | DEMIDAT_FLAG_LINE_80;
    }

    sdemi_init_dstrec(&dst, apflags);
    sdemi_append_f(&dst, "%s %s length=%d\n", tdesc, tnam, demp->demp_ninst);

    for (ii = 0; !dstat && ii < demp->demp_ninst; ii++) {
        sdemi_append_f(&dst, "%4d. ", ii);
        dstat = sdemi_print_demiinst(demi, demp->demp_ainst[ii], &dst);
        sdemi_append_nl(&dst);
    }

    if (!dstat && dst.dst_pbuf) {
        fprintf(dst.dst_file, "%s\n", dst.dst_pbuf);
    }
    sdemi_free_dstrec_data(&dst);

    return (dstat);
}
/***************************************************************/
void demi_emit_inst(struct demirec * demi,
    struct demiprog * demp,
    int inst)
{
/*
** 01/16/2020
*/
    if (demp->demp_ninst == demp->demp_xinst) {
        if (!demp->demp_xinst) demp->demp_xinst = 8;
        else demp->demp_xinst *= 2;
        demp->demp_ainst = Realloc(demp->demp_ainst, DINST, demp->demp_xinst);
    }
    demp->demp_ainst[demp->demp_ninst] = inst;
    demp->demp_ninst += 1;
}
/***************************************************************/
void demi_emit_short(struct demirec * demi,
    struct demiprog * demp,
    int shortop1,
    int shortop2)
{
/*
** 01/15/2020
*/
    int inst;

    inst = DEMSINST_MASK | (shortop1 << 6) | shortop2;
    demi_emit_inst(demi, demp, inst);
}
/***************************************************************/
void demi_emit_nibits(struct demirec * demi,
    struct demiprog * demp,
    int iop,
    int num)
{
/*
** 01/16/2020
*/
    int inst;

    inst = iop | (num & DEMINST_IBITS_MASK);
    demi_emit_inst(demi, demp, inst);
}
/***************************************************************/
void demi_emit_2op(struct demirec * demi,
    struct demiprog * demp,
    int iop,
    int num1,
    int num2)
{
/*
** 01/26/2020
*/
    int inst;

    inst = iop | ((num1 & 0x0F) << 4) | (num2 & 0x0F);
    demi_emit_inst(demi, demp, inst);
}
/***************************************************************/
void demi_emit_cpbytes(struct demirec * demi,
    struct demiprog * demp,
    int cpflags,
    int nbytes)
{
/*
** 01/16/2020
*/
    int instlo;
    int insthi;

    if (cpflags & DEMIPROG_CPFLAG_UNPACK) {
        insthi = DEMINST_COPY_TO_PTR_HI;
        instlo = DEMINST_COPY_TO_PTR;
    } else {
        insthi = DEMINST_COPY_TO_IX_HI;
        instlo = DEMINST_COPY_TO_IX;
    }

    if (nbytes >= DEMINST_IBITS_POW2) {
        if ((nbytes >> DEMINST_IBITS) < DEMINST_IBITS_POW2) {
            demi_emit_nibits(demi, demp, insthi, (nbytes >> DEMINST_IBITS));
        }
    }
    demi_emit_nibits(demi, demp, instlo, (nbytes & DEMINST_IBITS_MASK));
}
/***************************************************************/
void demi_emit_ldnum(struct demirec * demi,
    struct demiprog * demp,
    int nbytes)
{
/*
** 01/16/2020
*/
    if (nbytes >= DEMINST_IBITS_POW2) {
        if ((nbytes >> DEMINST_IBITS) < DEMINST_IBITS_POW2) {
            demi_emit_nibits(demi, demp, DEMINST_PUSHI_HI, (nbytes >> DEMINST_IBITS));
        }
    }
    demi_emit_nibits(demi, demp, DEMINST_PUSHI, (nbytes & DEMINST_IBITS_MASK));
}
/***************************************************************/
void demi_emit_addbytes(struct demirec * demi,
    struct demiprog * demp,
    int nbytes)
{
/*
** 01/17/2020
*/
    if (nbytes >= DEMINST_IBITS_POW2) {
        if ((nbytes >> DEMINST_IBITS) < DEMINST_IBITS_POW2) {
            demi_emit_nibits(demi, demp, DEMINST_ADDI_HI, (nbytes >> DEMINST_IBITS));
        }
    }
    demi_emit_nibits(demi, demp, DEMINST_ADDI, (nbytes & DEMINST_IBITS_MASK));
}
/***************************************************************/
void demi_emit_addtgtlen(struct demirec * demi,
    struct demiprog * demp,
    int nbytes)
{
/*
** 01/27/2020
*/
    if (nbytes >= DEMINST_IBITS_POW2) {
        if ((nbytes >> DEMINST_IBITS) < DEMINST_IBITS_POW2) {
            demi_emit_nibits(demi, demp, DEMINST_ADDTGTLENI_HI, (nbytes >> DEMINST_IBITS));
        }
    }
    demi_emit_nibits(demi, demp, DEMINST_ADDTGTLENI, (nbytes & DEMINST_IBITS_MASK));
}
/***************************************************************/
int demi_emit_copy_struct(struct demirec * demi,
    struct demiprog * demp,
    struct demityp_struct * demts,
    int cpflags)
{
/*
** 01/16/2020
*/
    int dstat;
    int ii;
    struct demifield * demf;

    dstat = 0;

    for (ii = 0; !dstat && ii < demts->demts_num_fields; ii++) {
        demf = demts->demts_fields[ii];
        dstat = demi_emit_copy_demityp(demi, demp, demf->demf_dmityp, cpflags);
    }

    return (dstat);
}
/***************************************************************/
int demi_emit_copy_array(struct demirec * demi,
    struct demiprog * demp,
    struct demityp_array * demta,
    int cpflags)
{
/*
** 01/16/2020
*/
    int dstat;

    dstat = 0;
    switch (demta->demta_array_type) {
        case DEMTA_ARRAY_TYPE_FIXED_SIZE:
            demi_emit_cpbytes(demi, demp, cpflags, demta->demta_total_size);
            break;

        default:
            dstat = demi_set_error(demi, DEMIERR_UNSUPPORTED_TYPE,
                "Usupported copy array type. Found: %d", demta->demta_array_type);
            break;
    }

    return (dstat);
}
/***************************************************************/
int demi_emit_pack_ptr(struct demirec * demi,
    struct demiprog * demp,
    struct demityp_ptr * demtp,
    int cpflags)
{
/*
** 01/16/2020
*/
    int dstat;
    int inst;

    dstat = 0;
    switch (demtp->demtp_ptr_type) {
        case DEMTP_PTR_TYPE_SINGLE_ITEM:
            /*
            ** Definitions:
            **  TgtIx is the index of the current target.
            **  @TgtIx is the pointer to the the current target.
            **  TgtIx++ is TgtIx + sizeof(void*)
            **  SrcPtr is the current source pointer
            **  @SrcPtr are the bytes pointed to by source pointer
            **  NextIx is target index of the next available area (In top of stack)
            **  @NextIx is the pointer to the next available area
            **  NextIx++ is next target index of the next available area
            **  n is the number of bytes to be copied
            **
            ** Goal before copy:
            **  TgtIx=new area index from tos
            **  SrcPtr=pointer to data to be copied 
            **  Stack=prev SrcPtr, prev TgtPtr, index of next area in target (NextTgtIx)
            **
            ** Goal after copy:
            **  TgtPtr=prev TgtPtr + sizeof(void*) 
            **  SrcPtr=prev SrcPtr + sizeof(void*) 
            **  Stack=index of next area in target (NextTgtIx)
            **
            ** Method:
            **  0. Stack: NextIx
            **  1. Push SrcPtr. Stack: NextTgtIx, OldSrcPtr
            **  2. Push TgtIx. Stack: NextTgtIx, OldSrcPtr, OldTgtIx
            **  3. Increment TgtLen by n. Stack: NextIx
            **  4. Rotate CAB. Stack: OldSrcPtr, OldTgtIx, NextTgtIx
            **  5. Duplicate A. Stack: OldSrcPtr, OldTgtIx, NextTgtIx, NextTgtIx
            **  6. Store @TgtIx. Stack: OldSrcPtr, OldTgtIx, NextTgtIx
            **  7. Duplicate A. Stack: OldSrcPtr, OldTgtIx, NextTgtIx, NextTgtIx
            **  8. Store TgtIx. Stack: OldSrcPtr, OldTgtIx, NextTgtIx
            **  9. Load @SrcPtr. Stack: OldSrcPtr, OldTgtIx, NextTgtIx, NewSrcPtr
            ** 10. Store A to SrcPtr. Stack: OldSrcPtr, OldTgtIx, NextTgtIx
            ** 11. Add n to A. Stack: OldSrcPtr, OldTgtIx, NextTgtIx + n
            ** 12. Copy n bytes from SrcPtr to TgtIx. Stack: OldSrcPtr, OldTgtIx, NextTgtIx + n
            ** 13. Exchange AB. Stack: OldSrcPtr, NextTgtIx + n, OldTgtIx
            ** 14. Add sizeof(void*). Stack: OldSrcPtr, NextTgtIx + n, OldTgtIx + sizeof(void*)
            ** 15. Store TgtIx. Stack: OldSrcPtr, NextTgtIx + n
            ** 16. Exchange AB. Stack: NextTgtIx + n, OldSrcPtr
            ** 17. Add sizeof(void*). Stack: NextTgtIx + n, OldSrcPtr + sizeof(void*)
            ** 18. Store A to SrcPtr. Stack: NextTgtIx + n
            */

            /* Stack: NextIx */
//demi_emit_2op(demi, demp, DEMINST_DEBUG, 0, CPU_FLAG_PRINT_CPU);
            /* 1 */demi_emit_short(demi, demp, DEMSINST_PUSHSRC_PTR, DEMSINST_NOOP);
            /* 2 */demi_emit_short(demi, demp, DEMSINST_PUSHTGT_IX, DEMSINST_NOOP);
            /* 3 */demi_emit_addtgtlen(demi, demp, demtp->demtp_dmityp->demt_typ_size);
            /* 4 */demi_emit_short(demi, demp, DEMSINST_ROTATE_CAB, DEMSINST_NOOP);
            /* 5 */demi_emit_short(demi, demp, DEMSINST_DUPA, DEMSINST_NOOP);
            /* 6 */demi_emit_short(demi, demp, DEMSINST_STORA_TGT_IX, DEMSINST_NOOP);
            /* 7 */demi_emit_short(demi, demp, DEMSINST_DUPA, DEMSINST_NOOP);
            /* 8 */demi_emit_short(demi, demp, DEMSINST_STORTGT_IX, DEMSINST_NOOP);
            /* 9 */demi_emit_short(demi, demp, DEMSINST_PUSHAT_SRC_PTR, DEMSINST_NOOP);
            /* 10 */demi_emit_short(demi, demp, DEMSINST_STORSRC_PTR, DEMSINST_NOOP);
            /* 11 */demi_emit_addbytes(demi, demp, demtp->demtp_dmityp->demt_typ_size);
            /* 12 */dstat = demi_emit_copy_demityp(demi, demp, demtp->demtp_dmityp, cpflags);
            /* 13 */demi_emit_short(demi, demp, DEMSINST_XCHGAB, DEMSINST_NOOP);
            /* 14 */demi_emit_short(demi, demp, DEMSINST_ADD_VOID_STAR, DEMSINST_NOOP);
            /* 15 */demi_emit_short(demi, demp, DEMSINST_STORTGT_IX, DEMSINST_NOOP);
            /* 16 */demi_emit_short(demi, demp, DEMSINST_XCHGAB, DEMSINST_NOOP);
            /* 17 */demi_emit_short(demi, demp, DEMSINST_ADD_VOID_STAR, DEMSINST_NOOP);
            /* 18 */demi_emit_short(demi, demp, DEMSINST_STORSRC_PTR, DEMSINST_NOOP);
            break;

        case DEMTP_PTR_TYPE_NULL_TERM:
            /*
            ** Definitions:
            **  TgtIx is the index of the current target.
            **  @TgtIx is the pointer to the the current target.
            **  TgtIx++ is TgtIx + sizeof(void*)
            **  SrcPtr is the current source pointer
            **  @SrcPtr are the bytes pointed to by source pointer
            **  NextIx is target index of the next available area (In top of stack)
            **  @NextIx is the pointer to the next available area
            **  NextIx++ is next target index of the next available area
            **  n is the number of bytes to be copied
            **
            ** Goal before copy:
            **  TgtIx=new area index from tos
            **  SrcPtr=pointer to data to be copied 
            **  Stack=prev SrcPtr, prev TgtPtr, index of next area in target (NextTgtIx)
            **
            ** Goal after copy:
            **  TgtPtr=prev TgtPtr + sizeof(void*) 
            **  SrcPtr=prev SrcPtr + sizeof(void*) 
            **  Stack=index of next area in target (NextTgtIx)
            **
            ** Method:
            **  0. Stack: NextTgtIx
            **  1. Push SrcPtr. Stack: NextTgtIx, OldSrcPtr
            **  2. Push TgtIx. Stack: NextTgtIx, OldSrcPtr, OldTgtIx
            **  3. Rotate CAB. Stack: OldSrcPtr, OldTgtIx, NextTgtIx
            **  4. Duplicate A. Stack: OldSrcPtr, OldTgtIx, NextTgtIx, NextTgtIx
            **  5. Store @TgtIx. Stack: OldSrcPtr, OldTgtIx, NextTgtIx
            **  6. Push number of bytes to move. Stack: OldSrcPtr, OldTgtIx, NextTgtIx, ALen
            **  7. Duplicate A. Stack: OldSrcPtr, OldTgtIx, NextTgtIx, ALen, ALen
            **  8. Rotate CAB. Stack: OldSrcPtr, OldTgtIx, ALen, ALen, NextTgtIx
            **  9. Duplicate A. Stack: OldSrcPtr, OldTgtIx, ALen, ALen, NextTgtIx, NextTgtIx
            ** 10. Store TgtIx. Stack: OldSrcPtr, OldTgtIx, ALen, ALen, NextTgtIx
            ** 11. Load @SrcPtr. Stack: OldSrcPtr, OldTgtIx, ALen, ALen, NextTgtIx, NewSrcPtr
            ** 12. Store A to SrcPtr. Stack: OldSrcPtr, OldTgtIx, ALen, ALen, NextTgtIx
            ** 13. Add A to B. Stack: OldSrcPtr, OldTgtIx, ALen, ALen + NextTgtIx
            ** 14. Exchange AB. Stack: OldSrcPtr, OldTgtIx, ALen + NextTgtIx, ALen
            ** 15. Duplicate A. Stack: OldSrcPtr, OldTgtIx, ALen + NextTgtIx, ALen, ALen
            ** 16. Increment TgtLen by A. Stack: OldSrcPtr, OldTgtIx, ALen + NextTgtIx, ALen
            ** 17. Copy A bytes from SrcPtr to TgtIx. Stack: OldSrcPtr, OldTgtIx, ALen + NextTgtIx
            ** 18. Exchange AB. Stack: OldSrcPtr, ALen + NextTgtIx, OldTgtIx
            ** 19. Add sizeof(void*). Stack: OldSrcPtr, ALen + NextTgtIx, OldTgtIx + sizeof(void*)
            ** 20. Store TgtIx. Stack: OldSrcPtr, ALen + NextTgtIx
            ** 21. Exchange AB. Stack: ALen + NextTgtIx, OldSrcPtr
            ** 22. Add sizeof(void*). Stack: ALen + NextTgtIx, OldSrcPtr + sizeof(void*)
            ** 23. Store A to SrcPtr. Stack: ALen + NextTgtIx
            */

            /* Stack: NextIx */
            switch (demtp->demtp_dmityp->demt_typ_size) {
                case 1: inst = DEMSINST_PUSH_STRLEN_SRC1; break;
                case 2: inst = DEMSINST_PUSH_STRLEN_SRC2; break;
                case 4: inst = DEMSINST_PUSH_STRLEN_SRC4; break;
                case 8: inst = DEMSINST_PUSH_STRLEN_SRC8; break;
                default:
                    dstat = demi_set_error(demi, DEMIERR_UNSUPPORTED_TYPE,
                        "Zero terminated array of size %d is unsupported.",
                        demtp->demtp_dmityp->demt_typ_size);
                    return (dstat);
                    break;
            }
            /* 1 */demi_emit_short(demi, demp, DEMSINST_PUSHSRC_PTR, DEMSINST_NOOP);
            /* 2 */demi_emit_short(demi, demp, DEMSINST_PUSHTGT_IX, DEMSINST_NOOP);
            /* 3 */demi_emit_short(demi, demp, DEMSINST_ROTATE_CAB, DEMSINST_NOOP);
            /* 4 */demi_emit_short(demi, demp, DEMSINST_DUPA, DEMSINST_NOOP);
            /* 5 */demi_emit_short(demi, demp, DEMSINST_STORA_TGT_IX, DEMSINST_NOOP);
            /* 6 */demi_emit_short(demi, demp, inst, DEMSINST_NOOP);
            /* 7 */demi_emit_short(demi, demp, DEMSINST_DUPA, DEMSINST_NOOP);
            /* 8 */demi_emit_short(demi, demp, DEMSINST_ROTATE_CAB, DEMSINST_NOOP);
            /* 9 */demi_emit_short(demi, demp, DEMSINST_DUPA, DEMSINST_NOOP);
            /* 10 */demi_emit_short(demi, demp, DEMSINST_STORTGT_IX, DEMSINST_NOOP);
            /* 11*/demi_emit_short(demi, demp, DEMSINST_PUSHAT_SRC_PTR, DEMSINST_NOOP);
            /* 12 */demi_emit_short(demi, demp, DEMSINST_STORSRC_PTR, DEMSINST_NOOP);
            /* 13 */demi_emit_short(demi, demp, DEMSINST_ADD, DEMSINST_NOOP);
            /* 14 */demi_emit_short(demi, demp, DEMSINST_XCHGAB, DEMSINST_NOOP);
            /* 15 */demi_emit_short(demi, demp, DEMSINST_DUPA, DEMSINST_NOOP);
            /* 16 */demi_emit_short(demi, demp, DEMSINST_ADDTGTLEN, DEMSINST_NOOP);
            /* 17 */demi_emit_short(demi, demp, DEMSINST_MEMCPY_SRC_TGTIX, DEMSINST_NOOP);
            /* 18 */demi_emit_short(demi, demp, DEMSINST_XCHGAB, DEMSINST_NOOP);
            /* 19 */demi_emit_short(demi, demp, DEMSINST_ADD_VOID_STAR, DEMSINST_NOOP);
            /* 20 */demi_emit_short(demi, demp, DEMSINST_STORTGT_IX, DEMSINST_NOOP);
            /* 11 */demi_emit_short(demi, demp, DEMSINST_XCHGAB, DEMSINST_NOOP);
            /* 22 */demi_emit_short(demi, demp, DEMSINST_ADD_VOID_STAR, DEMSINST_NOOP);
            /* 23 */demi_emit_short(demi, demp, DEMSINST_STORSRC_PTR, DEMSINST_NOOP);
            break;

        case DEMTP_PTR_TYPE_VOID_PTR:
            dstat = demi_set_error(demi, DEMIERR_UNSUPPORTED_TYPE,
                "Copy of void pointer is unsupported.");
            break;

        default:
            dstat = demi_set_error(demi, DEMIERR_UNSUPPORTED_TYPE,
                "Copy of pointer of type %d is unsupported.", demtp->demtp_ptr_type);
            break;
    }

    return (dstat);
}
/***************************************************************/
int demi_emit_unpack_ptr(struct demirec * demi,
    struct demiprog * demp,
    struct demityp_ptr * demtp,
    int cpflags)
{
/*
** 01/25/2020
*/
    int dstat;

    dstat = 0;

    switch (demtp->demtp_ptr_type) {
        case DEMTP_PTR_TYPE_SINGLE_ITEM:
            /*
            ** Definitions:
            **  HeadSrcPtr is the pointer to the source containing all data
            **  HeadSrcIx is the index within HeadSrcPtr of the source data
            **  n is the number of bytes to be copied
            **  NewSrcPtr is the source pointer to the data to be copied
            **  NewTgtPtr is the target pointer of the data destination
            **  NextTgtIx is the index to the next available target area
            **  OldSrcPtr is the original source pointer
            **  OldTgtPtr is the original target pointer
            **  SrcPtr is the current source pointer
            **  @SrcPtr are the bytes pointed to by SrcPtr.
            **  TgtPtr is the pointer to the current target.
================================================================
Input (0x9aec24,16) src=0x9aec34:
0000: 11 00 00 00  38 50 ee 00   f8 51 ee 00  41 62 63 00  .... 8P.. .Q.. Abc.
Output ix=16, len=32:
0000: 11 00 00 00  10 00 00 00   18 00 00 00  41 62 63 00  .... .... .... Abc.
0010: 22 00 00 00  44 65 66 00   33 00 00 00  48 69 6a 00  "... Def. 3... Hij.
================================================================
            **
            ** Goal before copy:
            **  TgtPtr=new area (new n)
            **  SrcPtr=pointer to data (HeadSrcPtr[@Srcptr])
            **  Stack=OldSrcPtr, OldTgtPtr, index of next area in target (NextTgtIx)
            **
            ** Goal after copy:
            **  TgtPtr=OldTgtPtr + sizeof(void*) 
            **  SrcPtr=OldSrcPtr + sizeof(void*) 
            **  Stack=index of next area in target (NextTgtIx)
            **
            ** Method:
            **  0. Stack: NextTgtIx
            **  1. Push SrcPtr. Stack: NextTgtIx, OldSrcPtr
            **  2. Push TgtPtr. Stack: NextTgtIx, OldSrcPtr, OldTgtPtr
            **  3. Push @SrcPtr. Stack: NextTgtIx, OldSrcPtr, OldTgtPtr, HeadSrcIx
            **  4. Add A to HeadSrcPtr, store in SrcPtr. Stack: NextTgtIx, OldSrcPtr, OldTgtPtr
            **  5. Push size of new area. Stack: NextTgtIx, OldSrcPtr, OldTgtPtr, n
            **  6. New area (ptr replaces size on tos). Stack: NextTgtIx, OldSrcPtr, OldTgtPtr, NewTgtPtr
            **  7. Duplicate A. Stack: NextTgtIx, OldSrcPtr, OldTgtPtr, NewTgtPtr, NewTgtPtr
            **  8. Store A to @TgtPtr. Stack: NextTgtIx, OldSrcPtr, OldTgtPtr, NewTgtPtr
            **  9. Store A to TgtPtr. Stack: NextTgtIx, OldSrcPtr, OldTgtPtr
            ** 10. Rotate CAB. Stack: OldSrcPtr, OldTgtPtr, NextTgtIx
            ** 11. ** Copy n bytes from @SrcPtr to @TgtPtr. Stack: OldSrcPtr, OldTgtPtr, NextTgtIx
            ** 12. Exchange AB. Stack: OldSrcPtr, NextTgtIx, OldTgtPtr
            ** 13. Add sizeof(void*). Stack: OldSrcPtr, NextTgtIx, OldTgtPtr + sizeof(void*)
            ** 14. Store TgtPtr. Stack: OldSrcPtr, NextTgtIx
            ** 15. Exchange AB. Stack: NextTgtIx, OldSrcPtr
            ** 16. Add sizeof(void*). Stack: NextTgtIx, OldSrcPtr + sizeof(void*)
            ** 17. Store SrcPtr. Stack: NextTgtIx
            */

            /* Stack: NextIx */
//demi_emit_2op(demi, demp, DEMINST_DEBUG, 0, CPU_FLAG_PRINT_CPU);
            /*  1 */demi_emit_short(demi, demp, DEMSINST_PUSHSRC_PTR, DEMSINST_NOOP);
            /*  2 */demi_emit_short(demi, demp, DEMSINST_PUSHTGT_PTR, DEMSINST_NOOP);
            /*  3 */demi_emit_short(demi, demp, DEMSINST_PUSHAT_SRC_PTR, DEMSINST_NOOP);
            /*  4 */demi_emit_short(demi, demp, DEMSINST_STOR_IX_SRC_PTR, DEMSINST_NOOP);
            /*  5 */demi_emit_ldnum(demi, demp, demtp->demtp_dmityp->demt_typ_size);
            /*  6 */demi_emit_short(demi, demp, DEMSINST_NEW, DEMSINST_NOOP);
            /*  7 */demi_emit_short(demi, demp, DEMSINST_DUPA, DEMSINST_NOOP);
            /*  8 */demi_emit_short(demi, demp, DEMSINST_STORAT_TGT_PTR, DEMSINST_NOOP);
            /*  9 */demi_emit_short(demi, demp, DEMSINST_STORTGT_PTR, DEMSINST_NOOP);
            /* 10 */demi_emit_short(demi, demp, DEMSINST_ROTATE_CAB, DEMSINST_NOOP);
            /* 11 */dstat = demi_emit_copy_demityp(demi, demp, demtp->demtp_dmityp, cpflags);
            /* 12 */demi_emit_short(demi, demp, DEMSINST_XCHGAB, DEMSINST_NOOP);
            /* 13 */demi_emit_short(demi, demp, DEMSINST_ADD_VOID_STAR, DEMSINST_NOOP);
            /* 14 */demi_emit_short(demi, demp, DEMSINST_STORTGT_PTR, DEMSINST_NOOP);
            /* 15 */demi_emit_short(demi, demp, DEMSINST_XCHGAB, DEMSINST_NOOP);
            /* 16 */demi_emit_short(demi, demp, DEMSINST_ADD_VOID_STAR, DEMSINST_NOOP);
            /* 17 */demi_emit_short(demi, demp, DEMSINST_STORSRC_PTR, DEMSINST_NOOP);
            break;

        case DEMTP_PTR_TYPE_NULL_TERM:
            /*
            ** Definitions:
            **  HeadSrcPtr is the pointer to the source containing all data
            **  HeadSrcIx is the index within HeadSrcPtr of the source data
            **  n is the number of bytes to be copied
            **  NewSrcPtr is the source pointer to the data to be copied
            **  NewTgtPtr is the target pointer of the data destination
            **  NextTgtIx is the index to the next available target area
            **  OldSrcPtr is the original source pointer
            **  OldTgtPtr is the original target pointer
            **  SrcPtr is the current source pointer
            **  @SrcPtr are the bytes pointed to by SrcPtr.
            **  TgtPtr is the pointer to the current target.
            **
            ** Goal before copy:
            **  TgtPtr=new area (new n)
            **  SrcPtr=pointer to data (HeadSrcPtr[@Srcptr])
            **  Stack=OldSrcPtr, OldTgtPtr, index of next area in target (NextTgtIx)
            **
            ** Goal after copy:
            **  TgtPtr=OldTgtPtr + sizeof(void*) 
            **  SrcPtr=OldSrcPtr + sizeof(void*) 
            **  Stack=index of next area in target (NextTgtIx)
            **
            ** Method:
            **  0. Stack: NextTgtIx
            **  1. Push SrcPtr. Stack: NextTgtIx, OldSrcPtr
            **  2. Push TgtPtr. Stack: NextTgtIx, OldSrcPtr, OldTgtPtr
            **  3. Push @SrcPtr. Stack: NextTgtIx, OldSrcPtr, OldTgtPtr, HeadSrcIx
            **  4. Add A to HeadSrcPtr, store in SrcPtr. Stack: NextTgtIx, OldSrcPtr, OldTgtPtr
            **  5. Push size of new area. Stack: NextTgtIx, OldSrcPtr, OldTgtPtr, n
            **  6. New area (ptr replaces size on tos). Stack: NextTgtIx, OldSrcPtr, OldTgtPtr, NewTgtPtr
            **  7. Duplicate A. Stack: NextTgtIx, OldSrcPtr, OldTgtPtr, NewTgtPtr, NewTgtPtr
            **  8. Store A to @TgtPtr. Stack: NextTgtIx, OldSrcPtr, OldTgtPtr, NewTgtPtr
            **  9. Store A to TgtPtr. Stack: NextTgtIx, OldSrcPtr, OldTgtPtr
            ** 10. Rotate CAB. Stack: OldSrcPtr, OldTgtPtr, NextTgtIx
            ** 11. ** Copy n bytes from @SrcPtr to @TgtPtr. Stack: OldSrcPtr, OldTgtPtr, NextTgtIx
            ** 12. Exchange AB. Stack: OldSrcPtr, NextTgtIx, OldTgtPtr
            ** 13. Add sizeof(void*). Stack: OldSrcPtr, NextTgtIx, OldTgtPtr + sizeof(void*)
            ** 14. Store TgtPtr. Stack: OldSrcPtr, NextTgtIx
            ** 15. Exchange AB. Stack: NextTgtIx, OldSrcPtr
            ** 16. Add sizeof(void*). Stack: NextTgtIx, OldSrcPtr + sizeof(void*)
            ** 17. Store SrcPtr. Stack: NextTgtIx
            */

            /* Stack: NextIx */
demi_emit_2op(demi, demp, DEMINST_DEBUG, 0, CPU_FLAG_PRINT_CPU);
            /*  1 */demi_emit_short(demi, demp, DEMSINST_PUSHSRC_PTR, DEMSINST_NOOP);
            /*  2 */demi_emit_short(demi, demp, DEMSINST_PUSHTGT_PTR, DEMSINST_NOOP);
            /*  3 */demi_emit_short(demi, demp, DEMSINST_PUSHAT_SRC_PTR, DEMSINST_NOOP);
            /*  4 */demi_emit_short(demi, demp, DEMSINST_STOR_IX_SRC_PTR, DEMSINST_NOOP);
            /*  5 */demi_emit_ldnum(demi, demp, demtp->demtp_dmityp->demt_typ_size);
            /*  6 */demi_emit_short(demi, demp, DEMSINST_NEW, DEMSINST_NOOP);
            /*  7 */demi_emit_short(demi, demp, DEMSINST_DUPA, DEMSINST_NOOP);
            /*  8 */demi_emit_short(demi, demp, DEMSINST_STORAT_TGT_PTR, DEMSINST_NOOP);
            /*  9 */demi_emit_short(demi, demp, DEMSINST_STORTGT_PTR, DEMSINST_NOOP);
            /* 10 */demi_emit_short(demi, demp, DEMSINST_ROTATE_CAB, DEMSINST_NOOP);
            /* 11 */dstat = demi_emit_copy_demityp(demi, demp, demtp->demtp_dmityp, cpflags);
            /* 12 */demi_emit_short(demi, demp, DEMSINST_XCHGAB, DEMSINST_NOOP);
            /* 13 */demi_emit_short(demi, demp, DEMSINST_ADD_VOID_STAR, DEMSINST_NOOP);
            /* 14 */demi_emit_short(demi, demp, DEMSINST_STORTGT_PTR, DEMSINST_NOOP);
            /* 15 */demi_emit_short(demi, demp, DEMSINST_XCHGAB, DEMSINST_NOOP);
            /* 16 */demi_emit_short(demi, demp, DEMSINST_ADD_VOID_STAR, DEMSINST_NOOP);
            /* 17 */demi_emit_short(demi, demp, DEMSINST_STORSRC_PTR, DEMSINST_NOOP);
            break;

        case DEMTP_PTR_TYPE_VOID_PTR:
            dstat = demi_set_error(demi, DEMIERR_UNSUPPORTED_TYPE,
                "Unpack of void pointer is unsupported.");
            break;

        default:
            dstat = demi_set_error(demi, DEMIERR_UNSUPPORTED_TYPE,
                "Unpack of pointer of type %d is unsupported.", demtp->demtp_ptr_type);
            break;
    }

    return (dstat);
}
/***************************************************************/
int demi_emit_copy_demityp(struct demirec * demi,
    struct demiprog * demp,
    struct demityp * demt,
    int cpflags)
{
/*
** 01/15/2020
*/
    int dstat;
    int uns;

    dstat = 0;
    uns   = 0;
    if (demt->demt_typ_flags & DEMT_TYP_FLAG_UNSIGNED) uns = 1;

    switch (demt->demt_simp_typ) {
        case DEMITYP_CHAR:
            demi_emit_cpbytes(demi, demp, cpflags, 1);
            break;
        case DEMITYP_INT8:
            demi_emit_cpbytes(demi, demp, cpflags, 1);
            break;
        case DEMITYP_INT16:
            demi_emit_cpbytes(demi, demp, cpflags, 2);
            break;
        case DEMITYP_INT32:
            demi_emit_cpbytes(demi, demp, cpflags, 4);
            break;
        case DEMITYP_INT64:
            demi_emit_cpbytes(demi, demp, cpflags, 8);
            break;
        case DEMITYP_FLOAT32:
            demi_emit_cpbytes(demi, demp, cpflags, 4);
            break;
        case DEMITYP_FLOAT64:
            demi_emit_cpbytes(demi, demp, cpflags, 8);
            break;
        case DEMITYP_VOID:
            break;
        case DEMITYP_STRUCT:
            dstat = demi_emit_copy_struct(demi, demp, demt->demt_u.demt_demts, cpflags);
            break;
        case DEMITYP_ARRAY:
            dstat = demi_emit_copy_array(demi, demp, demt->demt_u.demt_demta, cpflags);
            break;
        case DEMITYP_POINTER:
            if (cpflags & DEMIPROG_CPFLAG_UNPACK) {
                dstat = demi_emit_unpack_ptr(demi, demp, demt->demt_u.demt_demtp, cpflags);
            } else {
                dstat = demi_emit_pack_ptr(demi, demp, demt->demt_u.demt_demtp, cpflags);
            }
            break;

        default:
            dstat = demi_set_error(demi, DEMIERR_UNRECOGNIZED_TYPE,
                "Unrecognized type id. Found: '%d'", demt->demt_simp_typ);
            break;
    }

    return (dstat);
}
/***************************************************************/
int demi_gen_pack_prog(struct demirec * demi, struct deminamedtyp * demnt)
{
/*
** 01/13/2020
*/
    int dstat;
    struct demiprog *   demp_pack;
    struct demityp *    demt;

    dstat = 0;
    demp_pack = demnt->demnt_demp_pack;
    demt      = demnt->demnt_demt;

    demi_emit_addtgtlen(demi, demp_pack, demt->demt_typ_size);
    demi_emit_ldnum(demi, demp_pack, demt->demt_typ_size);

    dstat = demi_emit_copy_demityp(demi, demp_pack, demt, 0);

    if (!dstat) {
        demi_emit_short(demi, demp_pack, DEMSINST_END0, DEMSINST_NOOP);
    }

    if (!dstat) {
        dstat = demi_print_demiprog(demi, demp_pack, "Pack prog for", demnt->demnt_name, 0);
    }

    return (dstat);
}
/***************************************************************/
int demi_gen_unpack_prog(struct demirec * demi, struct deminamedtyp * demnt)
{
/*
** 01/13/2020
*/
    int dstat;
    struct demiprog *   demp_unpack;
    struct demityp *    demt;

    dstat = 0;
    demp_unpack = demnt->demnt_demp_unpack;
    demt        = demnt->demnt_demt;

    demi_emit_addtgtlen(demi, demp_unpack, demt->demt_typ_size);
    demi_emit_ldnum(demi, demp_unpack, demt->demt_typ_size);

    dstat = demi_emit_copy_demityp(demi, demp_unpack, demt, DEMIPROG_CPFLAG_UNPACK);

    if (!dstat) {
        demi_emit_short(demi, demp_unpack, DEMSINST_END0, DEMSINST_NOOP);
    }

    if (!dstat) {
        dstat = demi_print_demiprog(demi, demp_unpack, "Unpack prog for", demnt->demnt_name, 0);
    }

    return (dstat);
}
/**************************************************************/
struct demiprog * demi_new_demiprog(void)
{
/*
** 01/13/2020
*/
    struct demiprog * demp;

    demp = New(struct demiprog, 1);
    demp->demp_ninst    = 0; 
    demp->demp_xinst    = 0; 
    demp->demp_ainst    = NULL; 

    return (demp);
}
/***************************************************************/
void sdemi_free_demiprog(struct demiprog * demp)
{
/*
** 01/13/2020
*/
    if (!demp) return;

    Free(demp->demp_ainst);
    Free(demp);
}
/***************************************************************/
#if DEMI_TESTING
int demi_test_declare_funcs(struct demi_boss * demb,
    void ** vp_demi_test_function)
{
/*
** 01/04/2020
*/
    int estat = 0;
    int dstat;

    dstat = demb_define_func(demb,
        "demi_test_function",
        "demi_test_function_ARGS",
        "demi_test_function_OUT",
        vp_demi_test_function);

    return (estat);
}
/**************************************************************/
#if DEMI_TEST_INPUT_RECORD == 1
static void init_demi_test_function_rec(struct demi_test_function_rec * dtfr, int nn)
{
    dtfr->dtf1_num1 = nn * 16 + nn;
    memset(dtfr->dtf1_card, 0, sizeof(dtfr->dtf1_card));
    //strcpy(dtfr->dtf1_card, "Ijk");
    dtfr->dtf1_card[0] = (char)(64 + ((nn - 1) * 4));
    dtfr->dtf1_card[1] = (char)(96 + ((nn - 1) * 4) + 1);
    dtfr->dtf1_card[2] = (char)(96 + ((nn - 1) * 4) + 2);
    dtfr->dtf1_card[3] = '\0';
}
/**************************************************************/
static void init_demi_test_function_ARGS(struct demi_test_function_ARGS * args)
{
/*
** 01/07/2020
*/
    struct demi_test_function_rec * dtfr;

    args->dtf0_num = 17;
    memset(args->dtf0_card, 0, sizeof(args->dtf0_card));
    strcpy(args->dtf0_card, "Abc");

    dtfr = New(struct demi_test_function_rec, 1);
    init_demi_test_function_rec(dtfr, 2);
    args->dtf0_rec1 = dtfr;

    dtfr = New(struct demi_test_function_rec, 1);
    init_demi_test_function_rec(dtfr, 3);
    args->dtf0_rec2 = dtfr;

    dtfr = New(struct demi_test_function_rec, 1);
    init_demi_test_function_rec(dtfr, 4);
    args->dtf0_rec3 = dtfr;
}
#endif
/**************************************************************/
#if DEMI_TEST_INPUT_RECORD == 2
static void init_demi_test_function_ARGS(struct demi_test_function_ARGS * args)
{
/*
** 01/07/2020
*/
    args->dtf0_num1 = 17;
    args->dtf0_cardlist1 = Strdup("Abc");
    //args->dtf0_cardlist2 = Strdup("Def");
    args->dtf0_num2 = 34;
}
#endif
/**************************************************************/
int demi_call_test_function(struct demi_boss * demb, void * vdemu)
{
/*
** 01/07/2020
*/
    int dstat;
    struct demi_test_function_ARGS args;
    struct demi_test_function_OUT frtn;

    init_demi_test_function_ARGS(&args);

    dstat = demb_call_function(demb, vdemu, &args, &frtn);

    return (dstat);
}
/**************************************************************/
int demi_test_all(struct demi_boss * demb)
{
/*
** 01/07/2020
*/
    int dstat;
    void * vdemu;

    dstat = 0;
    if (!dstat) dstat = demb_define_const(demb, "MAX_CARDS_IN_SLOT", "%d", MAX_CARDS_IN_SLOT);
    dstat = demb_define_type(demb, "card_t", "char", sizeof(char));

#if DEMI_TEST_INPUT_RECORD == 1
    if (!dstat) {
        dstat = demb_define_type(demb, "demi_test_function_rec",
            "{"
                "dtf1_num1 : int;"
                "dtf1_card : card_t[4];"
            "}", sizeof(struct demi_test_function_rec));
    }

    if (!dstat) {
        dstat = demb_define_type(demb, "demi_test_function_ARGS",
            "{"
                "dtf0_num   : int;"
                "dtf0_rec1  : * demi_test_function_rec;"
                "dtf0_rec2  : * demi_test_function_rec;"
                "dtf0_rec3  : * demi_test_function_rec;"
                "dtf0_card  : card_t[4];"
            "}", sizeof(struct demi_test_function_ARGS));
    }
#endif
#if DEMI_TEST_INPUT_RECORD == 2
    if (!dstat) {
        dstat = demb_define_type(demb, "demi_test_function_ARGS",
            "{"
                "dtf0_num1      : int;"
                "dtf0_cardlist1 : * card_t[];"
                "dtf0_num2      : int;"
            "}", sizeof(struct demi_test_function_ARGS));
    }
#endif

#if DEMI_TEST_OUTPUT_RECORD == 1
    if (!dstat) {
        dstat = demb_define_type(demb, "demi_test_function_OUT",
            "{"
                "opiota  : int;"
            "}", sizeof(struct demi_test_function_OUT));
    }
#endif

    if (!dstat) {
        dstat = demi_test_declare_funcs(demb, &vdemu);
    }

    if (!dstat) {
        dstat = demi_call_test_function(demb, vdemu);
    }

    return (dstat);
}
/**************************************************************/
int demi_test_executor(struct demi_test_function_ARGS * args)
{
/*
** 01/07/2020
*/
    int opiota;

    opiota = 0;

    return (opiota);
}
/**************************************************************/
int demi_test_function(void * funcin, void * funcout)
{
/*
** 01/05/2020
*/
    int dstat;
    struct demi_test_function_ARGS * args;
    struct demi_test_function_OUT * frtn;

    dstat = 0;
    args = (struct demi_test_function_ARGS *)funcin;
    frtn = (struct demi_test_function_OUT *)funcout;

    frtn->opiota = demi_test_executor(args);

    return (dstat);
}
/**************************************************************/
int demi_test_register_callable_functions(void * vdemi,
    int (*register_func)(void * vd, const char * funcnam, DEMI_CALLABLE_FUNCTION func),
    const char * funccfg)
{
/*
** 01/07/2020
*/
    int dstat;

    dstat = register_func(vdemi, "demi_test_function", demi_test_function);

    return (dstat);
}
#endif /* DEMI_TESTING */
/***************************************************************/
static void sdemi_append_spaces(struct dstrec * dst, int nspaces)
{
/*
** 01/10/2020
*/

    appendbuf_spaces(&(dst->dst_pbuf), &(dst->dst_pbuflen), &(dst->dst_pbufmax), nspaces);
    dst->dst_pllen += nspaces;
}
/***************************************************************/
static void sdemi_append_tab(struct dstrec * dst, int pdepth)
{
/*
** 01/10/2020
*/
    sdemi_append_spaces(dst, pdepth * DEMIDAT_TAB_SIZE);
}
/***************************************************************/
static void sdemi_appendbuf(
        struct dstrec * dst,
        const char * cbuf,
        int pdepth)
{
/*
** 01/09/2020
*/
    int cbuflen;
    
    cbuflen = IStrlen(cbuf);
    if (((dst->dst_pflags & DEMIDAT_FLAG_LINE_80 ) && (dst->dst_pllen + cbuflen > 80 )) ||
        ((dst->dst_pflags & DEMIDAT_FLAG_LINE_132) && (dst->dst_pllen + cbuflen > 132))) {
        appendbuf_n(&(dst->dst_pbuf), &(dst->dst_pbuflen), &(dst->dst_pbufmax), "\n", 1);
        dst->dst_pllen = 0;
        if (dst->dst_pflags & DEMIDAT_FLAG_FMT)
            sdemi_append_tab(dst, pdepth);
    }
    appendbuf_n(&(dst->dst_pbuf), &(dst->dst_pbuflen), &(dst->dst_pbufmax), cbuf, cbuflen);
    dst->dst_pllen += cbuflen;
}
/***************************************************************/
static void sdemi_append_nl(struct dstrec * dst)
{
/*
** 01/10/2020
*/

    appendbuf_n(&(dst->dst_pbuf), &(dst->dst_pbuflen), &(dst->dst_pbufmax), "\n", 1);
    dst->dst_pllen = 0;
}
/***************************************************************/
static int demi_string_demityp_struct(
        struct demirec * demi,
        struct dstrec * dst,
        const uchar * udptr,
        struct demityp_struct * demts,
        int pdepth)
{
/*
** 01/10/2020
*/
    int dstat;
    int ii;
    struct demifield * demf;

    dstat = 0;
    sdemi_appendbuf(dst, "{", pdepth);
    if (dst->dst_pflags & DEMIDAT_FLAG_FMT) sdemi_append_nl(dst);

    for (ii = 0; !dstat && ii < demts->demts_num_fields; ii++) {
        demf = demts->demts_fields[ii];
        if (dst->dst_pflags & DEMIDAT_FLAG_FMT)
            sdemi_append_tab(dst, pdepth + 1);
        sdemi_appendbuf(dst, demf->demf_name, pdepth);
        if (dst->dst_pflags & DEMIDAT_FLAG_FMT)
            sdemi_append_spaces(dst, 16 - IStrlen(demf->demf_name));
        sdemi_appendbuf(dst, ": ", pdepth);
        dstat = demi_string_demityp_data(demi, dst,
            udptr + demf->demf_offset, demf->demf_dmityp, pdepth + 1);
        if (ii + 1 < demts->demts_num_fields) {
            sdemi_appendbuf(dst, ",", pdepth);
        }
        if (dst->dst_pflags & DEMIDAT_FLAG_FMT) sdemi_append_nl(dst);
    }

    if (dst->dst_pflags & DEMIDAT_FLAG_FMT)
        sdemi_append_tab(dst, pdepth);
    sdemi_appendbuf(dst, "}", pdepth);

    return (dstat);
}
/***************************************************************/
static int demi_string_demityp_array(
        struct demirec * demi,
        struct dstrec * dst,
        const uchar * udptr,
        struct demityp_array * demta,
        int pdepth)
{
/*
** 01/10/2020
*/
    int dstat;
    int offset;
    int ii;

    dstat = 0;
    switch (demta->demta_array_type) {
        case DEMTA_ARRAY_TYPE_FIXED_SIZE:
            if (demta->demta_dmityp->demt_simp_typ == DEMITYP_CHAR) {
                sdemi_appendbuf(dst, "\"", pdepth);
                for (ii = 0; ii < demta->demta_num_elements && udptr[ii]; ii++) {
                    appendbuf_n(&(dst->dst_pbuf), &(dst->dst_pbuflen), &(dst->dst_pbufmax),
                        udptr + ii, 1);
                }
                sdemi_appendbuf(dst, "\"", pdepth);
            } else {
                sdemi_appendbuf(dst, "[", pdepth);
                offset = 0;
                for (ii = 0; !dstat && ii < demta->demta_num_elements; ii++) {
                    if (ii) sdemi_appendbuf(dst, ",", pdepth);
                    dstat = demi_string_demityp_data(demi, dst, udptr + offset, demta->demta_dmityp, pdepth);
                    offset += demta->demta_array_stride;
                }
                sdemi_appendbuf(dst, "]", pdepth);
            }
            break;
        default:
            dstat = demi_set_error(demi, DEMIERR_PRINT_ERR,
                "Invalid array type. Found: %d", demta->demta_array_type);
            break;
    }

    return (dstat);
}
/***************************************************************/
static int demi_string_demityp_ptr(
        struct demirec * demi,
        struct dstrec * dst,
        const uchar * udptr,
        struct demityp_ptr * demtp,
        int pdepth)
{
/*
** 01/10/2020
*/
    int dstat;
    int ii;
    void ** vdptr = (void **)udptr;
    uchar * dudptr;

    dstat = 0;
    if (udptr) sdemi_append_f(dst, "(0x%x)->", *vdptr, pdepth);
    else sdemi_append_f(dst, "(null)->", pdepth);
    switch (demtp->demtp_ptr_type) {
        case DEMTP_PTR_TYPE_SINGLE_ITEM:
            dstat = demi_string_demityp_data(demi, dst, *((uchar**)udptr), demtp->demtp_dmityp, pdepth);
            break;
        case DEMTP_PTR_TYPE_NULL_TERM:
            if (demtp->demtp_dmityp->demt_simp_typ == DEMITYP_CHAR) {
                sdemi_appendbuf(dst, "\"", pdepth);
                dudptr = *((uchar**)udptr);
                for (ii = 0; dudptr[ii]; ii++) {
                    appendbuf_n(&(dst->dst_pbuf), &(dst->dst_pbuflen), &(dst->dst_pbufmax),
                        dudptr + ii, 1);
                }
                sdemi_appendbuf(dst, "\"", pdepth);
            } else if (demtp->demtp_dmityp->demt_typ_size == sizeof(void*)) {
                sdemi_appendbuf(dst, "[", pdepth);
                for (ii = 0; (vdptr + ii) != NULL; ii++) {
                    if (ii) sdemi_appendbuf(dst, ",", pdepth);
                    dstat = demi_string_demityp_data(demi, dst, *((uchar**)(vdptr + ii)), demtp->demtp_dmityp, pdepth);
                }
                sdemi_appendbuf(dst, "]", pdepth);
            } else {
                dstat = demi_set_error(demi, DEMIERR_PRINT_ERR,
                    "Unsupported null terminated array element size. Found: %d",
                        demtp->demtp_dmityp->demt_typ_size);
            }
            break;
        case DEMTP_PTR_TYPE_VOID_PTR:
            dstat = demi_string_demityp_data(demi, dst, udptr, demtp->demtp_dmityp, pdepth);
            break;
        default:
            dstat = demi_set_error(demi, DEMIERR_PRINT_ERR,
                "Invalid pointer type. Found: %d", demtp->demtp_ptr_type);
            break;
    }

    return (dstat);
}
/***************************************************************/
static int demi_string_demityp_data(
        struct demirec * demi,
        struct dstrec * dst,
        const uchar * udptr,
        struct demityp * demt,
        int pdepth)
{
/*
** 01/08/2020
*/
    int dstat;
    int uns;
    char nbuf[32];

    dstat = 0;
    uns   = 0;
    if (demt->demt_typ_flags & DEMT_TYP_FLAG_UNSIGNED) uns = 1;

    switch (demt->demt_simp_typ) {
        case DEMITYP_CHAR:
            sprintf(nbuf, "%c", (*((char*)udptr)));
            sdemi_appendbuf(dst, nbuf, pdepth);
            break;
        case DEMITYP_INT8:
            if (uns) sprintf(nbuf, "%d", (int)(*((int8*)udptr)));
            else     sprintf(nbuf, "%d", (int)(*((uint8*)udptr)));
            sdemi_appendbuf(dst, nbuf, pdepth);
            break;
        case DEMITYP_INT16:
            if (uns) sprintf(nbuf, "%d", (int)(*((int16*)udptr)));
            else     sprintf(nbuf, "%d", (int)(*((uint16*)udptr)));
            sdemi_appendbuf(dst, nbuf, pdepth);
            break;
        case DEMITYP_INT32:
            if (uns) sprintf(nbuf, "%d", (int)(*((int32*)udptr)));
            else     sprintf(nbuf, "%d", (int)(*((uint32*)udptr)));
            sdemi_appendbuf(dst, nbuf, pdepth);
            break;
        case DEMITYP_INT64:
            if (uns) sprintf(nbuf, "%I64d", (int64)(*((int64*)udptr)));
            else     sprintf(nbuf, "%I64d", (int64)(*((uint64*)udptr)));
            sdemi_appendbuf(dst, nbuf, pdepth);
            break;
        case DEMITYP_FLOAT32:
            sprintf(nbuf, "%g", (double)(*((float32*)udptr)));
            sdemi_appendbuf(dst, nbuf, pdepth);
            break;
        case DEMITYP_FLOAT64:
            sprintf(nbuf, "%g", (double)(*((float64*)udptr)));
            sdemi_appendbuf(dst, nbuf, pdepth);
            break;
        case DEMITYP_VOID:
            strcpy(nbuf, "void");
            sdemi_appendbuf(dst, nbuf, pdepth);
            break;
        case DEMITYP_STRUCT:
            dstat = demi_string_demityp_struct(demi, dst, udptr, demt->demt_u.demt_demts, pdepth);
            break;
        case DEMITYP_ARRAY:
            dstat = demi_string_demityp_array(demi, dst, udptr, demt->demt_u.demt_demta, pdepth);
            break;
        case DEMITYP_POINTER:
            dstat = demi_string_demityp_ptr(demi, dst, udptr, demt->demt_u.demt_demtp, pdepth);
            break;

        default:
            printf("?? type=%d ??", demt->demt_simp_typ);
            break;
    }

    return (dstat);
}
/***************************************************************/
static void sdemi_init_dstrec(struct dstrec * dst, int pflags)
{
/*
** 01/10/2020
*/
    dst->dst_pbuf       = NULL; 
    dst->dst_pbuflen    = 0; 
    dst->dst_pbufmax    = 0; 
    dst->dst_pllen      = 0; 
    dst->dst_pflags     = pflags;
    dst->dst_file       = stdout;
}
/***************************************************************/
static void sdemi_free_dstrec_data(struct dstrec * dst)
{
/*
** 01/13/2020
*/
    Free(dst->dst_pbuf);
}
/***************************************************************/
int demi_print_data(struct demirec * demi, void * dptr, struct demityp * demt, int pflags)
{
/*
** 01/08/2020
*/
    int dstat;
    int apflags;
    struct dstrec dst;

    apflags = pflags;
    if (!apflags) {
        apflags = DEMIDAT_FLAG_FMT | DEMIDAT_FLAG_LINE_80;
    }

    sdemi_init_dstrec(&dst, apflags);

    dstat = demi_string_demityp_data(demi, &dst, (uchar*)dptr, demt, 0);
    if (!dstat && dst.dst_pbuf) {
        fprintf(dst.dst_file, "%s\n", dst.dst_pbuf);
    }
    sdemi_free_dstrec_data(&dst);

    return (dstat);
}
/***************************************************************/
void demi_init_cpurec(
    struct cpurec * cpu,
    struct demiprog * demp,
    struct demimemory * tgtdemmem,
    void * srcptr,
    int srclen,
    int cpuflags)
{
/*
** 01/22/2020
*/
    cpu->cpu_state          = 0;
    cpu->cpu_pc             = 0;
    cpu->cpu_demp           = demp;
    cpu->cpu_stack_size     = DEMICPU_INITIAL_STACK;
    if (cpu->cpu_stack_size) cpu->cpu_stack = New(pint, cpu->cpu_stack_size);
    else cpu->cpu_stack     = NULL;
    cpu->cpu_tos            = 0;
    cpu->cpu_tgtdemmem      = tgtdemmem;
    cpu->cpu_srcptr         = (uchar*)srcptr;
    cpu->cpu_headsrcptr     = cpu->cpu_srcptr;
    cpu->cpu_tgtix          = 0;
    cpu->cpu_tgtptr         = tgtdemmem->demmem_ptr;
    cpu->cpu_srclen         = srclen;
    cpu->cpu_flags          = cpuflags;
    cpu->cpu_dbg_pc         = 0;

    tgtdemmem->demmem_length = 0;

#if DEMI_TESTING
    cpu->cpu_nzones     = 0;
    cpu->cpu_xzones     = 0;
    cpu->cpu_zonesz     = NULL;
    cpu->cpu_zoneptr    = NULL;
#endif
}
/***************************************************************/
#if 0
static void demmem_copy_to_index(struct demimemory * demmem, uchar * src, int srclen)
{
/*
** 01/22/2020
*/
    if (demmem->demmem_index + srclen > demmem->demmem_size) {
        demmem->demmem_size = demmem->demmem_index + srclen + 64;
        demmem->demmem_ptr =
            Realloc(demmem->demmem_ptr, uchar, demmem->demmem_size);
    }
    memcpy(demmem->demmem_ptr + demmem->demmem_index, src, srclen);
    demmem->demmem_index += srclen;
    if (demmem->demmem_index > demmem->demmem_length)
        demmem->demmem_length = demmem->demmem_index;
}
#endif
/***************************************************************/
void demi_free_cpurec_data(struct cpurec * cpu)
{
/*
** 01/22/2020
*/
    Free(cpu->cpu_stack);
}
/**************************************************************/
void continue_wait(void)
{
    char xbuf[80];

    printf("Continue...");
    fgets(xbuf, sizeof(xbuf), stdin);
}
/***************************************************************/
int demi_print_cpurec(struct demirec * demi,
    struct cpurec * cpu,
    DINST inst,
    int in_dstat,
    int pflags)
{
/*
** 01/23/2020
*/
    int dstat;
    int apflags;
    int ii;
    struct dstrec dst;

    dstat = 0;
    apflags = pflags;
    if (!apflags) {
        apflags = DEMIDAT_FLAG_FMT | DEMIDAT_FLAG_LINE_80;
    }

    fprintf(stdout, "================================================================\n");
    hlog(0, HLOG_FLAG_USE_ADDR, cpu->cpu_headsrcptr, cpu->cpu_srclen,
        "Input (0x%lx,len=%d) srcptr=0x%lx:",
        cpu->cpu_headsrcptr, cpu->cpu_srclen, cpu->cpu_srcptr);
    hlog(0, HLOG_FLAG_USE_ADDR, cpu->cpu_tgtdemmem->demmem_ptr, cpu->cpu_tgtdemmem->demmem_length,
        "Output (0x%lx,len=%d) tgtptr=0x%lx tgtix=0x%lx:",
        cpu->cpu_tgtdemmem->demmem_ptr,
        cpu->cpu_tgtdemmem->demmem_length,
        cpu->cpu_tgtptr,
        cpu->cpu_tgtix);

    fprintf(stdout, "Stack size=%d\n", cpu->cpu_tos);
    for (ii = 0; ii < cpu->cpu_tos; ii++) {
        fprintf(stdout, "%d 0x%lx\n", ii, cpu->cpu_stack[cpu->cpu_tos - ii - 1]);
    }
#if DEBUG_PRINT_CPU
    if (cpu->cpu_nzones) {
        /* fprintf(stdout, "Zones=%d\n", cpu->cpu_nzones); */
        for (ii = 0; ii < cpu->cpu_nzones; ii++) {
            hlog(0, HLOG_FLAG_USE_ADDR | HLOG_FLAG_MSG_EVERY_LINE,
                cpu->cpu_zoneptr[ii], cpu->cpu_zonesz[ii],
                "Zone %d,len=0x%lx",
                ii,
                cpu->cpu_zonesz[ii]);
        }
    }
#endif

    sdemi_init_dstrec(&dst, apflags);
    sdemi_append_f(&dst, "%2d. ", cpu->cpu_pc - cpu->cpu_dbg_pc);
    dstat = sdemi_print_demiinst(demi, inst, &dst);
    sdemi_append_nl(&dst);
    if (!dstat && dst.dst_pbuf) {
        fprintf(dst.dst_file, "%s\n", dst.dst_pbuf);
    }
    sdemi_free_dstrec_data(&dst);

    continue_wait();

    return (dstat);
}
/***************************************************************/
#if DEBUG_PRINT_CPU
void demi_add_zone(struct demirec * demi,
    struct cpurec * cpu,
    uchar * buf,
        int nbytes)
{
/*
** 01/28/2020
    cpu->cpu_nzones     = 0;
    cpu->cpu_xzones     = 0;
    cpu->cpu_zonesz     = NULL;
    cpu->cpu_zoneptr    = NULL;
*/

    if (cpu->cpu_nzones == cpu->cpu_xzones) {
        if (!cpu->cpu_xzones) cpu->cpu_xzones = 4;
        else cpu->cpu_xzones *= 2;
        cpu->cpu_zoneptr = Realloc(cpu->cpu_zoneptr, uchar *, cpu->cpu_xzones);
        cpu->cpu_zonesz  = Realloc(cpu->cpu_zonesz, int, cpu->cpu_xzones);
    }
    cpu->cpu_zoneptr[cpu->cpu_nzones] = buf;
    cpu->cpu_zonesz[cpu->cpu_nzones]  = nbytes;
    cpu->cpu_nzones += 1;
}
#endif
/***************************************************************/
uchar * demi_run_new(struct demirec * demi,
    struct cpurec * cpu,
        int nbytes)
{
/*
** 01/28/2020
*/
    uchar * buf;

    buf = New(uchar, nbytes);
#if DEBUG_PRINT_CPU
    demi_add_zone(demi, cpu, buf, nbytes);
#endif

    return (buf);
}
/***************************************************************/
void demi_run_addtgtlen(struct demirec * demi,
    struct cpurec * cpu,
        int nbytes)
{
/*
** 01/29/2020
*/
    cpu->cpu_tgtdemmem->demmem_length += nbytes;
    if (cpu->cpu_tgtdemmem->demmem_length > cpu->cpu_tgtdemmem->demmem_size) {
        cpu->cpu_tgtdemmem->demmem_size =
            cpu->cpu_tgtdemmem->demmem_length + 64;
        cpu->cpu_tgtdemmem->demmem_ptr =
            Realloc(cpu->cpu_tgtdemmem->demmem_ptr, uchar, cpu->cpu_tgtdemmem->demmem_size);
    }
    cpu->cpu_tgtptr = cpu->cpu_tgtdemmem->demmem_ptr + cpu->cpu_tgtix;
}
/***************************************************************/
int demi_run_strlen2(uchar * sptr)
{
/*
** 01/29/2020
*/
    int16 * i2ptr = (int16*)sptr;
    int len;

    for (len = 0; i2ptr[len]; len++) ;

    return (len);
}
/***************************************************************/
int demi_run_strlen4(uchar * sptr)
{
/*
** 01/29/2020
*/
    int32 * i4ptr = (int32*)sptr;
    int len;

    for (len = 0; i4ptr[len]; len++) ;

    return (len);
}
/***************************************************************/
int demi_run_strlen8(uchar * sptr)
{
/*
** 01/29/2020
*/
    int64 * i8ptr = (int64*)sptr;
    int len;

    for (len = 0; i8ptr[len]; len++) ;

    return (len);
}
/***************************************************************/
void demi_run_strcpy1(uchar * tptr, uchar * sptr)
{
/*
** 02/05/2020
*/
    uchar * i1tptr = (uchar*)tptr;
    uchar * i1sptr = (uchar*)sptr;

    while (*i1sptr) { *i1sptr++ = *i1tptr++; }
    *i1tptr = 0;
}
/***************************************************************/
void demi_run_strcpy2(uchar * tptr, uchar * sptr)
{
/*
** 02/05/2020
*/
    int16 * i2tptr = (int16*)tptr;
    int16 * i2sptr = (int16*)sptr;

    while (*i2sptr) { *i2sptr++ = *i2tptr++; }
    *i2tptr = 0;
}
/***************************************************************/
void demi_run_strcpy4(uchar * tptr, uchar * sptr)
{
/*
** 02/05/2020
*/
    int32 * i4tptr = (int32*)tptr;
    int32 * i4sptr = (int32*)sptr;

    while (*i4sptr) { *i4sptr++ = *i4tptr++; }
    *i4tptr = 0;
}
/***************************************************************/
void demi_run_strcpy8(uchar * tptr, uchar * sptr)
{
/*
** 02/05/2020
*/
    int64 * i8tptr = (int64*)tptr;
    int64 * i8sptr = (int64*)sptr;

    while (*i8sptr) { *i8sptr++ = *i8tptr++; }
    *i8tptr = 0;
}
/***************************************************************/
void demi_run_memcpy_src_tgtix(struct cpurec * cpu, int nbytes)
{
/*
** 02/08/2020
*/
    uchar * mtgt;
    uchar * msrc;

    mtgt = cpu->cpu_tgtdemmem->demmem_ptr + cpu->cpu_tgtix;
    msrc = cpu->cpu_srcptr;

    memcpy(mtgt, msrc, nbytes);
}
/***************************************************************/
int demi_run_cpurec(struct demirec * demi,
    struct cpurec * cpu)
{
/*
** 01/22/2020
*/
    int dstat;
    int iop;
    int iix;
    int num;
    pint tmppint;
    uchar * tmpptr;
    DINST inst;
    //uchar tmpbuf[16];

    dstat = 0;
    while (cpu->cpu_state == CPU_STATE_RUNNING) {
        if (cpu->cpu_pc >= cpu->cpu_demp->demp_ninst) {
            cpu->cpu_state = CPU_STATE_OVERRUN;
        } else {
            inst = cpu->cpu_demp->demp_ainst[cpu->cpu_pc];
            switch (inst >> 14) {
                case 0:
                    switch (inst >> DEMINST_IBITS) {
                        case 0:
                            break;
                        case (DEMINST_PUSHI >> DEMINST_IBITS) :
                            CPU_ADDS(cpu,1);
                            CPU_TOS(cpu) = inst & DEMINST_IBITS_MASK;
                            break;
                        case (DEMINST_PUSHI_HI >> DEMINST_IBITS) :
                            CPU_ADDS(cpu,1);
                            CPU_TOS(cpu) = (inst & DEMINST_IBITS_MASK) << DEMINST_IBITS;
                            break;
                        case (DEMINST_COPY_TO_IX >> DEMINST_IBITS) :
                            num = inst & DEMINST_IBITS_MASK;
                            memcpy(cpu->cpu_tgtdemmem->demmem_ptr + cpu->cpu_tgtix, cpu->cpu_srcptr, num);
                            cpu->cpu_srcptr += num;
                            cpu->cpu_tgtix  += num;
                            break;
                        case (DEMINST_COPY_TO_IX_HI >> DEMINST_IBITS) :
                            num = (inst & DEMINST_IBITS_MASK) << DEMINST_IBITS;
                            memcpy(cpu->cpu_tgtdemmem->demmem_ptr + cpu->cpu_tgtix, cpu->cpu_srcptr, num);
                            cpu->cpu_srcptr += num;
                            cpu->cpu_tgtix  += num;
                            break;
                        case (DEMINST_COPY_TO_PTR >> DEMINST_IBITS) :
                            num = inst & DEMINST_IBITS_MASK;
                            memcpy(cpu->cpu_tgtptr, cpu->cpu_srcptr, num);
                            cpu->cpu_srcptr += num;
                            cpu->cpu_tgtptr += num;
                            break;
                        case (DEMINST_COPY_TO_PTR_HI >> DEMINST_IBITS) :
                            num = (inst & DEMINST_IBITS_MASK) << DEMINST_IBITS;
                            memcpy(cpu->cpu_tgtptr, cpu->cpu_srcptr, num);
                            cpu->cpu_srcptr += num;
                            cpu->cpu_tgtptr += num;
                            break;
                        case (DEMINST_ADDI >> DEMINST_IBITS) :
                            CPU_TOS(cpu) += inst & DEMINST_IBITS_MASK;
                            break;
                        case (DEMINST_ADDI_HI >> DEMINST_IBITS) :
                            CPU_TOS(cpu) += (inst & DEMINST_IBITS_MASK) << DEMINST_IBITS;
                            break;
                        case (DEMINST_ADDTGTLENI >> DEMINST_IBITS) :
                            demi_run_addtgtlen(demi, cpu, inst & DEMINST_IBITS_MASK);
                            break;
                        case (DEMINST_ADDTGTLENI_HI >> DEMINST_IBITS) :
                            demi_run_addtgtlen(demi, cpu, (inst & DEMINST_IBITS_MASK) << DEMINST_IBITS);
                            break;

                        default:
                            cpu->cpu_state = CPU_STATE_INSTRUCTION;
                            dstat = demi_set_error(demi, DEMIERR_UNIMPLEMENTED_INST,
                                    "Unimplemented instruction for category 0: %x", inst);
                            break;
                    }
                    break;

                case 1:
                    switch (inst >> 8) {
                        case 0:
                            break;

                        case (DEMINST_DEBUG >> 8):
                            cpu->cpu_flags |= (inst & 0x0F);
                            cpu->cpu_dbg_pc = cpu->cpu_pc;
                            break;

                        default:
                            cpu->cpu_state = CPU_STATE_INSTRUCTION;
                            dstat = demi_set_error(demi, DEMIERR_UNIMPLEMENTED_INST,
                                "Unimplemented instruction category: %d", inst >> 14);
                            break;
                    }
                    break;

                case 2:
                    for (iix = 0; iix <= 1; iix++) {
                        if (!iix) iop = (inst >> DEMSINST_IBITS) & DEMSINST_IBITS_MASK;
                        else      iop = inst & DEMSINST_IBITS_MASK;
                        switch (iop) {
                            case DEMSINST_NOOP:
                                break;
                            case DEMSINST_ZEROTGT_IX:
                                cpu->cpu_tgtix = 0;
                                break;
                            case DEMSINST_PUSHTGT_IX:
                                CPU_ADDS(cpu,1);
                                CPU_TOS(cpu) = cpu->cpu_tgtix;
                                break;
                            case DEMSINST_STORTGT_IX:
                                cpu->cpu_tgtix = CPU_TOS(cpu);
                                CPU_SUBS(cpu,1);
                                break;
                            case DEMSINST_STORA_TGT_IX:
                                num = sizeof(pint);
                                memcpy(cpu->cpu_tgtdemmem->demmem_ptr + cpu->cpu_tgtix, &(CPU_TOS(cpu)), num);
                                cpu->cpu_tgtix += num;
                                CPU_SUBS(cpu,1);
                                break;
                            case DEMSINST_STORSRC_PTR:
                                cpu->cpu_srcptr = (uchar*)CPU_TOS(cpu);
                                CPU_SUBS(cpu,1);
                                break;
                            case DEMSINST_STORTGT_PTR:
                                cpu->cpu_tgtptr = (uchar*)CPU_TOS(cpu);
                                CPU_SUBS(cpu,1);
                                break;
                            case DEMSINST_XCHGAB:
                                tmppint = CPU_STACKB(cpu);
                                CPU_STACKB(cpu) = CPU_TOS(cpu);
                                CPU_TOS(cpu) = tmppint;
                                break;
                            case DEMSINST_DUPA:
                                CPU_ADDS(cpu,1);
                                CPU_TOS(cpu) = CPU_STACKB(cpu);
                                break;
                            case DEMSINST_PUSHSRC_PTR:
                                CPU_ADDS(cpu,1);
                                CPU_TOS(cpu) = (pint)(cpu->cpu_srcptr);
                                break;
                            case DEMSINST_PUSHTGT_PTR:
                                CPU_ADDS(cpu,1);
                                CPU_TOS(cpu) = (pint)(cpu->cpu_tgtptr);
                                break;
                            case DEMSINST_DEREFERENCE:
                                CPU_TOS(cpu) = (pint)(*((uchar**)CPU_TOS(cpu)));
                                break;
                            case DEMSINST_ADD_VOID_STAR:
                                CPU_TOS(cpu) += sizeof(void*);
                                break;
                            case DEMSINST_END0:
                                cpu->cpu_state = CPU_STATE_END0;
                                break;
                            case DEMSINST_END1:
                                cpu->cpu_state = CPU_STATE_END1;
                                break;
                            case DEMSINST_PUSHAT_SRC_PTR:
                                CPU_ADDS(cpu,1);
                                CPU_TOS(cpu) = (pint)(*((uchar**)(cpu->cpu_srcptr)));
                                break;
                            case DEMSINST_STOR_IX_SRC_PTR:
                                cpu->cpu_srcptr = cpu->cpu_headsrcptr + CPU_TOS(cpu);
                                CPU_SUBS(cpu,1);
                                break;
                            case DEMSINST_STOR_IX_TGT_PTR:
                                cpu->cpu_tgtptr = cpu->cpu_tgtdemmem->demmem_ptr + CPU_TOS(cpu);
                                CPU_SUBS(cpu,1);
                                break;
                            case DEMSINST_ROTATE_CAB:
                                tmppint = CPU_STACKC(cpu);
                                CPU_STACKC(cpu) = CPU_STACKB(cpu);
                                CPU_STACKB(cpu) = CPU_TOS(cpu);
                                CPU_TOS(cpu) = tmppint;
                                break;
                            case DEMSINST_NEW:
                                CPU_TOS(cpu) = (pint)demi_run_new(demi, cpu, CPU_TOS(cpu));
                                break;
                            case DEMSINST_STORAT_TGT_PTR:
                                (pint)(*((uchar**)(cpu->cpu_tgtptr))) = CPU_TOS(cpu);
                                CPU_SUBS(cpu,1);
                                break;
                            case DEMSINST_ADDTGTLEN:
                                demi_run_addtgtlen(demi, cpu, CPU_TOS(cpu));
                                CPU_SUBS(cpu,1);
                                break;
                            case DEMSINST_PUSH_STRLEN_SRC1:
                                CPU_ADDS(cpu,1);
                                tmpptr = cpu->cpu_srcptr;
                                tmpptr = *((uchar**)tmpptr);
                                CPU_TOS(cpu) = strlen(tmpptr) + 1;
                                break;
                            case DEMSINST_PUSH_STRLEN_SRC2:
                                CPU_ADDS(cpu,1);
                                CPU_TOS(cpu) = demi_run_strlen2(cpu->cpu_srcptr) + 2;
                                break;
                            case DEMSINST_PUSH_STRLEN_SRC4:
                                CPU_ADDS(cpu,1);
                                CPU_TOS(cpu) = demi_run_strlen4(cpu->cpu_srcptr) + 4;
                                break;
                            case DEMSINST_PUSH_STRLEN_SRC8:
                                CPU_ADDS(cpu,1);
                                CPU_TOS(cpu) = demi_run_strlen8(cpu->cpu_srcptr) + 8;
                                break;
                            case DEMSINST_ADD:
                                CPU_STACKB(cpu) = CPU_TOS(cpu) + CPU_STACKB(cpu);
                                CPU_SUBS(cpu,1);
                                break;
                            case DEMSINST_SUB:
                                CPU_STACKB(cpu) = CPU_TOS(cpu) - CPU_STACKB(cpu);
                                CPU_SUBS(cpu,1);
                                break;
                            case DEMSINST_MEMCPY_SRC_TGTIX:
                                tmppint = CPU_TOS(cpu);
                                CPU_SUBS(cpu,1);
                                demi_run_memcpy_src_tgtix(cpu, tmppint);
                                break;

                            default:
                                cpu->cpu_state = CPU_STATE_INSTRUCTION;
                                dstat = demi_set_error(demi, DEMIERR_UNIMPLEMENTED_INST,
                                    "Unimplemented instruction for category 2: %x", inst);
                                break;
                        }
                    }
                    break;

                case 3:
                    cpu->cpu_state = CPU_STATE_INSTRUCTION;
                    dstat = demi_set_error(demi, DEMIERR_UNIMPLEMENTED_INST,
                        "Unimplemented instruction category: %d", inst >> 14);
                    break;

                default:
                    cpu->cpu_state = CPU_STATE_INSTRUCTION;
                    dstat = demi_set_error(demi, DEMIERR_UNIMPLEMENTED_INST,
                        "Unimplemented instruction category: %d", inst >> 14);
                    break;
            }
#if DEBUG_PRINT_CPU
            if (cpu->cpu_flags & CPU_FLAG_PRINT_CPU) {
                demi_print_cpurec(demi, cpu, inst, dstat, 0);
            }
#endif
        }
        cpu->cpu_pc += 1;
    }

    return (dstat);
}
/***************************************************************/
int demi_run_demiprog(struct demirec * demi,
    struct demiprog * demp,
    struct demimemory * tgtdemmem,
    void * srcptr,
    int srclen,
    int cpuflags)
{
/*
** 01/22/2020
*/
    int dstat = 0;
    struct cpurec cpu;

    demi_init_cpurec(&cpu, demp, tgtdemmem, srcptr, srclen, cpuflags);
    dstat = demi_run_cpurec(demi, &cpu);
    demi_free_cpurec_data(&cpu);

    return (dstat);
}
/***************************************************************/
