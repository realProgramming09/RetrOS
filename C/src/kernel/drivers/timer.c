#include "kernel/timer.h"

static int timer = 0;

void stepTimer(){
    timer++;
}
int now(){
    return timer;
}
void resetTimer(){
    timer = 0;
}