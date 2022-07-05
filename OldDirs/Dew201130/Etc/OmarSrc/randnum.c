/***************************************************************/
/*  randnum.c - Random numbers                                 */
/***************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <math.h>

#include "randnum.h"

#define IS_ODD(x)  ((x)&1)          /* units bit of x */
#define MAXINT32    (0x7FFFFFFFL)
#define MOD_DIFF(x,y) (((x)-(y))&((1L<<30)-1)) /* subtraction mod (1L<<30) */

/***************************************************************/
/* Random number generators                                    */
/***************************************************************/
#if KNUTH_INT
/*    This program by D E Knuth is in the public domain and freely copyable
 *    AS LONG AS YOU MAKE ABSOLUTELY NO CHANGES!
 *    It is explained in Seminumerical Algorithms, 3rd edition, Section 3.6
 *    (or in the errata to the 2nd edition --- see
 *        http://www-cs-faculty.stanford.edu/~knuth/taocp.html
 *    in the changes to Volume 2 on pages 171 and following).              */

/*    N.B. The MODIFICATIONS introduced in the 9th printing (2002) are
      included here; there's no backwards compatibility with the original. */

/*    This version also adopts Brendan McKay's suggestion to
      accommodate naive users who forget to call ki_ran_start(seed).          */

/*    If you find any bugs, please report them immediately to
 *                 taocp@cs.stanford.edu
 *    (and you will be rewarded if the bug is genuine). Thanks!            */

/************ see the book for explanations and caveats! *******************/
/************ in particular, you need two's complement arithmetic **********/
#define KI_KK 100                     /* the long lag */
#define KI_LL  37                     /* the short lag */
#define KI_MM (1L<<30)                 /* the modulus */
#define KI_QUALITY 1009 /* recommended quality level for high-res use */
#define KI_TT  70   /* guaranteed separation between streams */

struct kidata {
    int     kid_ran_arr_ix;
    long    kid_ran_arr_buf[KI_QUALITY];
    long    kid_ran_x[KI_KK];
};

void ki_ran_array(struct kidata * kid, int aa[],int n)
{
  register int i,j;
  for (j=0;j<KI_KK;j++) aa[j]=kid->kid_ran_x[j];
  for (;j<n;j++) aa[j]=MOD_DIFF(aa[j-KI_KK],aa[j-KI_LL]);
  for (i=0;i<KI_LL;i++,j++) kid->kid_ran_x[i]=MOD_DIFF(aa[j-KI_KK],aa[j-KI_LL]);
  for (;i<KI_KK;i++,j++) kid->kid_ran_x[i]=MOD_DIFF(aa[j-KI_KK],kid->kid_ran_x[i-KI_LL]);
}

/* the following routines are from exercise 3.6--15 */
/* after calling ki_ran_start, get new randoms by, e.g., "x=ran_arr_next()" */

void ki_ran_start(struct kidata * kid, long seed)
{
  register int t,j;
  long x[KI_KK+KI_KK-1];              /* the preparation buffer */
  register long ss=(seed+2)&(KI_MM-2);
  for (j=0;j<KI_KK;j++) {
    x[j]=ss;                      /* bootstrap the buffer */
    ss<<=1; if (ss>=KI_MM) ss-=KI_MM-2; /* cyclic shift 29 bits */
  }
  x[1]++;              /* make x[1] (and only x[1]) odd */
  for (ss=seed&(KI_MM-1),t=KI_TT-1; t; ) {       
    for (j=KI_KK-1;j>0;j--) x[j+j]=x[j], x[j+j-1]=0; /* "square" */
    for (j=KI_KK+KI_KK-2;j>=KI_KK;j--)
      x[j-(KI_KK-KI_LL)]=MOD_DIFF(x[j-(KI_KK-KI_LL)],x[j]),
      x[j-KI_KK]=MOD_DIFF(x[j-KI_KK],x[j]);
    if (IS_ODD(ss)) {              /* "multiply by z" */
      for (j=KI_KK;j>0;j--)  x[j]=x[j-1];
      x[0]=x[KI_KK];            /* shift the buffer cyclically */
      x[KI_LL]=MOD_DIFF(x[KI_LL],x[KI_KK]);
    }
    if (ss) ss>>=1; else t--;
  }
  for (j=0;j<KI_LL;j++) kid->kid_ran_x[j+KI_KK-KI_LL]=x[j];
  for (;j<KI_KK;j++) kid->kid_ran_x[j-KI_LL]=x[j];
  for (j=0;j<10;j++) ki_ran_array(kid,x,KI_KK+KI_KK-1); /* warm things up */
}
long ki_ran_arr_cycle(struct kidata * kid)
{
  ki_ran_array(kid,kid->kid_ran_arr_buf,KI_QUALITY);
  kid->kid_ran_arr_buf[KI_KK]=-1;
  kid->kid_ran_arr_ix=1;
  return kid->kid_ran_arr_buf[0];
}

void * kiseed (long seed) {

    struct kidata * kid;

    kid = New(struct kidata, 1);
    ki_ran_start(kid, 316277L + seed);      /* sqrt(10) = 3.16227766 */
    kid->kid_ran_arr_buf[0] = -1;           /* Force a cycle */

    return (kid);
}

int kigenint (void * vkid) {

    struct kidata * kid = (struct kidata *)vkid;
    int out;

    out = kid->kid_ran_arr_buf[kid->kid_ran_arr_ix++];
    if (out < 0) out = ki_ran_arr_cycle(kid);

    return (out);
}

void kifree (void * vkid) {

    struct kidata * kid = (struct kidata *)vkid;

    Free(kid);
}

#endif /* KNUTH_INT */

#ifdef KNUTH_FLT
/*    This program by D E Knuth is in the public domain and freely copyable
 *    AS LONG AS YOU MAKE ABSOLUTELY NO CHANGES!
 *    It is explained in Seminumerical Algorithms, 3rd edition, Section 3.6
 *    (or in the errata to the 2nd edition --- see
 *        http://www-cs-faculty.stanford.edu/~knuth/taocp.html
 *    in the changes to Volume 2 on pages 171 and following).              */

/*    N.B. The MODIFICATIONS introduced in the 9th printing (2002) are
      included here; there's no backwards compatibility with the original. */

/*    This version also adopts Brendan McKay's suggestion to
      accommodate naive users who forget to call kf_ranf_start(seed).         */

/*    If you find any bugs, please report them immediately to
 *                 taocp@cs.stanford.edu
 *    (and you will be rewarded if the bug is genuine). Thanks!            */

/************ see the book for explanations and caveats! *******************/
/************ in particular, you need two's complement arithmetic **********/
#define KF_KK 100                     /* the long lag */
#define KF_LL  37                     /* the short lag */
#define KF_TT  70   /* guaranteed separation between streams */
#define MOD_SUM(x,y) (((x)+(y))-(int)((x)+(y)))   /* (x+y) mod 1.0 */
#define KF_QUALITY 1009 /* recommended quality level for high-res use */

struct kfdata {
    int    kfd_ran_arr_ix;
    double kfd_ranf_arr_buf[KF_QUALITY];
    double kfd_ranf_u[KF_KK];           /* the generator state */
};



void kf_ranf_array(struct kfdata * kfd, double aa[], int n)
{
  register int i,j;
  for (j=0;j<KF_KK;j++) aa[j]=kfd->kfd_ranf_u[j];
  for (;j<n;j++) aa[j]=MOD_SUM(aa[j-KF_KK],aa[j-KF_LL]);
  for (i=0;i<KF_LL;i++,j++) kfd->kfd_ranf_u[i]=MOD_SUM(aa[j-KF_KK],aa[j-KF_LL]);
  for (;i<KF_KK;i++,j++) kfd->kfd_ranf_u[i]=MOD_SUM(aa[j-KF_KK],kfd->kfd_ranf_u[i-KF_LL]);
}

/* the following routines are adapted from exercise 3.6--15 */
/* after calling kf_ranf_start, get new randoms by, e.g., "x=ranf_arr_next()" */

//  double ranf_arr_dummy=-1.0, ranf_arr_started=-1.0;
//  double *ranf_arr_ptr=&ranf_arr_dummy; /* the next random fraction, or -1 */


void kf_ranf_start(struct kfdata * kfd, long seed)
{
  register int t,s,j;
  double u[KF_KK+KF_KK-1];
  double ulp=(1.0/(1L<<30))/(1L<<22);               /* 2 to the -52 */
  double ss=2.0*ulp*((seed&0x3fffffff)+2);

  for (j=0;j<KF_KK;j++) {
    u[j]=ss;                                /* bootstrap the buffer */
    ss+=ss; if (ss>=1.0) ss-=1.0-2*ulp;  /* cyclic shift of 51 bits */
  }
  u[1]+=ulp;                     /* make u[1] (and only u[1]) "odd" */
  for (s=seed&0x3fffffff,t=KF_TT-1; t; ) {
    for (j=KF_KK-1;j>0;j--)
      u[j+j]=u[j],u[j+j-1]=0.0;                         /* "square" */
    for (j=KF_KK+KF_KK-2;j>=KF_KK;j--) {
      u[j-(KF_KK-KF_LL)]=MOD_SUM(u[j-(KF_KK-KF_LL)],u[j]);
      u[j-KF_KK]=MOD_SUM(u[j-KF_KK],u[j]);
    }
    if (IS_ODD(s)) {                             /* "multiply by z" */
      for (j=KF_KK;j>0;j--) u[j]=u[j-1];
      u[0]=u[KF_KK];                    /* shift the buffer cyclically */
      u[KF_LL]=MOD_SUM(u[KF_LL],u[KF_KK]);
    }
    if (s) s>>=1; else t--;
  }
  for (j=0;j<KF_LL;j++) kfd->kfd_ranf_u[j+KF_KK-KF_LL]=u[j];
  for (;j<KF_KK;j++) kfd->kfd_ranf_u[j-KF_LL]=u[j];
  for (j=0;j<10;j++) kf_ranf_array(kfd,u,KF_KK+KF_KK-1);  /* warm things up */
}

/* #define ranf_arr_next() (*ranf_arr_ptr>=0? *ranf_arr_ptr++: ranf_arr_cycle()) */
double kf_ranf_arr_cycle(struct kfdata * kfd)
{
  //if (ranf_arr_ptr==&ranf_arr_dummy)
  //  kf_ranf_start(kfd, 314159L); /* the user forgot to initialize */
  kf_ranf_array(kfd, kfd->kfd_ranf_arr_buf,KF_QUALITY);
  kfd->kfd_ranf_arr_buf[KF_KK]=-1;
  kfd->kfd_ran_arr_ix = 1;    /* ranf_arr_ptr=kfd->kfd_ranf_arr_buf+1; */
  return kfd->kfd_ranf_arr_buf[0];
}
double kfgenflt (void * vkfd) {

    struct kfdata * kfd = (struct kfdata *)vkfd;
    double out;

    out = kfd->kfd_ranf_arr_buf[kfd->kfd_ran_arr_ix++];
    if (out < 0) out = kf_ranf_arr_cycle(kfd);

    return (out);
}

#if 0
#include <stdio.h>
int main()
{
  register int m; double a[2009]; /* a rudimentary test */
  kf_ranf_start(310952);
  for (m=0;m<2009;m++) kf_ranf_array(a,1009);
  printf("%.20f\n", ran_u[0]);            /* 0.36410514377569680455 */
     /* beware of buggy printf routines that do not give full accuracy here! */
  kf_ranf_start(310952);
  for (m=0;m<1009;m++) kf_ranf_array(a,2009);
  printf("%.20f\n", ran_u[0]);            /* 0.36410514377569680455 */
  return 0;
}
#endif

#endif /* KNUTH_FLT */













#if SEDGEWICK_INT

#define RS_asize    55
#define RS_m1       10000
#define RS_b        16180321    /* digits of golden ratio 1.61803398875 */
/*
**  RS_asize    Size of state array. 55 is good.
**  RS_m1       Large number. Should be larger than largest random number
**              needed. RS_m1^2 needs to fit in int.
**  RS_b        Random digits with 1 fewer digits than RS_m1^2. Also,
**              the last two digits should be 21.
*/

struct rra {
    int a[RS_asize];
    int rrix;
    long m;
    long m1;
};
/**********************************************************************/
static int rrmult(struct rra * rr, int p, int q) {

    int p1, p0, q1, q0;
    p1 = p / rr->m1; p0 = p % rr->m1;
    q1 = q / rr->m1; q0 = q % rr->m1;

    return (((p0 * q1 + p1 * q0) % rr->m1) * rr->m1 + p0 * p0) % rr->m;
}
/**************************************************************************/
static void * rrseed(int s) {

    int jj;
    struct rra * rr;

    rr = calloc(sizeof(struct rra), 1);
    rr->m1 = RS_m1;
    rr->m = rr->m1 * rr->m1;

    rr->a[0] = s;
    for (jj = 1; jj < RS_asize; jj++) {
        rr->a[jj] = (rrmult(rr, rr->a[jj-1], RS_b) + 1) % rr->m;
    }
    rr->rrix = 0;

    return (rr);
}
/**************************************************************************/
static int rrgen(void * rrv, int r) {

    struct rra * rr = rrv;

    rr->rrix = (rr->rrix + 1) % RS_asize;
    rr->a[rr->rrix] =
        (rr->a[(rr->rrix+23) % RS_asize] + rr->a[(rr->rrix + (RS_asize-1)) % RS_asize]) % rr->m;

    return ((rr->a[rr->rrix] / rr->m1) * r) / rr->m1;
}
/**************************************************************************/
static void rrfree(void * rrv) {

    struct rra * rr = rrv;

    free(rr);
}
/**************************************************************************/
#endif
/**************************************************************************/
#if MS_CRT_LIB
void * csrand (unsigned int seed) {
    long * holdrand;

    holdrand = New(long, 1);
    *holdrand = seed;

    return (holdrand);
}
int crand (void * pholdrand) {
    long * holdrand = pholdrand;
    return((((*holdrand) = (*holdrand) * 214013L + 2531011L) >> 16) & 0x7fff);
}
void cfree (void * pholdrand) {
    long * holdrand = pholdrand;
    Free(holdrand);
}
#endif  /* MS_CRT_LIB */
/**************************************************************************/










#if DE_MASK
/* FAIL- Does not pass chi square test */
struct demrec {
    int dem_ixs[8];
};
/***************************************************************/
unsigned char rand8_8_v0[8][256] = {
    /* 8 sets of 0-255 in completely random order */
    { /*0*/
104,  7,158, 70,172,151,251, 51,226,178, 92,150,247,  2,117, 29,
184, 28,187, 33,160,241,240,164,217,173, 53,169,101,254, 57, 85,
100, 72,224,145, 74,108, 43, 86, 90, 69,210, 55,233,154,200, 14,
 27,211, 39,253, 21,168,193,122,132,242, 68,181,138, 66,176,123,
 20,208,103, 26, 82,115,220,213,144,195, 62,  8,147,116,111,228,
152,148,140,131, 22,120,167,225, 48, 76,252,163,186, 17,  3, 95,
 31,249,190,139,214, 80,  9,146, 41,222,135, 24, 71, 23,  1,166,
204,162,244,141,243,212,238,182,230, 36,192,234,126,227,207,137,
 67, 34, 30, 61,197,231,171, 35,102,  4,136, 15,191,112, 97, 65,
245, 99,118,174,130, 56,153, 37, 52,127,219, 87,239, 83,189,106,
170, 60,175,237, 91,223, 11, 13,221, 64,133, 10, 18, 77,199,209,
 32,250,203,180,155,119,134,202,  6,156,  5, 12,235, 59,105,255,
 58,129,165, 88, 25, 50, 93, 75,246,121,114, 49,107,215, 96, 78,
183,248,159,149, 42, 40, 81,110, 84,196,125, 94,194, 98, 19, 79,
179,  0,205,161,206,124,177,232,188,143,128,218,216,185,157, 63,
 73, 16, 89,142,113, 47,236, 54,109,229, 44,201,198, 45, 38, 46
    },{ /*1*/
115,193, 89,244,229,104, 50,235,176, 22, 96,225,  2,142, 26, 60,
 63,207, 55,217, 71,169,140,108,196, 91,241,213, 53,100,  5,226,
130,141,240,250,134, 33,148,177,190,160,212,155,  8,168, 75,125,
  7, 54,239,228,204,254,203, 81,238, 43,135,137, 97, 98,182,210,
211,123, 82,253,236,175, 68,242,154, 28,112,101, 99,102,219,185,
251, 64,  0, 93,132, 69, 94, 47,151,205,161,122,145, 39,232, 57,
188, 21, 51,189,248,171,192,146, 84,245,162, 49,180, 70,158,181,
 37,246, 85,150,165,166, 35,172, 66,220, 20,179,133,113,  1,170,
 36, 76,202,144, 23,136,124,222, 31,105,187, 52,114, 78, 19,  4,
 29, 40, 62, 80,121, 10, 86, 87,156,183,200, 44,157,198, 46,194,
209,110,111,119, 41,106,  6,117,173,149, 15,221,186, 95,120,174,
 25,129,131, 73, 38,243, 58,184, 16,163,118,178,252, 45, 77, 27,
167,255, 18,  3,234,249,147,164, 83, 14,191, 30, 24,227, 61,138,
247,127,128,208,231,143,199, 13,206,152, 48, 56,159,214,139,197,
109, 42, 67, 17,201,103,223,216, 92, 72,233, 79, 11,215,224,  9,
153, 34,237,116, 90,126, 74, 12,195, 32,230,218, 59, 88, 65,107
    },{ /*2*/
 69, 71,  7,255,111,119,150,207, 98,249,247, 76, 48,221,102,239,
217,226,115,175,156, 35,118,125,103, 26, 12,202,137,166,107,254,
 42,243,233,143,138, 80, 19,173, 54,117,109,200,204,191,253,154,
136, 77,106,176, 99,192,193,142,188,158,172,214, 32,251,212, 64,
225,  8,227, 96,135, 93,101,218,174,151, 25, 17,186,195,157,228,
 11,230, 92,183,105,104,244,246,196,  0, 39, 63, 45,203,114,210,
 87,222,159,165,169, 72, 61,211, 41,250, 55,182,141,128,240,163,
122, 37, 56,248, 30, 70,241,126, 59,127, 21,185,187, 73,  3, 88,
 81,231,224, 83,235,146, 52,145, 43,179,170,130, 66,116, 44, 74,
 13,131, 84,129,208,216,133, 47,178,194,168, 53, 89, 34, 29,205,
 23,121,120,139, 65,124,153,245,199, 46,215,134, 51, 82,197, 31,
234,149,167, 18,237,  2,232, 24, 36, 38,162,  6, 90, 94,147, 49,
152,180,171,  1,110, 68, 91,  9,113,198,164,220,236, 16, 58, 14,
100,  4,201, 79,219,160, 97,161,206,213, 78, 95, 27, 57,177,132,
155, 22,189, 86,223, 10,148, 85, 60,112,108, 75, 67,123, 62,209,
181, 20,140, 40, 33,184, 50,238,  5,190,144,252,242, 15, 28,229
    },{ /*3*/
 49, 28,226,188,171,121, 65,197,205,227,242,221,126,176,160, 56,
 63,108,134,  4,241,181,  0, 54,117, 82,173,177,159,125,209,223,
167, 79,  9,198,206,103, 67,109,210,184,127,195,105,201, 74,192,
129, 13,254,182, 31,233,116,161, 88, 21,183, 97,137, 58, 85, 92,
 96, 55,247, 52,202,252, 99,204,128,115,218,146,163,100,175,131,
 68,178,187,231, 39,  6, 78,122, 40,144,  7,239,189,164, 43,111,
170, 57, 75,208, 64, 94,139,104,  8, 26,220,119, 32,255,234, 33,
 77,244, 59,235, 19, 76,152,253,172,196,112,240,186,169,213, 70,
101, 62, 73,113,156, 61, 41,185, 98, 90,251, 20,212, 38, 44, 53,
232, 29,179, 42,245, 46,207,138, 45,120, 91, 17,132,174,250, 47,
 69,194,237, 15, 80,154,228, 25, 81, 83,124,151, 22,153, 35, 89,
238, 84,200,114,236,217, 95,215,149,166,147,155,248,135,190,123,
211, 34,246, 86,219, 27,141,157,133,249, 48,214, 36,110,191, 50,
165,162,224,193, 16, 18, 66,222,158,243,216, 24, 72, 37,145,142,
 71,107,  1,  3,168,180,143,203, 11, 12, 30, 87, 10,148,229,106,
  5, 60, 14,136,230,130,199,  2, 51, 93,140,225,150,118,102, 23
    },{ /*4*/
170,  3,230, 79, 94, 37,110,130,217,118,  7,119,114,183, 46,167,
250, 33, 78, 65,165,152,122, 53,169,124, 41,131,182,201, 89,137,
 13, 71,142,100,240,107,208,  8,244,226, 48,228,210, 70,251, 88,
 67,129, 69,204,229,159, 92,177,197,242,220,104,133,206,221,243,
255, 59,143,175, 99,218, 19,188,241,176,189, 51,202,149,158,192,
147,  2,252,109,238,128,139,194,235,239,145,199,  0,186,157,214,
 16,233, 97,112,136,236,172, 44,179, 96, 66, 15,146, 17,216, 61,
248, 11, 29, 34, 40,215,231,219, 38,132,193,151,187,150,  6,196,
168,191, 50, 72, 18, 10, 36, 86,209,180,225, 58,245,115,103,105,
 91, 55,134, 31, 25,163, 12, 23,184,213,249, 81,160, 42,101,121,
 54, 28, 21, 14,102,234, 87, 93,205,156, 60,207, 49,106,232,166,
 98, 47,200,185, 76,126, 30,154,211,127,144,237, 57,120, 90, 85,
 75,178,173,254, 83,116,212, 63, 68, 20,  1, 24, 62,161,203, 80,
125, 39,222,108,135, 35,113,141, 27, 64,  9, 82,148,224, 73,162,
140, 84, 95,  4, 32,247,153, 52,117, 45,164, 43,  5,123,111,246,
223, 77,138,174, 26, 22,227,171,253,195, 56,155,190,181,198, 74
    },{ /*5*/
121,215, 37,  1, 72,190,133, 99,253, 43,108,134, 95,146, 80, 59,
192,212, 15, 55, 93,239,182, 12,191, 47, 66,227,196, 81,173,115,
200,124,164,213, 20,197,235,199,236, 97, 70,248,132,102,  9, 36,
246, 85,145,123,247, 92,136,230, 48,127,181, 46,129,152, 39,172,
 71,148,186,175, 38, 54, 60, 33,187,160,170,249,117, 51, 91,176,
  0,220,107,135,242,228,144, 86, 84,208,101,143, 34,225,  7, 73,
201, 79,184,254,223,157, 40,  5, 11,210,100,125, 62, 26,114,243,
250,245, 52,179,103,171,150,104,178,174,233,244,229,232, 94, 14,
 23,240, 10,116, 56,255,177,217,222, 53, 29, 78, 18,218,163,216,
 98,140,112,211,  3, 19, 77,  2,142,221,  6,137, 13,214,161,224,
110, 58,147, 32, 41,106, 22,159,168, 44, 21,130,226,169,138,234,
 87,195,252,166,149, 16, 69, 45,109,189,153,198,122,231,202, 61,
120,  8,193, 28, 83,194, 89, 49,203,165,204,185, 63,154,207, 65,
 17,119, 57,126, 74,206, 31,251, 90,131,139,180, 35, 50,183,241,
 27,188,205, 42, 24, 25,105,158,118, 88,155, 76,237,209,111,162,
 75,  4,219,167, 30,238,113, 82, 64, 68, 96, 67,128,141,151,156
    },{ /*6*/
197,215,149,106,  6,148, 44,160,133, 87,247, 72, 77,136,131, 57,
117, 37,173, 71, 10, 14,  9,163,111, 15,225,171,253,240, 19,208,
203, 26, 42,217,194, 40,235,105,196,190,  5,213,246,156, 48,129,
193, 55,153, 49,114,212,180, 27,122,154,150, 65, 70, 46,142,191,
157, 47,179,  8, 30,224,172,230, 82, 94, 24, 63, 36,201,207,125,
236,109, 34,249,108,200,102,206,245,186,  1,  3, 54,  2,145, 86,
 41,139,166, 90,121,113, 22, 38,168,233,238,116,115,244,218, 39,
 51,181,100, 67, 92, 80,174,177,135,134, 66, 62, 25, 89, 97,127,
 16, 29,189, 64,195,226,  7,  0,128,159,144,221, 68,192, 58, 33,
124,132,216, 93, 28,141, 31,222,175,119,254, 78,202,120,158,242,
 60, 76,228,248,161,199, 85, 69,183, 73,214,205,178, 20,176,137,
223,169, 83,167, 61, 74,104,182,250, 45, 13, 32, 18,239,227, 35,
255, 59,229,147,101,126,241, 17,209,118,184, 12, 53,103,  4,252,
 79, 75,204,185,138, 95,146, 91,151,123, 81, 50,237,232,188, 21,
 56,231, 11,210,251,165, 23,187, 99,211, 98,219, 88,107, 84,143,
 43,234,155,140, 96,198,110, 52,170,112,243,162,164,152,130,220
    },{ /*7*/
169,175,  1, 96,189,173,139,242,109, 47,130, 93,160, 70,171,249,
  5,134, 68, 10,237,132,247,102, 73, 11,205,211, 46,213, 37, 13,
104, 97,210,231, 49, 28,146, 29,221,128,219,201,  9, 98,172,143,
 59, 18,245, 63,115, 86, 91,  6,230,180, 39,  0,105,123,252, 34,
232, 77,158,162,177,194,248, 89, 88,141, 21,202, 78,135,120,129,
 43,195, 52,204, 95, 16,170,250,235,184,107,224,234,118, 14,181,
144,233, 36,244,203, 51, 20, 33,240, 12,159, 23,150,155,114, 30,
 32,145, 56, 62,222,117,106,191,166,185,112,236, 87,241,168,238,
214, 85,116, 90,133,122,200, 69,251, 80,149,154,208, 94,206, 84,
125, 79,148,126,182,216,176, 48, 66, 92,110,151,186, 74, 82,254,
212,119,147,209,131,108,156,111,161,153,207, 72, 40,100,121,  7,
136, 76,220,199,243,228,197, 25, 38,152,140, 26,179,137, 54,165,
239,  8, 99,  4, 27, 81,127,187, 17,255, 31, 58,190,157, 42, 55,
124,215, 75,246,138, 41, 45, 60, 50, 61, 57,218, 64, 24,101,  2,
178,192,198,142, 35,227,  3,113,229, 22,226, 53, 65, 67, 19,225,
 15,164,188,163,183,253,196,167,217, 83,223, 71,193, 44,174,103
    }
};
/**************************************************************/
void gen_mask64_0(
    struct demrec * dem,
    unsigned char * outbuf,
    int outbytes)
{
    int ii;
    int jj;
    int carry;
    unsigned char m7;

    /* calculate m7 */
    m7 = rand8_8_v0[0][dem->dem_ixs[0]];
    for (jj = 1; jj < 7; jj++) {
        m7 ^= rand8_8_v0[jj][dem->dem_ixs[jj]];
    }

    for (ii = 0; ii < outbytes; ii++) {
        outbuf[ii] = m7 ^ rand8_8_v0[7][dem->dem_ixs[7]];

        jj = 7;
        /* carry = (outbuf[ii] & 15); */
        do {
            /* dem->dem_ixs[jj] += 1 + carry; */
            dem->dem_ixs[jj] += 1;
            carry = 0;
            if (dem->dem_ixs[jj] >= 256) {
                dem->dem_ixs[jj] -= 256;
                jj--;
                if (jj >= 0) carry = 1;
            }
        } while (carry);
        if (jj < 7) {
            /* calculate m7 */
            m7 = rand8_8_v0[0][dem->dem_ixs[0]];
            for (jj = 1; jj < 7; jj++) {
                m7 ^= rand8_8_v0[jj][dem->dem_ixs[jj]];
            }
        }
    }
}
/**************************************************************/
static void * demseed(int s) {

    int jj;
    struct demrec * dem;

    dem = calloc(sizeof(struct demrec), 1);
    memset(dem->dem_ixs, 0, 0);
    jj = s;
    dem->dem_ixs[7] = (jj & 255); jj >>= 8;
    dem->dem_ixs[6] = (jj & 255); jj >>= 8;
    dem->dem_ixs[5] = (jj & 255); jj >>= 8;
    dem->dem_ixs[4] = (jj & 255); jj >>= 8;

    return (dem);
}
/**************************************************************/
static int demgen(void * vdem, int r) {

    struct demrec * dem = vdem;
    int num;
    int out;

    gen_mask64_0(dem, (char*)(&num), 4);
    if (num < 0) out = (~num) % r;
    else         out = num % r;

    return (out);
}
/**************************************************************/
static void demfree(void * vdem) {

    struct demrec * dem = vdem;

    free(dem);
}
/**************************************************************/
#endif  /* DE_MASK */


#if PSDES_INT

struct pda {
    int pda_idums;
    int pda_idum;
};

/*
** Found longer c1[] and c2[] on internet.  Supports NITER up to 20.

http://meru.cs.missouri.edu/courses/cs4610/projects/ws01/airwolf/source/RandomGenerator.cpp

const long RandomGenerator::xormix1[] = {
                0xbaa96887, 0x1e17d32c, 0x03bcdc3c, 0x0f33d1b2,
                0x76a6491d, 0xc570d85d, 0xe382b1e3, 0x78db4362,
                0x7439a9d4, 0x9cea8ac5, 0x89537c5c, 0x2588f55d,
                0x415b5e1d, 0x216e3d95, 0x85c662e7, 0x5e8ab368,
                0x3ea5cc8c, 0xd26a0f74, 0xf3a9222b, 0x48aad7e4};

const long RandomGenerator::xormix2[] = {
                0x4b0f3b58, 0xe874f0c3, 0x6955c5a6, 0x55a7ca46,
                0x4d9a9d86, 0xfe28a195, 0xb1ca7865, 0x6b235751,
                0x9a997a61, 0xaa6e95c8, 0xaaa98ee1, 0x5af9154c,
                0xfc8e2263, 0x390f5e8c, 0x58ffd802, 0xac0a5eba,
                0xac4874f6, 0xa9df0913, 0x86be4c74, 0xed2c123b};

RandomGenerator::RandomGenerator( long iseed, long iindex, long ireps )
{
	seed = iseed + 1024;
	index = iindex;

	reps = ireps;
}

unsigned long RandomGenerator::Long()
{
    unsigned long hiword, loword, hihold, temp, itmpl, itmph, i;

    loword = seed;
    hiword = index++;

    for (i = 0; i < reps; i++) 
	{
        hihold  = hiword;                           // save hiword for later
        temp    = hihold ^  xormix1[i];             // mix up bits of hiword
        itmpl   = temp   &  0xffff;                 // decompose to hi & lo
        itmph   = temp   >> 16;                     // 16-bit words
        temp    = itmpl * itmpl + ~(itmph * itmph); // do a multiplicative mix
        temp    = (temp >> 16) | (temp << 16);      // swap hi and lo halves
        hiword  = loword ^ ((temp ^ xormix2[i]) + itmpl * itmph); //loword mix
        loword  = hihold;                           // old hiword is loword
    }

	return hiword;
}
*/

#define NITER 4

void psdes(unsigned long *lword, unsigned long *irword)
{
	unsigned long i,ia,ib,iswap,itmph=0,itmpl=0;
	static unsigned long c1[NITER]={
		0xbaa96887L, 0x1e17d32cL, 0x03bcdc3cL, 0x0f33d1b2L};
	static unsigned long c2[NITER]={
		0x4b0f3b58L, 0xe874f0c3L, 0x6955c5a6L, 0x55a7ca46L};

	for (i=0;i<NITER;i++) {
		ia=(iswap=(*irword)) ^ c1[i];
		itmpl = ia & 0xffff;
		itmph = ia >> 16;
		ib=itmpl*itmpl+ ~(itmph*itmph);
		*irword=(*lword) ^ (((ia = (ib >> 16) |
			((ib & 0xffff) << 16)) ^ c2[i])+itmpl*itmph);
		*lword=iswap;
	}
}
/**************************************************************************/
static int pdgenint(void * pdv, int r) {

    struct pda * pd = pdv;
	unsigned int irword,lword;

	if (pd->pda_idum < 0) {
		pd->pda_idums = -(pd->pda_idum);
		pd->pda_idum=1;
	}

	irword=pd->pda_idums;
	lword=pd->pda_idum;
	psdes(&lword,&irword);
    pd->pda_idum += 1;

    return (irword % r);
}
/**************************************************************************/
static void * pdseed(int s) {

    struct pda * pd;

    pd = calloc(sizeof(struct pda), 1);
	pd->pda_idums=1;
	pd->pda_idum=s;

    return (pd);
}
/**************************************************************************/
static void pdfree(void * pdv) {

    struct pda * pd = pdv;

    free(pd);
}
#if 0
float ran4(long *idum)
{
	unsigned long irword,itemp,lword;
	static long idums = 0;

	static unsigned long jflone = 0x3f800000;
	static unsigned long jflmsk = 0x007fffff;

	if (*idum < 0) {
		idums = -(*idum);
		*idum=1;
	}
	irword=(*idum);
	lword=idums;
	psdes(&lword,&irword);
	itemp=jflone | (jflmsk & irword);
	++(*idum);
	return (*(float *)&itemp)-1.0;
}
#endif

#endif  /* PSDES_INT */











/**************************************************************************/
struct randint_info {
    int      rntype;
    void   * rvp;
};
/**************************************************************************/
rand_seed_t randoriginalseed(void) {

    rand_seed_t rseed;
    time_t tm;

    time(&tm);
    rseed = (rand_seed_t)tm;
    rseed ^= (rand_seed_t)(0x5A7E936C);

    return (rseed);
}
/**************************************************************************/
void * randintseed(int rntype, rand_seed_t r_s) {

    struct randint_info * rp;
    rand_seed_t rseed;

    rp = New(struct randint_info, 1);
    rp->rntype = rntype;
    /* printf("randintseed(%d,%d)\n", rntype, r_s); */

    rseed = r_s;
    if (rseed == -1) rseed = randoriginalseed();

    switch (rntype) {
        case RANDNUM_GENINT_0 :  rp->rvp = csrand(rseed);   break;
        case RANDNUM_GENINT_1 :  rp->rvp = rrseed(rseed);   break;
        case RANDNUM_GENINT_2 :  rp->rvp = kiseed(rseed);   break;
        case RANDNUM_GENINT_3 :  rp->rvp = pdseed(rseed);   break;
#ifdef RANDNUM_GENINT_4
        case RANDNUM_GENINT_4 :  rp->rvp = demseed(rseed);  break;
#endif
        default               :  rp = NULL;                 break;
    }

    return (rp);
}
/**************************************************************************/
int randintbetween(void * rvp, int loval, int hival) {

    struct randint_info * rp = (struct randint_info *) rvp;
    int range;
    int out;

    range = hival - loval + 1;

    if (range <= 1) {
        if (range == 1) {
            out = loval;
        } else {
            out = -1;
        }
    } else {
        switch (rp->rntype) {
            case RANDNUM_GENINT_0 :
                out = crand(rp->rvp) % range + loval;
                break;

            case RANDNUM_GENINT_1 :
                out = rrgen(rp->rvp, range) + loval;
                break;

            case RANDNUM_GENINT_2 :
                out = kigenint(rp->rvp) % range + loval;
                break;

            case RANDNUM_GENINT_3 :
                out = pdgenint(rp->rvp, range) + loval;
                break;

#ifdef RANDNUM_GENINT_4
            case RANDNUM_GENINT_4 :
                out = demgen(rp->rvp, r);
                break;
#endif

            default            :
                break;
        }
    }

    return (out);
}
/**************************************************************************/
int randintnext(void * rvp) {

    struct randint_info * rp = (struct randint_info *) rvp;
    int out;

    switch (rp->rntype) {
        case RANDNUM_GENINT_0 :
            out = crand(rp->rvp);
            break;

        case RANDNUM_GENINT_1 :
            out = rrgen(rp->rvp, MAXINT32);
            break;

        case RANDNUM_GENINT_2 :
            out = kigenint(rp->rvp);
            break;

        case RANDNUM_GENINT_3 :
            out = pdgenint(rp->rvp, MAXINT32);
            break;

        default            :
            break;
    }

    return (out);
}
/**************************************************************************/
void randintfree(void * rvp) {

    struct randint_info * rp = (struct randint_info *) rvp;

    switch (rp->rntype) {
        case RANDNUM_GENINT_0 :  cfree(rp->rvp);            break;
        case RANDNUM_GENINT_1 :  rrfree(rp->rvp);           break;
        case RANDNUM_GENINT_2 :  kifree(rp->rvp);           break;
        case RANDNUM_GENINT_3 :  pdfree(rp->rvp);           break;
#ifdef RANDNUM_GENINT_4
        case RANDNUM_GENINT_4 :  demfree(rp->rvp);          break;
#endif
        default               :                             break;
    }

    Free(rp);
}
/**************************************************************************/
#if RANDNUM_TESTING
#define F_max   2200

double chisquare(void * rr, int N, int r) {

    int ii;
    int jj;
    double tt;
    int rn;
    double csv;
    int ff[F_max];

    for (jj = 0; jj < r; jj++) ff[jj] = 0;

    for (ii = 0; ii < N; ii++) {
        rn = randintbetween(rr, 0, r - 1);
        if (rn < 0 || rn >= r) {
            printf("random number %d is outside of range %d\n", rn, r);
        }
        ff[rn] += 1;
    }


#if 1
    for (ii = 0; ii < (r + 9) / 10; ii++) {
        for (jj = 0; jj < 10; jj++) {
            if (ii * 10 + jj < r) {
                printf("%6d ", ff[ii * 10 + jj]);
            }
        }
        printf("\n");
    }
#endif

    tt = 0;
    for (ii = 0; ii < r; ii++) {
        tt += (double)ff[ii] * (double)ff[ii];
    }

    csv = ((double)tt * (double)r / (double)N) - (double)N;

    return (csv);
}
/**************************************************************************/
void chisquaretest(void * rr, int N, int r) {

    double csv;
    double csvl;
    struct proctimes pt_start ;
    struct proctimes pt_end   ;
    struct proctimes pt_result;
   
    start_cpu_timer(&pt_start, 0);
    csv = chisquare(rr, N, r);
    stop_cpu_timer(&pt_start, &pt_end, &pt_result);
    printf("Elapsed time for %d loops: %d.%03d seconds\n",
        N, pt_result.pt_wall_time.ts_second, pt_result.pt_wall_time.ts_microsec / 1000);
    printf("chi square=%g\n", csv);

    csvl = 2.0 * sqrt((double)r);
    printf("chi square range is %g to %g\n", (double)r - csvl, (double)r + csvl);

    if (fabs((double)r - csv) < csvl) {
        printf("chi square is within limit\n");
    } else {
        printf("**** chi square is outside limit ****\n");
    }
}
/**************************************************************************/
int randinttest(int rntype, rand_seed_t s) {

    void * rr;

    rr = randintseed(rntype, s);

    chisquaretest(rr, 16 * 1024 * 1024, 100);

    randintfree(rr);

    return (0);
}
/**************************************************************************/
#if 0 /* was RANDNUM_FLOAT */
int fltmain() {

    void * rr;
    int rn;
    int ii;

    rr = randintseed(RANDNUM_GENINT_0, 52);
    rn =  randintgen(rr, 0);
    if (rn != 208) {
        printf("*ERROR* randintgen=%d\n", rn);
    }
    randintfree(rr);
    /**************************************/
    rr = randintseed(RANDNUM_GENINT_2, 310952L);
    rn =  randintgen(rr, 0);

    if (rn == 708622036L) {
        printf("randintgen=%d\n", rn);
    } else {
        printf("*ERROR* randintgen=%d\n", rn);
    }

    for (ii = 0; ii < 480; ii++) {
        if (!(ii % 6)) printf("\n");
        printf("%10d ", randintgen(rr, 0));
    }
    printf("\n");

    randintfree(rr);
    /**************************************/
    rr = randintseed(RANDNUM_GENINT_1, 310952L);
    rn =  randintgen(rr, 0);

    if (rn == 12560) {
        printf("randintgen=%d\n", rn);
    } else {
        printf("*ERROR* randintgen=%d\n", rn);
    }
    randintfree(rr);
    /**************************************/

    return (0);
}
/**************************************************************************/
void sort2desc(int * nums) {

    int tt;

    if (nums[0] < nums[1]) {
        tt = nums[0];
        nums[0] = nums[1];
        nums[1] = tt;
    }
}
/**************************************************************************/
void sort3desc(int * nums) {

    sort2desc(nums);
    sort2desc(nums + 1);
    sort2desc(nums);
}
/**************************************************************************/
void sort4desc(int * nums) {

    sort2desc(nums);
    sort2desc(nums + 2);

    if (nums[0] < nums[2]) {
        if (nums[0] < nums[3]) {
        } else {
        }
    } else {
    }
    sort2desc(nums + 1);
    sort2desc(nums + 2);
    sort2desc(nums);
    sort2desc(nums + 2);
}
/**************************************************************************/
void sorttest(void) {

    #define NN 4

    int ii;
    int nums[NN] = {1, 2, 3, 4};

    sort4desc(nums);
    for (ii = 0; ii < NN; ii++) printf("%d ", nums[ii]);
    printf("\n");
}
/**************************************************************************/
int rand_main() {

    char bbuf[100];

    sorttest();

    printf("Continue...");
    fgets(bbuf, sizeof(bbuf), stdin);

    return (0);
}
#endif  /* RANDNUM_FLOAT */
#endif  /* RANDNUM_TESTING */
/**************************************************************************/
/* floating randum numbers                                                */
/**************************************************************************/
struct randflt_info { /* rfi_ */
    int          rfi_rntype;
    void       * rfi_rvp;
	int         rfi_gasdev_iset;
    double      rfi_gasdev_gset;
};
/**************************************************************************/
void * kfseed (long seed) {

    struct kfdata * kfd;

    kfd = New(struct kfdata, 1);
    kf_ranf_start(kfd, 316277L + seed);       /* sqrt(10) = 3.16227766 */
    kfd->kfd_ranf_arr_buf[0] = -1;           /* Force a cycle */

    return (kfd);
}
/**************************************************************************/
void kffree (void * vkfd) {

    struct kfdata * kfd = (struct kfdata *)vkfd;

    Free(kfd);
}
/**************************************************************************/
/* Numerical Recipes in C ran1.c                                          */
/**************************************************************************/

#define NR1_IA 16807
#define NR1_IM 2147483647
#define NR1_AM (1.0/NR1_IM)
#define NR1_IQ 127773
#define NR1_IR 2836
#define NR1_NTAB 32
#define NR1_NDIV (1+(NR1_IM-1)/NR1_NTAB)
#define NR1_EPS 1.2e-7
#define NR1_RNMX (1.0-NR1_EPS)

struct nr1data {
    long   nr1d_idnum;
    long   nr1d_iy;
    long   nr1d_iv[NR1_NTAB];
};

double numrec_ran1(struct nr1data * nr1d)
{
	int j;
	long k;
	double temp;

	if (nr1d->nr1d_idnum <= 0 || !nr1d->nr1d_iy) {
		if (-(nr1d->nr1d_idnum) < 1) nr1d->nr1d_idnum=1;
		else nr1d->nr1d_idnum = -(nr1d->nr1d_idnum);
		for (j=NR1_NTAB+7;j>=0;j--) {
			k=(nr1d->nr1d_idnum)/NR1_IQ;
			nr1d->nr1d_idnum=NR1_IA*(nr1d->nr1d_idnum-k*NR1_IQ)-NR1_IR*k;
			if (nr1d->nr1d_idnum < 0) nr1d->nr1d_idnum += NR1_IM;
			if (j < NR1_NTAB) nr1d->nr1d_iv[j] = nr1d->nr1d_idnum;
		}
		nr1d->nr1d_iy=nr1d->nr1d_iv[0];
	}
	k=(nr1d->nr1d_idnum)/NR1_IQ;
	nr1d->nr1d_idnum=NR1_IA*(nr1d->nr1d_idnum-k*NR1_IQ)-NR1_IR*k;
	if (nr1d->nr1d_idnum < 0) nr1d->nr1d_idnum += NR1_IM;
	j=nr1d->nr1d_iy/NR1_NDIV;
	nr1d->nr1d_iy=nr1d->nr1d_iv[j];
	nr1d->nr1d_iv[j] = nr1d->nr1d_idnum;
	if ((temp=NR1_AM*nr1d->nr1d_iy) > NR1_RNMX) return NR1_RNMX;
	else return temp;
}
/**************************************************************************/
void * nrran1seed (long seed) {

    struct nr1data * nr1d;

    nr1d = New(struct nr1data, 1);
    nr1d->nr1d_iy = 0;
    nr1d->nr1d_idnum = -(seed + 123443L);  /* New seed must be < 0 */

    return (nr1d);
}
double nrran1rand (void * vnr1d) {
    struct nr1data * nr1d = (struct nr1data *)vnr1d;

    return numrec_ran1(nr1d);
}
void nrran1free (void * vnr1d) {
    struct nr1data * nr1d = (struct nr1data *)vnr1d;
    Free(nr1d);
}
/**************************************************************************/
void * randfltseed(int rntype, rand_seed_t r_s) {

    struct randflt_info * rfi;
    rand_seed_t rseed;

    rfi = New(struct randflt_info, 1);
    rfi->rfi_rntype = rntype;
    rfi->rfi_gasdev_iset = 0;
    /* printf("randfltseed(%d,%d)\n", rntype, r_s); */

    rseed = r_s;
    if (rseed == -1) rseed = randoriginalseed();

    switch (rntype) {
        case RANDNUM_GENFLT_2 :  rfi->rfi_rvp = kfseed(rseed);      break;
        case RANDNUM_GENFLT_5 :  rfi->rfi_rvp = nrran1seed(rseed);  break;
        default               :  rfi = NULL;                        break;
    }

    return (rfi);
}
/**************************************************************************/
void randfltfree(void * vrfi) {

    struct randflt_info * rfi = (struct randflt_info *) vrfi;

    switch (rfi->rfi_rntype) {
        case RANDNUM_GENFLT_2 :  kffree(rfi->rfi_rvp);           break;
        case RANDNUM_GENFLT_5 :  nrran1free(rfi->rfi_rvp);           break;
        default               :                             break;
    }

    Free(rfi);
}
/**************************************************************************/
double randfltnext(void * vrfi) {

    struct randflt_info * rfi = (struct randflt_info *) vrfi;
    double out;

    switch (rfi->rfi_rntype) {
        case RANDNUM_GENFLT_2 :
            out = kfgenflt(rfi->rfi_rvp);
            break;

        case RANDNUM_GENFLT_5 :
            out = nrran1rand(rfi->rfi_rvp);
            break;

        default            :
            break;
    }

    return (out);
}
/**************************************************************************/
/* Numerical Recipes in C gasdev.c                                        */
/**************************************************************************/
#ifdef NUMREC_GASDEV

#include <math.h>

float gasdev(long *idum)
{
	float ran1(long *idum);
	static int iset=0;
	static float gset;
	float fac,rsq,v1,v2;

	if (*idum < 0) iset=0;
	if  (iset == 0) {
		do {
			v1=2.0*ran1(idum)-1.0;
			v2=2.0*ran1(idum)-1.0;
			rsq=v1*v1+v2*v2;
		} while (rsq >= 1.0 || rsq == 0.0);
		fac=sqrt(-2.0*log(rsq)/rsq);
		gset=v1*fac;
		iset=1;
		return v2*fac;
	} else {
		iset=0;
		return gset;
	}
}
#endif
/**************************************************************************/
double randfltnextgauss(void * vrfi) {

    struct randflt_info * rfi = (struct randflt_info *) vrfi;
    double out;

	double fac,rsq,v1,v2;

	if  (rfi->rfi_gasdev_iset == 0) {
		do {
			v1=2.0*randfltnext(rfi)-1.0;
			v2=2.0*randfltnext(rfi)-1.0;
			rsq=v1*v1+v2*v2;
		} while (rsq >= 1.0 || rsq == 0.0);
		fac=sqrt(-2.0*log(rsq)/rsq);
		rfi->rfi_gasdev_gset=v1*fac;
		rfi->rfi_gasdev_iset=1;
		out = v2*fac;
	} else {
		rfi->rfi_gasdev_iset=0;
		out = rfi->rfi_gasdev_gset;
	}

    return (out);
}
/**************************************************************************/
