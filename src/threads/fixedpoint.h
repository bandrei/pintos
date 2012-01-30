typedef fixed int32_t
typedef fp_int int32_t 
typedef fixed_large int64_t
// 31 bit signed integer + 1 sign bit
#define FP_COUNT 14
// sign bit, 17bits . 14 bits
#define FP_F 0x1<<(FP_COUNT-1)
// FP_F = 2^FP_COUNT

/**
 * FP_FROMINT(N)
 * convert fp_int N to fixed
 * return fixed
 **/
 
inline fixed fp_fromint(fp_int n) {
	return n * FP_F;
} 

#define FP_FROMINT(N) N * FP_F


/**
 * FP_FLOOR(X)
 * round fixed X to 0
 * return fp_int
 **/
 
 inline fp_int fp_floor(fixed x) {
	return x / FP_F;
 }

#define FP_FLOOR(X) X / FP_F

/**
 * FP_ROUND(X)
 * round fixed X to nearest
 * return fp_int
 **/

inline fp_int fp_round(fixed x) {
	return (x<0) ? (x - FP_F/2)/FP_F : (x + FP_F/2)/FP_F;
}
 
#define FP_ROUND(X) (X < 0) ? (X - FP_F/2)/FP_F : (X + FP_F/2)/FP_F

/**
 * FP_OP(X,Y)
 * fixed X OP fixed Y
 * return fixed
 **/
 inline fixed fp_add(fixed x, fixed y) {
	return x + y;
 }
#define FP_ADD(X,Y) X + Y

inline fixed fp_sub(fixed x, fixed y) {
	return x - y;
}
#define FP_SUB(X,Y) X - Y

inline fixed fp_mul(fixed x, fixed y) {
	return ((fixed_large)x) * y / FP_F;
}
#define FP_MUL(X,Y) ((fixed_large) X) * Y / FP_F

inline fixed fp_div(fixed x, fixed y) {
	return ((fixed_large)x) * FP_F / y;
}
#define FP_DIV(X,Y) ((fixed_large) X) * FP_F / Y

/**
 * FP_ADD(X,N)
 * Add fixed x to fp_int n
 * return fixed
**/
inline fixed fp_addi(fixed x, fp_int n) {
	return x + n * FP_F;
}
#define FP_ADDI(X,N) X + N * FP_F

inline fixed fp_subi(fixed x, fp_int n) {
	return x - n * FP_F;
}
#define FP_SUBI(X,N) X - N * FP_F

inline fixed fp_muli(fixed x, fp_int n) {
	return x * n;
}
#define FP_MULI(x,N) X * N

inline fixed fp_divi(fixed x, fp_int n) {
	return x / n;
}
#define FP_DIVI(X,N) X / N
