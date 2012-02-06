#ifndef THREADS_FIXEDPOINT_H
#define THREADS_FIXEDPOINT_H

typedef int32_t fixed;
typedef int32_t fp_int; 
typedef int64_t fixed_large;
// 31 bit signed integer + 1 sign bit
#define FP_COUNT 14
// sign bit, 17bits . 14 bits

static fp_int const _fp_f = 0x1<<(FP_COUNT-1);
#define FP_F _fp_f
// FP_F = 2^FP_COUNT

inline fixed fp_fromint(fp_int n);
inline fp_int fp_floor(fixed x);
inline fp_int fp_round(fixed x);
inline fixed fp_inc(fixed x);
inline fixed fp_add(fixed x, fixed y);
inline fixed fp_sub(fixed x, fixed y);
inline fixed fp_mul(fixed x, fixed y);
inline fixed fp_div(fixed x, fixed y);
inline fixed fp_addi(fixed x, fp_int n);
inline fixed fp_subi(fixed x, fp_int n);
inline fixed fp_muli(fixed x, fp_int n);
inline fixed fp_divi(fixed x, fp_int n);


/**
 * FP_FROMINT(N)
 * convert fp_int N to fixed
 * return fixed
 **/
 


#define FP_FROMINT(N) (N) * FP_F


/**
 * FP_FLOOR(X)
 * round fixed X to 0
 * return fp_int
 **/
 

#define FP_FLOOR(X) (X) / FP_F

/**
 * FP_ROUND(X)
 * round fixed X to nearest
 * return fp_int
 **/

 
#define FP_ROUND(X) ((X) < 0) ? ((X) - (FP_F>>2))/FP_F : ((X) + (FP_F>>2))/FP_F



#define FP_INC(X) (X) + FP_F

/**
 * FP_OP(X,Y)
 * fixed X OP fixed Y
 * return fixed
 **/

#define FP_ADD(X,Y) (X) + (Y)


#define FP_SUB(X,Y) (X) - (Y)


#define FP_MUL(X,Y) ((fixed_large) (X)) * (Y) / FP_F

#define FP_DIV(X,Y) ((fixed_large) (X)) * FP_F / (Y)

/**
 * FP_OP(X,N)
 * fixed x OP fp_int n
 * return fixed
**/

#define FP_ADDI(X,N) (X) + (N) * FP_F


#define FP_SUBI(X,N) (X) - (N) * FP_F


#define FP_MULI(X,N) (X) * (N)

#define FP_DIVI(X,N) (X) / (N)

#endif /* threads/fixedpoint.h */