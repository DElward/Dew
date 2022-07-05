/****************************************************/
/*  CMEMC                                           */
/*                                                  */
/*  This is the Warehouse memory interface.         */
/*  The New, Free and Realloc functions are in this */
/*  file.                                           */
/*                                                  */
/*  To turn heap checking on, the HEAP_CHECKING     */
/*  preprocessor variable needs to be defined.      */
/*  The easiest way to do this is to issue the      */
/*  following MPE/iX command before compiling this  */
/*  file:                                           */
/*     :SETVAR CCOPTS, "-DHEAP_CHECKING"            */
/****************************************************/

/***************************************************************/
/*
**      TMEM_TYPE = 0  Straight calls, no Taurus
**      TMEM_TYPE = 1  Taurus malloc 1
**
**  CMEM_TYPE defined in CMEMH:
**      CMEM_TYPE = 0  Straight calls, no Taurus
**      CMEM_TYPE = 1  Minimum overhead, no memory tracking
**      CMEM_TYPE = 2  Use memory tracking (old default)
*/
#define TMEM_TYPE 1

/*****************************************/
/* #define HEAP_CHECKING */
/*****************************************/

#define MT_MEM  0   /* Multi-threaded memory */

#if MT_MEM
    #ifdef _Windows
        #define WIN32_LEAN_AND_MEAN
        #include <Windows.h>
        #define CRIT_INIT(cs)  (InitializeCriticalSection(cs))
        #define CRIT_LOCK(cs)  (EnterCriticalSection(cs))
        #define CRIT_ULOCK(cs) (LeaveCriticalSection(cs))
        #define CRIT_DEL(cs)   (DeleteCriticalSection(cs))
        #define CRIT_TRY(cs)   (TryEnterCriticalSection(cs))
        #define CRITSECTYP CRITICAL_SECTION
        CRITSECTYP * mem_crit_section;
    #else
        #include <pthread.h>
        #define CRIT_INIT(cs)  (pthread_mutex_init(cs,NULL))
        #define CRIT_LOCK(cs)  (pthread_mutex_lock(cs))
        #define CRIT_ULOCK(cs) (pthread_mutex_unlock(cs))
        #define CRIT_DEL(cs)   (pthread_mutex_destroy(cs))
        #define CRIT_TRY(cs)   (!pthread_mutex_trylock(cs))
        #define CRITSECTYP pthread_mutex_t
        CRITSECTYP * mem_crit_section;
    #endif /* _Windows */
#endif

/***************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>

#include "cmemh.h"

/*
** Setup MTYPE as a 32-bit integer
*/
#if (INT_MAX == 2147483647L)
typedef int MTYPE;
#elif (LONG_MAX == 2147483647L)
typedef long int MTYPE;
#else
#error No MTYPE value
#endif

#if TMEM_TYPE == 0
#define CALLOC calloc
#define MALLOC malloc
#define FREE free
#define REALLOC realloc
#elif TMEM_TYPE == 1
#define CALLOC  T1calloc
#define MALLOC  T1malloc
#define FREE    T1free
#define REALLOC T1realloc
#elif TMEM_TYPE == 2
#define CALLOC  T2calloc
#define MALLOC  T2malloc
#define FREE    T2free
#define REALLOC T2realloc

typedef long msize;
void   * T2malloc (msize n);
void T2free (void *mem);
void   * T2realloc (char * mem, register msize n);
void * T2calloc (msize num, msize size);
#endif

/***************************************************************/
#if TMEM_TYPE == 1
#define T1New(t,n)         ((t*)malloc(sizeof(t)*(n)))
#define T1Realloc(p,t,n)   ((t*)realloc((p),sizeof(t)*(n)))
#define T1Free(p)           free(p)

struct block_info {
    char *  blockptr;
    long    blocksize;
    char *  blockix;        /* Current index into block */
};
struct heap_info {
    struct block_info ** bl;
    long                 blocksize;
    long                 nbblocks;
    long                 blocknum;
};

#define BLOCKSIZE   200000
#define MSIZE       4               /* Size of marker */
#define MSIZE2      8               /* 2 * MSIZE */
#define ALIGN       8               /* Alignment, must by 2^n */
#define ALIGNM1     (ALIGN - 1)     /* Alignment minus 1 */
#define ALIGNMASK   (~(ALIGNM1))    /* Alignment mask */
#define ALIGN_START ((ALIGN>MSIZE)?(ALIGN-MSIZE):(MSIZE))

#define LVAL(ip) (*(MTYPE*)(ip))
static struct heap_info * ghi = 0;
/***************************************************************/
#ifdef UNUSED
static void show_block(struct block_info * bi) {

    long size;
    long ix;
    int done;
    int showix;
    int col;
    char bbuf[100];

    col = 0;
    size = LVAL(bi->blockptr);
    sprintf(bbuf, "[0]=%ld", size);
    if (col + strlen(bbuf) > 72) {printf("\n"); col=0; }
    printf("%s", bbuf);
    col += strlen(bbuf);

    ix = ALIGN_START;
    showix = 0;
    done = 0;
    while (!done) {
        if (ix < 0 || ix + MSIZE > bi->blocksize) {
            printf("\n");
            printf("Bad block structure 1.\n");
            exit(2);
        }

        size = LVAL(bi->blockptr + ix);
        sprintf(bbuf, " [%ld]=%ld", ix, size);
        if (bi->blockptr + ix == bi->blockix) {
            strcat(bbuf, "*");
            showix = 1;
        }
        if (col + strlen(bbuf) > 72) {printf("\n"); col=0; }
        printf("%s", bbuf);
        col += strlen(bbuf);

        if (size > 0) {
            ix += size + MSIZE;
        } else if (size < 0) {
            ix += (MSIZE - size);
        } else {
            done = 1;
        }

        if (!done) {
            if (ix < 0 || ix + MSIZE > bi->blocksize) {
                printf("\n");
                printf("Bad block structure 2.\n");
                exit(2);
            }

            size = LVAL(bi->blockptr + ix);
            sprintf(bbuf, " [%ld]=%ld", ix, size);
            if (col + strlen(bbuf) > 72) {printf("\n"); col=0; }
            printf("%s", bbuf);
            col += strlen(bbuf);
            ix += MSIZE;
        }
    }
    if (!showix) {
        sprintf (bbuf, " [%ld]=%ld* ",
            bi->blockix - bi->blockptr, LVAL(bi->blockix));
        if (col + strlen(bbuf) > 72) {printf("\n"); col=0; }
        printf("%s", bbuf);
        col += strlen(bbuf);
    }
    printf("\n");
}
/***************************************************************/
static int check_block(struct block_info * bi) {

    long size;
    long size2;
    long ix;
    int done;
    int foundix;

    foundix = 0;
    size = LVAL(bi->blockptr);
    if (size) {
        printf("check_block: First marker not NULL.\n");
        return (1);
    }

    ix = ALIGN_START;
    done = 0;
    while (!done) {
        if (ix < 0 || ix + MSIZE > bi->blocksize) {
            printf("check_block: Bad block structure 1.\n");
            return (1);
        }

        if (bi->blockptr + ix == bi->blockix) foundix = 1;

        size = LVAL(bi->blockptr + ix);
        if (size > 0) {
            ix += size + MSIZE;
        } else if (size < 0) {
            ix += (MSIZE - size);
        } else {
            done = 1;
        }

        if (!done) {
            if (ix < 0 || ix + MSIZE > bi->blocksize) {
                printf("check_block: Bad block structure 2.\n");
                return (1);
            }

            size2 = LVAL(bi->blockptr + ix);
            if (size != size2) {
                printf("check_block: Sizes don't match.\n");
                return (1);
            }
            ix += MSIZE;
        }
    }

    if (!foundix) {
        ix = bi->blockix - bi->blockptr;
        size = LVAL(bi->blockptr + ix);
        if (size < 0) {
            ix += (MSIZE2 - size);
            size = LVAL(bi->blockptr + ix);
            if (size > 0) {
                size2 = LVAL(bi->blockptr - size - MSIZE);
                if (size2 != size) {
                    printf("check_block: Check index size mismatch.\n");
                    return (1);
                }
            } else if (size > 0) {
                printf("check_block: Check index not free.\n");
                return (1);
            }
        } else {
            printf("check_block: Check index not used.\n");
            return (1);
        }
    }


    return (0);
}
/***************************************************************/
static int check_heap(struct heap_info * hi) {

    long bnum;
    int badblocks;

    badblocks = 0;
    bnum = hi->blocknum;
    do {
        if (check_block(hi->bl[bnum])) badblocks++;
        bnum--;
        if (bnum < 0) bnum = hi->nbblocks - 1;
    } while (bnum != hi->blocknum);

    return (badblocks);
}
#endif /* UNUSED */
/***************************************************************/
static void * find_block_space(struct block_info * bi, long numbytes) {

    register int found;
    register long size;
    register char * start_ix;
    register long remainder;
    register long memsize;
    register char * head;
    void * ptr;

    memsize = ((numbytes + MSIZE2 + ALIGNM1) & ALIGNMASK) - MSIZE2;

    head = bi->blockix;
    found = 0;
/*
** We need to do one check first before setting start_ix because
** blockix won't be pointing to a place we'll visit again if it's
** in the middle of a free area, as can happen when two blocks
** are combined during a free.
*/
    size = LVAL(head);
    if (size > 0) {
        if (size >= memsize) found = 1;
        else head += size + MSIZE2;
    } else if (size < 0) {
        head += (MSIZE2 - size);
    } else {
        head = bi->blockptr + ALIGN_START;
    }

    if (!found) {
        start_ix = head;
        do {
            size = LVAL(head);
            if (size > 0) {
                if (size >= memsize) found = 1;
                else head += size + MSIZE2;
            } else if (size < 0) {
                head += (MSIZE2 - size);
            } else {
                head = bi->blockptr + ALIGN_START;
            }
        } while (!found && head != start_ix);
    }

    if (found) {
        remainder = size - memsize - MSIZE2;
        if (remainder > MSIZE2) {
            LVAL(head)                    = -memsize;
            LVAL(head + memsize + MSIZE)  = -memsize;
            LVAL(head + memsize + MSIZE2) = remainder;
            LVAL(head + size + MSIZE)     = remainder;
        } else {
            LVAL(head)                    = -size;
            LVAL(head + size + MSIZE)     = -size;
        }
        ptr = head + MSIZE;
        bi->blockix = head;
    } else {
        ptr = 0;
    }

    return (ptr);
}
/***************************************************************/
static struct block_info * new_t1_block(long blocksize) {

    struct block_info * bi;
    long size;
    long bsize;


    bi = T1New(struct block_info, 1);
    if (!bi) return (0);

    bsize = ((blocksize + ALIGNM1) & ALIGNMASK) + MSIZE2;
    bi->blockptr = T1New(char, bsize + ALIGN + MSIZE2);
    if (!bi->blockptr) return (0);

/*
** Align blockptr
*/
    while ((long)bi->blockptr & ALIGNM1) {
        bi->blockptr++;
        bsize--;
    }
    bsize = (bsize & ALIGNMASK) + MSIZE2;
    bi->blocksize = bsize;

    size = bsize - (ALIGN_START + MSIZE + MSIZE2);
    memset(bi->blockptr, 0, ALIGN_START);
    bi->blockix = bi->blockptr + ALIGN_START;
    LVAL(bi->blockix)                   = size;
    LVAL(bi->blockptr + bsize - MSIZE2) = size;
    LVAL(bi->blockptr + bsize - MSIZE)  = 0; /* End of block */

    return (bi);
}
/***************************************************************/
static struct heap_info * new_heap(void) {

    struct heap_info * hi;

    hi = T1New(struct heap_info, 1);
    if (!hi) return (0);

    hi->blocksize = BLOCKSIZE;
    hi->blocknum  = 0;

    hi->bl = T1New(struct block_info *, 1);
    hi->bl[0] = new_t1_block(hi->blocksize);
    if (!hi->bl[0]) return (0);

    hi->nbblocks = 1;

    return (hi);
}
/***************************************************************/
static void * search_blocks(struct heap_info * hi, long size) {

    long bnum;
    void * ptr;

    bnum = hi->blocknum - 1;
    if (bnum < 0) bnum = hi->nbblocks - 1;
    ptr = 0;

    while (!ptr && bnum != hi->blocknum) {
        ptr = find_block_space(hi->bl[bnum], size);
        if (!ptr) {
            bnum--;
            if (bnum < 0) bnum = hi->nbblocks - 1;
        }
    }
    hi->blocknum = bnum;

    return (ptr);
}
/***************************************************************/
static int make_new_block(struct heap_info * hi, long size) {
/*
** Returns 1 if OK, 0 if problems
*/

    long needed;

    hi->bl = T1Realloc(hi->bl, struct block_info *, hi->nbblocks + 1);
    if (!hi->bl) return (0);

    needed = size + ALIGN_START + MSIZE + MSIZE2;
    if (needed <= hi->blocksize) needed = hi->blocksize;
    hi->bl[hi->nbblocks] = new_t1_block(needed);
    if (!hi->bl[hi->nbblocks]) return (0);

    hi->blocknum = hi->nbblocks;
    /* printf("Created block number %ld (%ld).\n", hi->nbblocks, needed); */
    hi->nbblocks++;

    return (1);
}
/***************************************************************/
void * T1malloc(long size) {

    void * ptr;

    if (!ghi) {
        ghi = new_heap();
        if (!ghi) return (0);
    }

    ptr = find_block_space(ghi->bl[ghi->blocknum], size);
    if (!ptr) {
        ptr = search_blocks(ghi, size);
        if (!ptr) {
            if (!make_new_block(ghi, size)) return (0);
            ptr = find_block_space(ghi->bl[ghi->blocknum], size);
        }
    }

    return (ptr);
}
/***************************************************************/
void T1free(void * ptr) {

    register long size;
    register char * head;
    register char * tail;
    register char * tailp;
    register char * headn;

    if (!ptr) return;

    head = (char*)(ptr) - MSIZE;
    tail = head - LVAL(head) + MSIZE;

    headn = tail + MSIZE;
    if (LVAL(headn) > 0) {
        tail = tail + LVAL(headn) + MSIZE2;
        LVAL(headn) = 0; /* Needed in case blockix is here */
    }

    tailp = head - MSIZE;
    if (LVAL(tailp) > 0) {
        LVAL(head) = 0; /* In case blockix is here */
        head = head - LVAL(tailp) - MSIZE2;
    }

    size = (long)(tail - head - MSIZE);
    LVAL(head) = size;
    LVAL(tail) = size;
}
/***************************************************************/
void * T1calloc(long n, long s) {

    void * ptr;
    long size;

    size = n * s;

    ptr = T1malloc(size);
    if (ptr) memset(ptr, 0, size);

    return (ptr);
}
/***************************************************************/
void * T1realloc(void * p, long size) {

    void * ptr;
    long oldsize;

    if (!p) return (T1malloc(size));

    oldsize = -LVAL((char*)(p) - MSIZE);

    if (size > oldsize) {
        ptr = T1malloc(size);
        memcpy(ptr, p, oldsize);
        T1free(p);
    } else {
        ptr = p;
    }

    return (ptr);
}
#endif
/***************************************************************/

#define HASH_TABLE_SIZE 2503

#ifdef HEAP_CHECKING
#define HEAP_CHECK_RATE 1
#endif

#define HEAP_WATCHING 0      /* Turns on 'GOT xx KB' messages */

#ifdef HEAP_CHECKING

#ifndef HEAP_CHECK_TAIL_SIZE
#define HEAP_CHECK_TAIL_SIZE 16
#endif

#ifndef HEAP_CHECK_HEAD_SIZE
#define HEAP_CHECK_HEAD_SIZE 16  /* Must be multiple of 2^? */
#endif

#define HEAP_CHECK_SIZE (HEAP_CHECK_HEAD_SIZE + HEAP_CHECK_TAIL_SIZE)

#ifndef HEAP_CHECK_RATE
#define HEAP_CHECK_RATE 16
#endif

#endif

/***************************************************************/

#if defined(__mpexl) || defined(__hpux)
#define MALLINFO 1
#include <malloc.h>
#endif

static long totnew;
static long totfree;

/*******************************************************************/
void show_mallinfo (FILE *fp) {

#if MALLINFO
    struct mallinfo m;

    m = mallinfo();
    fprintf (fp,"---- mallinfo() ----\n");
    fprintf (fp,"arena    = %d\n", m.arena);
    fprintf (fp,"ordblks  = %d\n", m.ordblks);
    fprintf (fp,"smblks   = %d\n", m.smblks);
    fprintf (fp,"hblkhd   = %d\n", m.hblkhd);
    fprintf (fp,"usmblks  = %d\n", m.usmblks);
    fprintf (fp,"fsmblks  = %d\n", m.fsmblks);
    fprintf (fp,"uordblks = %d\n", m.uordblks);
    fprintf (fp,"fordblks = %d\n", m.fordblks);
    fprintf (fp,"keepcost = %d\n", m.keepcost);
    fprintf (fp,"\n");
    fflush (fp);
#else
    /* fprintf (fp,"---- mallinfo() not available ----\n"); */
    fflush (fp);
#endif
}
/*******************************************************************/
#if CMEM_TYPE == 2
/*******************************************************************/

static long totbytes;
static long totrepow2;
static long totresame;
static long totrenew;
static long hiwater;

static FILE * repfnum;  /* File number of output file for print_mem_stats */

/***************** hash table definitions **********************/

#define hash(value) (abs(value) % (hashtable->hashsize))

typedef
        long hash_t;

typedef
        struct hashrec_p {
                hash_t hashvalue;
                int hashindex;
                void* blockptr;
                struct hashrec_p * nexthash;
                struct hashrec_p * prevhash;
                } hashrec_t;

typedef
        struct hashtable_p {
                long   hashsize;
                hashrec_t ** hasharray;
                } hashtable_t;

typedef
        void print_func(void*);

/***************************************************************/
/*@*/void out_of_memory(char * fname, int linenum, long itemsize) {

    printf("Out of new memory.\n");
    printf("New(%ld) at %s, line %d\n", itemsize,
           fname, linenum);
    printf("Bytes in heap=%ld\n", totbytes);
    show_mallinfo (stdout);
    *((char*)0) = 0;  /* Cause blowout */
    exit(1);
}
/***************************************************************/
/*@*/static void printhashtable(hashtable_t * hashtable,
                                print_func *pr_func)
{
    hashrec_t * hashptr;
    int hashix;

    if (!hashtable) return;

    for (hashix = 0; hashix < hashtable->hashsize; hashix++) {
        hashptr = hashtable->hasharray[hashix];
        while (hashptr != NULL) {
            (*pr_func)(hashptr->blockptr);
            hashptr = hashptr->nexthash;
          }
    }
}
/***************************************************************/
/*@*/static void inithashtable(hashtable_t * hashtable, long hashsize)
{
        int hashix;

        hashtable->hashsize = hashsize;

        hashtable->hasharray =
            (hashrec_t**) MALLOC(hashsize * sizeof(hashrec_t*));
        if (!hashtable->hasharray)
            out_of_memory(__FILE__, __LINE__, sizeof(hashrec_t*));

        for (hashix = 0; hashix < hashtable->hashsize; hashix++)
                hashtable->hasharray[hashix] = NULL;
}
/***************************************************************/
/*@*/static hashtable_t * newhashtable(long hashsize) {

        hashtable_t* hashtable;

        hashtable = (hashtable_t*) MALLOC(sizeof(hashtable_t));
        if (!hashtable)
            out_of_memory(__FILE__, __LINE__, sizeof(hashtable_t));

        inithashtable(hashtable, hashsize);
        return (hashtable);
}
/***************************************************************/
/*@*/static void delhashtable(hashtable_t * hashtable, hashrec_t * hashrec)
{
        if (hashrec->prevhash == NULL)
                hashtable->hasharray[hashrec->hashindex] = hashrec->nexthash;
        else
                hashrec->prevhash->nexthash = hashrec->nexthash;

        if (hashrec->nexthash == NULL)
                /* do nothing, no tail pointer to fix */ ;
        else
                hashrec->nexthash->prevhash = hashrec->prevhash;

        FREE(hashrec);
}
/***************************************************************/
/*@*/static void addhashtable(hashtable_t * hashtable, hash_t value,
                  void *blockptr)
{
        hashrec_t * hashrec;
        int hashix;

        hashrec = (hashrec_t*) MALLOC(sizeof(hashrec_t));
        if (!hashrec)
            out_of_memory(__FILE__, __LINE__, sizeof(hashrec_t));
        hashrec->hashvalue = value;
        hashrec->blockptr  = blockptr;

        hashix = hash(value);
        hashrec->prevhash = NULL;
        hashrec->hashindex = hashix;
        hashrec->nexthash = hashtable->hasharray[hashix];
        if (hashtable->hasharray[hashix] != NULL)
            hashtable->hasharray[hashix]->prevhash = hashrec;
        hashtable->hasharray[hashix] = hashrec;
}
/***************************************************************/
/*@*/static hashrec_t * findhashtable(hashtable_t * hashtable,
                                      hash_t value)
{
        hashrec_t * hashptr;
        int hashix;
        int found;

        hashix = hash(value);
        hashptr = hashtable->hasharray[hashix];
        found = 0;
        while (!found && hashptr != NULL)
                if (hashptr->hashvalue == value)
                    found = 1;
                else
                    hashptr = hashptr->nexthash;
        return hashptr;
}
/***************************************************************/
#ifdef FUNCTION_NOT_USED
/*@*/static void freehashtable(hashtable_t * hashtable) {

        FREE(hashtable->hasharray);

        FREE(hashtable);
}
#endif
/******************** mem table definitions ********************/

typedef
        struct {
                long    ptr;
                char*   fname;
                long    linenum;
                long    numbytes;
                long    numresame; /* Realloc returns same ptr */
                long    numrenew;  /* Realloc returns new ptr */
#ifdef CMEM_CRBY
                int     crby;      /* 1=New, 2=Realloc */
#endif
                } meminfo_t;

static hashtable_t* mhashtable = 0;
static int hdpr;

/***************** heap checking functions *********************/
#ifdef HEAP_CHECKING
#define HEAP_CHECK_VALUE 165

static long num_errors;
/***************************************************************/
/*@*/void mark_heap(void * ptr, size_t size) {

    unsigned char *p;
    long ii;

    /* Mark front of heap area */
    p = ptr;
    for (ii = 0; ii < HEAP_CHECK_HEAD_SIZE; ii++) {
        *(p + ii) = HEAP_CHECK_VALUE;
    }

    /* Mark rear of heap area */
    p = (unsigned char *)ptr + HEAP_CHECK_HEAD_SIZE + size;
    for (ii = 0; ii < HEAP_CHECK_TAIL_SIZE; ii++) {
        *(p + ii) = HEAP_CHECK_VALUE;
    }
}
/***************************************************************/
/*@*/static long check_heap_area(unsigned char * p, long size) {

    long ii;
    long jj;
    long kk;

    for (ii = 0; ii < size; ii++) {
        if (p[ii] != HEAP_CHECK_VALUE) {
            jj = ii + 1;
            while (jj < size && p[jj] != HEAP_CHECK_VALUE) jj++;
            printf("Heap error:");
            for (kk = ii; kk < jj; kk++) {
                printf(" %2x", (int)p[kk]);
            }
            printf("\n");
            return (jj - ii);
        }
    }
    return (0);
}
/***************************************************************/
/*@*/static void checkmeminfo(void* mi) {

    meminfo_t* meminfo;
    unsigned char *p;
    long nberrs = 0;

    meminfo = (meminfo_t*) mi;

    /* Check front of heap area */
    p = (unsigned char*)(meminfo->ptr);
    if (check_heap_area(p, HEAP_CHECK_HEAD_SIZE)) nberrs++;

    /* Check rear of heap area */
    p = (unsigned char*)(meminfo->ptr) + HEAP_CHECK_HEAD_SIZE +
        meminfo->numbytes;
    if (check_heap_area(p, HEAP_CHECK_TAIL_SIZE)) nberrs++;

    if (nberrs) {
        num_errors += nberrs;
        printf("   %d heap errors for pointer created by '%s', line %d\n",
               nberrs, meminfo->fname, meminfo->linenum);
    }
}
/***************************************************************/
/*@*/void check_heap(char* fname, int linenum) {

    static long rate = HEAP_CHECK_RATE;

    rate--;
    if (rate>0) return;

    rate = HEAP_CHECK_RATE;

    num_errors = 0;
    printhashtable(mhashtable, checkmeminfo);
    if (num_errors) {
        printf("%d total heap errors detected at '%s', line %d\n",
               num_errors, fname, linenum);
    }
}
#endif /* #ifdef HEAP_CHECKING */
/***************************************************************/
/*                CMEM_TYPE == 2 Entry points                  */
/***************************************************************/
/*@*/void* TNew(size_t itemsize, size_t numitems, char* fname,
                int linenum) {

    void * ptr;
    meminfo_t * meminfo;
    size_t numbytes;
#ifndef HEAP_CHECKING
    size_t pow2bytes;
#endif

    if (!mhashtable) {
#if MT_MEM
        mem_crit_section = MALLOC(sizeof(CRITSECTYP));
        CRIT_INIT(mem_crit_section);
        CRIT_LOCK(mem_crit_section);
#endif
        mhashtable = newhashtable(HASH_TABLE_SIZE);
        totnew = 0;
        totbytes = 0;
        totrepow2 = 0;
        totresame = 0;
        totrenew = 0;
        totfree = 0;
      } else {
#if MT_MEM
        CRIT_LOCK(mem_crit_section);
#endif
      }

#ifdef HEAP_CHECKING
    check_heap(fname, linenum);
#endif
/*
**  Round size up to next power of 2
*/
    numbytes = itemsize * numitems;
#ifndef HEAP_CHECKING
    pow2bytes = 32;
    while (pow2bytes < numbytes) {
        pow2bytes <<= 1;
    }
    numbytes = pow2bytes;
#endif

    totnew++;
    totbytes += (long)numbytes;
    if (totbytes > hiwater) {
#if HEAP_WATCHING
        if (hiwater >> 10 != totbytes >> 10) {
            printf ("CMEMC: GOT %d KB\n", totbytes >> 10);
            fflush (stdout);
        }
#endif
        hiwater = totbytes;
    }

#ifndef HEAP_CHECKING
    ptr = CALLOC((long)numbytes, 1);
#else
    ptr = CALLOC(numbytes + HEAP_CHECK_SIZE, 1);
#endif

    if (!ptr) {
        printf("Out of new memory.\n");
        printf("New(%ld,%ld) at %s, line %d\n", itemsize, numitems,
               fname, linenum);
        printf("Bytes in heap=%ld\n", totbytes);
        show_mallinfo (stdout);
        *((char*)0) = 0;  /* Cause blowout */
        exit(1);
    }

#ifdef HEAP_CHECKING
    mark_heap(ptr, numbytes);
#endif

    meminfo = (meminfo_t*) MALLOC(sizeof(meminfo_t));
    if (!meminfo)
        out_of_memory(__FILE__,__LINE__,sizeof(meminfo_t));
    meminfo->ptr = (long) ptr;
    meminfo->fname = (char*) MALLOC((long)strlen(fname)+1);
    if (!meminfo->fname)
        out_of_memory(__FILE__,__LINE__, (long)strlen(fname)+1);
    strcpy(meminfo->fname, fname);
    meminfo->linenum = linenum;
    meminfo->numbytes = (long)numbytes;
    meminfo->numresame = 0;
    meminfo->numrenew = 0;
#ifdef CMEM_CRBY
    meminfo->crby     = 1;
#endif

    addhashtable(mhashtable, meminfo->ptr, meminfo);

#if MT_MEM
    CRIT_ULOCK(mem_crit_section);
#endif

#ifdef HEAP_CHECKING
    return ((void*)((char*)ptr + HEAP_CHECK_HEAD_SIZE));
#else
    return (ptr);
#endif
}
/***************************************************************/
/*@*/void TFree(void* ptr, char* fname, int linenum) {

    meminfo_t * meminfo;
    hashrec_t * hashrec;

#if MT_MEM
    CRIT_LOCK(mem_crit_section);
#endif

#ifdef HEAP_CHECKING
    check_heap(fname, linenum);
    if (!ptr) {
#if MT_MEM
        CRIT_ULOCK(mem_crit_section);
#endif
        return;
    };    /* Return if NULL pointer */
    ptr = (void*)((char*)ptr - HEAP_CHECK_HEAD_SIZE);
#else
    if (!ptr) {
#if MT_MEM
        CRIT_ULOCK(mem_crit_section);
#endif
        return;
    };    /* Return if NULL pointer */
#endif

    totfree++;

    hashrec = findhashtable(mhashtable, (long) ptr);
    if (!hashrec) {
        printf("Can't find free pointer at '%s', line %d\n",
               fname, linenum);
        *((char*)0) = 0;  /* Cause blowout */
        exit(1);
    } else {
        meminfo = hashrec->blockptr;
        delhashtable(mhashtable, hashrec);
        totbytes -= meminfo->numbytes;
        FREE(meminfo->fname);
        FREE(meminfo);

        FREE(ptr);
      }

#if MT_MEM
    CRIT_ULOCK(mem_crit_section);
#endif
}
/***************************************************************/
/*@*/void* TRealloc(void * ptr, size_t numbytes, char* fname,
                    int linenum) {

    void      * rptr;
    meminfo_t * meminfo;
    hashrec_t * hashrec;
#ifndef HEAP_CHECKING
    size_t pow2bytes;
#endif

    if (!ptr) {
        return (TNew(numbytes, 1, fname, linenum));
    }

#if MT_MEM
    CRIT_LOCK(mem_crit_section);
#endif

#ifdef HEAP_CHECKING
    check_heap(fname, linenum);
    ptr = (void*)((char*)ptr - HEAP_CHECK_HEAD_SIZE);
#endif

    hashrec = findhashtable(mhashtable, (long) ptr);
/*
**  Round size up to next power of 2
*/
#ifndef HEAP_CHECKING
    pow2bytes = 32;
    while (pow2bytes < numbytes) {
        pow2bytes <<= 1;
    }
    numbytes = pow2bytes;
#endif
/*
**  See if this is same as last REALLOC
*/
    if (!hashrec)
        printf("Can't find REALLOC pointer at '%s', line %d\n",
               fname, linenum);
    else {
        meminfo = hashrec->blockptr;
#ifdef CMEM_CRBY
        if (meminfo->crby == 1) {
printf("******************************************************************\n");
            printf("Realloc at %s : %ld of\n", fname, linenum);
            printf("                           New at %s : %ld\n",
                meminfo->fname, meminfo->linenum);
printf("******************************************************************\n");
            meminfo->crby = 3;
        }
#endif
        if ((size_t)meminfo->numbytes == numbytes) {
            totrepow2++;
#ifdef HEAP_CHECKING
    #if MT_MEM
            CRIT_ULOCK(mem_crit_section);
    #endif
            return ((void*)((char*)ptr + HEAP_CHECK_HEAD_SIZE));
#else
    #if MT_MEM
            CRIT_ULOCK(mem_crit_section);
    #endif
            return (ptr);
#endif
        }
    }

#ifndef HEAP_CHECKING
    rptr = REALLOC(ptr, (long)numbytes);
#else
    rptr = REALLOC(ptr, numbytes + HEAP_CHECK_SIZE);
#endif

    if (!rptr) {
        printf("Out of memory.\n");
        printf("Realloc(%ld) at %s, line %d\n", numbytes, fname, linenum);
        printf("Bytes in heap=%ld\n", totbytes);
        show_mallinfo (stdout);
        *((char*)0) = 0;  /* Cause blowout */
        exit(1);
    }

#ifdef HEAP_CHECKING
    mark_heap(rptr, numbytes);
#endif

    if (!hashrec)
        printf("Can't find realloc pointer at '%s', line %d\n",
               fname, linenum);
    else {
        meminfo = hashrec->blockptr;
        if (rptr == ptr) {
          meminfo->numresame++;
          totresame++;
        } else {
            delhashtable(mhashtable, hashrec);
            meminfo->ptr = (long) rptr;
            meminfo->numrenew++;
            totrenew++;
            addhashtable(mhashtable, meminfo->ptr, meminfo);
          }
        totbytes += (long)numbytes - meminfo->numbytes;
        if (totbytes > hiwater) {
#if HEAP_WATCHING
            if (hiwater >> 10 != totbytes >> 10) {
                printf ("CMEMC: GOT %d KB\n", totbytes >> 10);
                fflush (stdout);
            }
#endif
            hiwater = totbytes;
        }

        meminfo->numbytes = (long)numbytes;
      }

#if MT_MEM
    CRIT_ULOCK(mem_crit_section);
#endif

#ifdef HEAP_CHECKING
    return ((void*)((char*)rptr + HEAP_CHECK_HEAD_SIZE));
#else
    return (rptr);
#endif
}
/***************************************************************/
static void printmeminfo(void* mi) {

    meminfo_t* meminfo;
    char pbuf[20];
    char * fnp;
    int fnl;

    meminfo = (meminfo_t*) mi;

    if (!hdpr) {
        fprintf(repfnum, "%-48.48s", "File Name");
        fprintf(repfnum, "%8s", "Line");
        fprintf(repfnum, "%8s", "Bytes");
        fprintf(repfnum, "%12s","Ptr");
    /*  fprintf(repfnum, "%8s", "ReSame"); */
    /*  fprintf(repfnum, "%8s", "ReNew"); */
        fprintf(repfnum, "\n");
        hdpr = 1;
      }

/*  fprintf(repfnum, "ptr=%8lx", meminfo->ptr); */
    sprintf(pbuf, "0x%lx", meminfo->ptr);

    fnl = (int)strlen(meminfo->fname);
    if (fnl <= 48) {
        fnp = meminfo->fname;
    } else {
        fnp = meminfo->fname + (fnl - 48);
    }

    fprintf(repfnum, "%-48.48s", fnp);
    fprintf(repfnum, "%8ld", meminfo->linenum);
    fprintf(repfnum, "%8ld", meminfo->numbytes);
    fprintf(repfnum, "%12s", pbuf);
 /* fprintf(repfnum, "%8ld", meminfo->numresame); */
 /* fprintf(repfnum, "%8ld", meminfo->numrenew);  */
    fprintf(repfnum, "\n");
}
/***************************************************************/
void print_mem_stats(char *fname) {

/* Get time */
    time_t tp;
    struct tm *ts;
    char   tbuf[40];
    int do_close = 0;

#if MT_MEM
        CRIT_LOCK(mem_crit_section);
#endif

    tp = time(NULL);
    ts = localtime(&tp);
    strftime(tbuf, sizeof(tbuf), "%c", ts);
/* */

    if (fname && strlen(fname)) {
        if (!strcmp(fname, "stdout")) {
            repfnum = stdout;
        } else if (!strcmp(fname, "stderr")) {
            repfnum = stderr;
        } else {
            repfnum = fopen(fname, "w");
            if (!repfnum) {
                printf("Error opening file '%s'\n", fname);
                repfnum = stdout;
            } else {
                do_close = 1;
            }
        }
    } else {
        repfnum = stdout;
    }
if (!totbytes) {
fprintf(repfnum, "-All %d bytes freed with %d frees.\n", hiwater, totfree);
} else {
fprintf(repfnum, "%s\n",tbuf);
fprintf(repfnum, "\n");
fprintf(repfnum, "Number of mallocs not freed           = %ld\n",
        totnew - totfree);
fprintf(repfnum, "Number of mallocs                     = %ld\n", totnew);
fprintf(repfnum, "Number of frees                       = %ld\n", totfree);
fprintf(repfnum, "Number of reallocs prevented by pow2  = %ld\n", totrepow2);
fprintf(repfnum, "Number of reallocs reusing same space = %ld\n", totresame);
fprintf(repfnum, "Number of reallocs using new space    = %ld\n", totrenew);
fprintf(repfnum, "Number of bytes not freed             = %ld\n", totbytes);
fprintf(repfnum, "Number of bytes at high water         = %ld\n", hiwater);
fprintf(repfnum, "\n");

show_mallinfo (repfnum);
}
    hdpr = 0;
    printhashtable(mhashtable, printmeminfo);

    if (fname && strlen(fname)) {
        if (do_close) {
            fclose(repfnum);
            if (totbytes)
                printf("Memory information printed to file %s.\n", fname);
        }
    }
#if MT_MEM
        CRIT_ULOCK(mem_crit_section);
#endif
}
/***************************************************************/
/*@*/static meminfo_t * searchhashtable(hashtable_t * hashtable,
                    void * ptr) {

    hashrec_t * hashptr;
    int hashix;
    meminfo_t * meminfo;

    if (!hashtable) return (0);

    for (hashix = 0; hashix < hashtable->hashsize; hashix++) {
        hashptr = hashtable->hasharray[hashix];
        while (hashptr != NULL) {
            meminfo = hashptr->blockptr;
            if ((long)ptr >= meminfo->ptr &&
                (long)ptr < meminfo->ptr + meminfo->numbytes) {
                return (meminfo);
            }
            hashptr = hashptr->nexthash;
        }
    }

    return (0);
}
/***************************************************************/
char * malloced_by(void * ptr, char * buf) {

    meminfo_t * meminfo;
    hashrec_t * hashrec;

#if MT_MEM
        CRIT_LOCK(mem_crit_section);
#endif

    if (!ptr) {
        sprintf(buf, "Pointer NULL");
    } else {
        hashrec = findhashtable(mhashtable, (long) ptr);
        if (!hashrec) {
            meminfo = searchhashtable(mhashtable, ptr);
            if (!meminfo) {
                sprintf(buf, "Pointer (0x%lx) not malloc'd", (long)ptr);
            } else {
                sprintf(buf,
      "Pointer (0x%lx) is byte %ld of %ld bytes malloc'd by %s line %ld",
                (long)ptr, (long)ptr - meminfo->ptr, meminfo->numbytes,
                meminfo->fname, meminfo->linenum);
            }
        } else {
            meminfo = hashrec->blockptr;
            sprintf(buf, "Pointer (0x%lx) malloc'd by %s line %ld",
                (long)ptr, meminfo->fname, meminfo->linenum);
        }
    }

#if MT_MEM
        CRIT_ULOCK(mem_crit_section);
#endif

    return (buf);
}
/***************************************************************/
long get_unfreed_mem(void) {

    return (totbytes);
}
/***************************************************************/
#endif
#if CMEM_TYPE == 1
/***************************************************************/
/*                CMEM_TYPE == 1 Entry points                  */
/***************************************************************/
/*@*/void out_of_memory(long itemsize) {

    printf("Out of new memory (New %ld).\n", itemsize);
    printf("Number of mallocs=%ld\n", totnew);
    show_mallinfo (stdout);
    *((char*)0) = 0;  /* Cause blowout */
    exit(1);
}
/***************************************************************/
/*@*/void* TNew(size_t itemsize, size_t numitems) {

    void * ptr;

    totnew++;

    ptr = CALLOC(itemsize, numitems);
    if (!ptr) out_of_memory(itemsize * numitems);

    return (ptr);
}
/***************************************************************/
/*@*/void TFree(void* ptr) {

    if (!ptr) return;    /* Return if NULL pointer */

    totfree++;
    FREE(ptr);
}
/***************************************************************/
/*@*/void* TRealloc(void * ptr, size_t numbytes) {

    void      * rptr;
    size_t pow2bytes;

    if (!ptr) {
        rptr = TNew(numbytes, 1);
        return (rptr);
    }

/*
**  Round size up to next power of 2
*/
    pow2bytes = 32;
    while (pow2bytes < numbytes) {
        pow2bytes <<= 1;
    }
    numbytes = pow2bytes;

    rptr = REALLOC(ptr, numbytes);
    if (!rptr) out_of_memory(numbytes);

    return (rptr);
}
/***************************************************************/
void print_mem_stats(char *fname) {

/* Get time */
    time_t tp;
    struct tm *ts;
    char   tbuf[40];
    FILE * repfnum;  /* File number of output file for print_mem_stats */

    tp = time(NULL);
    ts = localtime(&tp);
    strftime(tbuf, sizeof(tbuf), "%c", ts);
/* */

    if (strlen(fname)) {
        repfnum = fopen(fname, "w");
        if (!repfnum) {
            printf("Error opening file '%s'\n", fname);
            repfnum = stdout;
        }
    } else {
        repfnum = stdout;
    }

fprintf(repfnum, "%s\n",tbuf);
fprintf(repfnum, "\n");
fprintf(repfnum, "Number of mallocs not freed           = %ld\n",
        totnew - totfree);
fprintf(repfnum, "Number of mallocs                     = %ld\n", totnew);

    if (strlen(fname)) {
        fclose(repfnum);
        printf("Memory information printed to file %s.\n", fname);
#if CMEM_TYPE == 2
        if (strcmp(fname, "WHMEMGO") && totbytes)
            printf("**** Not all memory freed ****\n");
#endif
    }
}
/***************************************************************/
char * malloced_by(void * ptr) {

    return (0);
}
/***************************************************************/
long get_unfreed_mem(void) {

    return (totnew - totfree);
}
/***************************************************************/
#endif
#if CMEM_TYPE == 0
/***************************************************************/
/*                CMEM_TYPE == 0 Entry points                  */
/***************************************************************/
void print_mem_stats(char *fname) {

    printf("Memory information not available.\n");
}
/***************************************************************/
long get_unfreed_mem(void) { return (0); }
#endif
/***************************************************************/
