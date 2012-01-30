typedef fixed int64_t
typedef fp_int int64_t 
// 63 bit signed integer + 1 sign bit
#define FP_COUNT 28
// sign bit, 35bits . 28 bits
#define FP_F 0x1<<(FP_COUNT-1)
// FP_F = 2^FP_COUNT

/**
 * FP_FROMINT(N)
 * convert fp_int N to fixed
 **/

#define FP_FROMINT(N) N * FP_F


/**
 * FP_FLOOR(X)
 * round X to 0
 **/

#define FP_FLOOR (X) X / FP_F



