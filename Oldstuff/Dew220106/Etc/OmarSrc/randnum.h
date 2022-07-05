/***************************************************************/
/*  randnum.h - Random numbers                                 */
/***************************************************************/
#define RANDNUM_TESTING 0
#define WHMEM 1

#if WHMEM
    #include "cmemh.h"
#else
    #define New(t,n)         ((t*)calloc(sizeof(t),(n)))
    #define Realloc(p,t,n)   ((t*)realloc((p),sizeof(t)*(n)))
    #define Free(p)           free(p)
#endif
/***************************************************************/
#define MS_CRT_LIB    1  /* RANDNUM_GENINT_0 */
#define SEDGEWICK_INT 1  /* RANDNUM_GENINT_1 */
#define KNUTH_INT     1  /* RANDNUM_GENINT_2 */
#define PSDES_INT     1  /* RANDNUM_GENINT_3 */
#define DE_MASK       0  /* RANDNUM_GENINT_4 -- FAIL- Does not pass chi square test */

#define KNUTH_FLT     1  /* RANDNUM_GENFLT_2 */
#define NUMREC1_FLT   1  /* RANDNUM_GENFLT_5 */
/***************************************************************/
#if MS_CRT_LIB
#define RANDNUM_GENINT_0    0   /* Microsoft C runtime library integers */
#endif

#if SEDGEWICK_INT
#define RANDNUM_GENINT_1    1   /* Sedgewick C integers */
#endif

#if KNUTH_INT
#define RANDNUM_GENINT_2    2   /* Knuth C integers */
#endif

#if PSDES_INT
#define RANDNUM_GENINT_3    3   /* Dave's random bit mask */
#endif

#if DE_MASK
#define RANDNUM_GENINT_4    4   /* Dave's random bit mask */
#endif

#if KNUTH_FLT
#define RANDNUM_GENFLT_2    102   /* Knuth C doubles */
#endif

#if NUMREC1_FLT
#define RANDNUM_GENFLT_5    105   /* Numerical recipes ran1.c */
#endif

/***************************************************************/

typedef int rand_seed_t;

int randintbetween(void * rvp, int loval, int hival);
void randintfree(void * rvp);
void * randintseed(int rntype, rand_seed_t s);
int randintnext(void * rvp);
#if RANDNUM_TESTING
    int randinttest(int rntype, rand_seed_t s);
#endif
double randfltnext(void * rvp);
void randfltfree(void * rvp);
void * randfltseed(int rntype, rand_seed_t s);
double randfltnextgauss(void * vrfi);
/***************************************************************/

