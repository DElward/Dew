/***************************************************************/
/*  demicp.h                                                   */
/***************************************************************/
#ifndef DEMICPH_INCLUDED
#define DEMICPH_INCLUDED

#define DEMIDAT_FLAG_LINE_80    1
#define DEMIDAT_FLAG_LINE_132   2
#define DEMIDAT_FLAG_FMT        4

#define DEMIDAT_TAB_SIZE        4

#define DEMICPU_MEMORY_DEFAULT_SIZE     16

#define CPU_FLAG_PRINT_CPU          1


int demi_print_data(struct demirec * demi, void * dptr, struct demityp * demt, int pflags);
int demi_gen_pack_prog(struct demirec * demi, struct deminamedtyp * demnt);
int demi_gen_unpack_prog(struct demirec * demi, struct deminamedtyp * demnt);

struct demiprog * demi_new_demiprog(void);
void sdemi_free_demiprog(struct demiprog * demp);
int demi_print_demiprog(struct demirec * demi,
    struct demiprog * demp,
    const char * tdesc,
    const char * tnam,
    int pflags);
int demi_run_demiprog(struct demirec * demi,
    struct demiprog * demp,
    struct demimemory * tgtdemmem,
    void * srcptr,
    int srclen,
    int cpuflags);

/***************************************************************/
#endif /* DEMICPH_INCLUDED */
/***************************************************************/
