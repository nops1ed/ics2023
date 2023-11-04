#include <stdio.h>
#include <time.h>
#include <sys/time.h>

int main(void) {
    struct timeval *tv;
    int half_sec = 500000;
    printf("Entering program...\n");
    while(1) {
        gettimeofday(tv, NULL);
        while(tv->tv_usec < half_sec) {
            printf("Hello \n");
        }
    }
    return 0;
}