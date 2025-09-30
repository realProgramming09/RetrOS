#include "sys.h"

typedef  int32_t fixed_t;

fixed_t fixedMul(fixed_t a, fixed_t b); //Moltiplicazione in virgola fissa
fixed_t fixedDiv(fixed_t a, fixed_t b); //Divisione in virgola fissa   
int32_t toInt(fixed_t n); //Trasforma un decimale in un intero
fixed_t toFixed(float f); //Trasforma un intero o un decimale in virgola mobile a un decimale in virgola fissa