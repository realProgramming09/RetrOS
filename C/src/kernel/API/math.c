#include "kernel/fixedMath.h"

#define PRECISION 8

fixed_t fixedMul(fixed_t a, fixed_t b){
    fixed_t c= a * b;
    return  (c >> PRECISION);
}
fixed_t fixedDiv(fixed_t a, fixed_t b){
    fixed_t c =  (a << PRECISION) * b;
    return c;
}
int32_t toInt(fixed_t n){
    return (int32_t)(n >> PRECISION);
}
fixed_t toFixed(float f){
    return (fixed_t)(f * ( 1 << PRECISION));
}