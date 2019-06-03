#ifndef __ANALYSE_H__
#define __ANALYSE_H__

#include <time.h>
#include <stdlib.h>

time_t getTime();
time_t timeSpend2(time_t, time_t);
double timeSpend(clock_t, clock_t);
int *getRandom(int);

#endif