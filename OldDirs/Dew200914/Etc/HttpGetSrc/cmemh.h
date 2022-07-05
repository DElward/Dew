/*
**  CMEMH  --  Header file for Memory Manager
*/
/***************************************************************/
#ifndef CMEMH_INCLUDED
#define CMEMH_INCLUDED
/*
**      CMEM_TYPE = 0  Straight calls, no Taurus
**      CMEM_TYPE = 1  Minimum overhead, no memory tracking
**      CMEM_TYPE = 2  Use memory tracking (old default)
*/

#ifdef _DEBUG
#  define CMEM_TYPE 2
#else
#  define CMEM_TYPE 0
#endif

/*
** The Networking includes on AIX define Free in net/radix.h
*/

#ifdef Free
#undef Free
#endif

#if CMEM_TYPE == 0
#define New(t,n)         ((t*)calloc(sizeof(t),(n)))
#define Realloc(p,t,n)   ((t*)realloc((p),sizeof(t)*(n)))
#define Free(p)           free(p)
#elif CMEM_TYPE == 1
#define New(t,n) (t*)TNew(sizeof(t), (n))
#define Free(p) TFree(p)
#define Realloc(p,t,n) TRealloc(p, sizeof(t) * (n))
void* TNew(size_t itemsize, size_t numitems);
void TFree(void* ptr);
void* TRealloc(void * ptr, size_t numbytes);
#else
#define New(t,n) (t*)TNew(sizeof(t), (n), __FILE__, __LINE__)
#define Free(p) TFree(p, __FILE__, __LINE__)
#define Realloc(p,t,n) TRealloc(p, sizeof(t) * (n), __FILE__, __LINE__)
void* TNew(size_t itemsize, size_t numitems, char* fname, int linenum);
void TFree(void* ptr, char* fname, int linenum);
void* TRealloc(void * ptr, size_t numbytes, char* fname, int linenum);
#endif
void print_mem_stats(char *fname);
char * malloced_by(void * ptr, char * buf);
/* Set_Mem_Error procedure to handle memory errors */
/***************************************************************/
/* #define Strdup(s)        (strcpy(New(char,strlen(s)+1),(s))) */
long mem_not_freed(void);
/***************************************************************/
#endif /* CMEMH_INCLUDED */
/***************************************************************/
