#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <NDL.h>

int main(void) {
    struct timeval tv;
    NDL_Init(0);
    printf("Entering program...\n");
    for(int i = 1; i < 10; i++) {
        do {
            tv.tv_usec = NDL_GetTicks();
        } while(tv.tv_usec / 1000000 < i);
        printf("hello from timer %d time\n", i);
    }
    NDL_Quit();
    printf("PASS !!!\n");
    return 0;
}