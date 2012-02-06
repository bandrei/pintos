#include "fixedpoint.h"

inline fixed fp_fromint(fp_int n) {
	return n * FP_F;
} 

inline fp_int fp_floor(fixed x) {
	return x / FP_F;
}

inline fp_int fp_round(fixed x) {
	return (x<0) ? (x - FP_F/2)/FP_F : (x + FP_F/2)/FP_F;
}

inline fixed fp_inc(fixed x) {
    return x + FP_F;
}

inline fixed fp_add(fixed x, fixed y) {
      return x + y;
}
 
 
inline fixed fp_sub(fixed x, fixed y) {
      return x - y;
}

inline fixed fp_mul(fixed x, fixed y) {
	return ((fixed_large)x) * y / FP_F;
}

inline fixed fp_div(fixed x, fixed y) {
	return ((fixed_large)x) * FP_F / y;
}

inline fixed fp_addi(fixed x, fp_int n) {
	return x + n * FP_F;
}

inline fixed fp_subi(fixed x, fp_int n) {
	return x - n * FP_F;
}

inline fixed fp_muli(fixed x, fp_int n) {
	return x * n;
}

inline fixed fp_divi(fixed x, fp_int n) {
	return x / n;
}