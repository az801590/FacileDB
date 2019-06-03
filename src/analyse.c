#include "analyse.h"

time_t getTime()
{
    return time(NULL);
}

time_t timeSpend2(time_t t1, time_t t2)
{
    return t2-t1;
}

double timeSpend(clock_t t1, clock_t t2)
{
    return (double) (t2-t1)/(CLOCKS_PER_SEC);
}

int *getRandom(int size)
{
    int temp, pos;
    int *poker = (int*) malloc(size * sizeof(int));
    srand(time(NULL));

    poker[0] = size;
    for(int i = 1; i < size; i++)
    {
        poker[i] = i;
    }
    
    for(int i = 0; i < size; i++)
    {
        pos = (size - 1) * rand() / RAND_MAX;
        temp = poker[i];
        poker[i] = poker[pos];
        poker[pos] = temp;
    } 
     
    return poker;
}