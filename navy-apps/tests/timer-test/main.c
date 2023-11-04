#include <stdio.h>
#include <time.h>
#include <sys/time.h>

int main(void) {
    struct timeval tv;
    int half_sec = 500000;
    printf("Entering program...\n");
    for(int i = 1; i < 1000; i++) {
        do {
            gettimeofday(&tv, NULL);
            //printf("now usec have val %ld\n", tv.tv_usec);
        } while(tv.tv_usec < half_sec * i);
        printf("hello from timer %d time\n", i);
    }
    return 0;
}