#include <stdio.h>
#include <time.h>
#include <sys/time.h>

int main(void) {
    struct timeval tv;
    printf("Entering program...\n");
    for(int i = 1; i < 10; i++) {
        do {
            gettimeofday(&tv, NULL);
        } while(tv.tv_usec / 1000000 < i);
        printf("hello from timer %d time\n", i);
    }
    printf("PASS !!!\n");
    return 0;
}