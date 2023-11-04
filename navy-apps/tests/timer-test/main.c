#include <stdio.h>
#include <time.h>
#include <sys/time.h>

int main(void) {
    struct timeval tv;
    int half_sec = 500000;
    printf("Entering program...\n");
    for(int i = 0; i < 1000; i++) {
        do {
            gettimeofday(&tv, NULL);
        }while(tv.tv_usec < half_sec * i);
        printf("hello from timer %d time\n", i);
    }
    return 0;
}