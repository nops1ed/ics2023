/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include "sdb.h"

#define NR_WP 32
#define WP_BUF_MAX 64

typedef struct watchpoint {
	int NO;
	struct watchpoint *next;
	char stored_expr[WP_BUF_MAX];	
	bool flag;
	word_t old_val , new_val;
} WP;

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;
/* Indicate whether wp_pool is initialized */
static bool flag = false;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }

  head = NULL;
  free_ = wp_pool;
}

/* Display all watchpoints */
void display_wp (void) {
	if (!head) {
		printf("watchpoint: No watchpoint\n");
		return;
	}
	WP* _tmp = head;
	printf("%5s %20s %10s\n" , "Num" , "Type" , "What");
	while(_tmp) {
		printf("%5d %20s %10s\n" , _tmp -> NO , "hw watchpoint" , _tmp -> stored_expr);
		_tmp = _tmp -> next;
	}
}

WP* new_wp() {
	if (!free_)	return NULL;
	if (!head) {
		head = free_;
		free_ = free_ -> next;
		head -> next = NULL;
		return head;	
	}
	WP *_tmp;
	for (_tmp = head; _tmp -> next != NULL ; _tmp = _tmp -> next);	
	_tmp -> next = free_;
	free_ = free_ -> next;
	_tmp -> next -> next = NULL;
	_tmp -> next -> flag = false;
	return _tmp -> next;
}

/* We'd better not to expose any detail to external caller 
*  So it is better to locate the node we need to delete in our delete function 
*/
void free_wp(int NO) {
	if (!head) {
		printf("watchpoint: wp free failed\n");
		return;
	}
	
	WP *_tmp = head, *_pre = NULL;
	while(_tmp && _tmp -> NO != NO) {
		_tmp = _tmp -> next;
		_pre = _pre ? _pre -> next : head;
	}
	if (!_tmp) {
		printf("watchpoint: No node with NO%2d could be deleted\n" , NO);
		return ;
	}
	if (_pre) _pre -> next = _tmp -> next;
	else head = _tmp -> next;
	WP* tail = free_;
	while (tail && tail -> next) tail = tail -> next;
	if (!tail) {
		_tmp -> next = tail;
		free_ = _tmp;
	}
	else {
		_tmp -> next = tail -> next;
		tail -> next = _tmp;
	}
}

void free_wp_all(void) {
	WP* _tmp = head;
	while (_tmp) {
		WP* r = _tmp -> next;
		_tmp -> next = free_;
		free_ = _tmp;
		_tmp = r;
	}
	head = NULL;
}

int sdb_watchpoint_create(char *s) {
  if (!flag) {
	init_wp_pool();
	flag = true;
  } 
  /* Below code detected whether expression is valid instead of creating wp */
  if (sizeof(s) > WP_BUF_MAX) {
    printf("watchpoint: too long expression\n");
    return -1;
  }
  bool success = true;
  word_t val = expr(s , &success);
  if (!success) {
    printf("watchpoint: Bad expression\n");
    return -1;
  }

  WP* _tmp = new_wp();
  /* Allocation exception */
  if (!_tmp) {
    printf("watchpoint: allocation failed\n");
    return -1;
  }

  /* Initialize the node */
  memcpy(_tmp -> stored_expr , s , strlen(s));
  _tmp -> old_val = _tmp -> new_val = val;
  return _tmp -> NO;
}

void sdb_watchpoint_delete (int NO) {
  if (NO > NR_WP || NO < 0) {
    printf("Valid watchpoint number should between 0 and %d\n" , NR_WP - 1);
    return;
  }
  free_wp(NO);
}

void sdb_watchpoint_delete_all(void) {
	free_wp_all();
	/* More debug information required */
}

void sdb_watchpoint_display(void) {
  display_wp();
  /* More debug information required */
}

bool trace_watchpoint_diff_test(void) {
  /* trace all watchpoints and difftest */
  bool trace_flag = false;
  WP* _tmp = head;
  while (_tmp) {
	bool success = true;
	word_t val = expr(_tmp -> stored_expr , &success);
	if (!success) {
		printf("\033[31mtrace: eval error\033[0m\n");
		assert(0);
	}
	if (_tmp -> new_val != val) {
		_tmp -> old_val = _tmp -> new_val;
		_tmp -> new_val = val;
		_tmp -> flag = true;
		trace_flag = true;
	}	
	else _tmp -> flag = false;
	_tmp = _tmp -> next;
  }
  return trace_flag;
}

void trace_watchpoint_diff_display(void) {
  WP* _tmp = head;
  while(_tmp) {
	if (_tmp -> flag)
		printf("Hardware watchpoint %d: %s\nOld value = 0x%lx\nNew value = 0x%lx\n"
				, _tmp -> NO , _tmp -> stored_expr , _tmp -> old_val , _tmp -> new_val);
	_tmp = _tmp -> next;
  }
}