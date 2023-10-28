#include <cpu/cpu.h>

#ifdef CONFIG_ITRACE
typedef struct IRingBuffer_Node {
    char _logbuf[128];
    struct IRingBuffer_Node *next;
}IRingBuffer_Node;

static IRingBuffer_Node RBN_pool[CONFIG_RSIZE] = {};
/* Indicate IRINGBUFFER initialized */
static bool flag = false;
static IRingBuffer_Node *head = NULL, *tail = NULL;

static void Init_RingBuffer(void) {
    for (int i = 0; i < CONFIG_RSIZE; i++)
        RBN_pool[i].next = &RBN_pool[(i + 1) % CONFIG_RSIZE];
    flag = true;
    head = tail = &RBN_pool[0];
}

/* Always wrap next node */
void Insert_RingBuffer(const char *logbuf, const uint32_t _size) {
    if(!flag) Init_RingBuffer();
    if (tail->next == head) {
        tail = head;
        head = head->next;
        memcpy(tail->_logbuf, logbuf, _size);
    }
    else {
        memcpy(tail->_logbuf, logbuf, _size);
        tail = tail->next;
    }
}

void Display_RingBuffer(void) {
    if(head == tail) {
        printf("Run Program first\n");
        return;
    }
    IRingBuffer_Node *_tmp = head;
    while(_tmp->next != tail) {
        printf("     %s\n", _tmp->_logbuf);
        _tmp = _tmp->next;
    }
    printf(" --> %s\n", _tmp->_logbuf);
} 
#endif