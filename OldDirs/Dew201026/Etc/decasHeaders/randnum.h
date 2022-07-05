/***************************************************************/
/*  randnum.h - Random numbers                                 */
/***************************************************************/
#define MS_CRT_LIB    1  /* RANDNUM_GENINT_0 */
#define SEDGEWICK_INT 1  /* RANDNUM_GENINT_1 */
#define KNUTH_INT     1  /* RANDNUM_GENINT_2 */
#define PSDES_INT     1  /* RANDNUM_GENINT_3 */
#define DE_MASK       0  /* RANDNUM_GENINT_4 -- FAIL- Does not pass chi square test */

#define RANDNUM_FLOAT 0  /* Floating point */
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
/***************************************************************/

typedef int rand_seed_t;

int randbetween(void * rvp, int loval, int hival);
void randintfree(void * rvp);
void * randintseed(int rntype, rand_seed_t s);
int randinttest(int rntype, rand_seed_t s);
/***************************************************************/

