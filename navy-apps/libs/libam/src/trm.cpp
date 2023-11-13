#include <am.h>


Area heap;

void putch(char ch) {
    //write(1, (const char *)&ch, 1);
}

void halt(int code) {
    //exit(code);

    while(1) ;
}
/*
#include <am.h>
#include <stdlib.h>
Area heap = {};
// allocated nil heap
// cannot run oslab0-16xxxxxxx, it used unallocated heap
// microbench would ignore much of its benches due to heap
void putch(char ch) {
    putchar(ch);
}

void halt(int code) {
    exit(code);
}
*/