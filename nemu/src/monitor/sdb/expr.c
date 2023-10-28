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

#include <isa.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>

enum {
  	TK_NOTYPE = 256, 
	TK_LBT = 1 , TK_RBT , TK_NEG , TK_POS , TK_DEREF , TK_GADDR , 	/* Tier 0 - 1*/
	TK_MULTI , TK_DIV , TK_MOD , TK_PLUS , TK_MINUS ,				/* Tier 2 - 3*/
	TK_EQ , TK_NEQ , TK_LAND , 
	TK_REG , 
	TK_DEC, TK_HEX ,  
	// And so on
	/* TODO: Add more token types */

};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */
  	{" +", TK_NOTYPE},					// spaces
  	{"\\+", TK_PLUS},					// plus
	{"\\-", TK_MINUS},					// minus
	{"\\*", TK_MULTI},					// multi
	{"\\/", TK_DIV},					// divide
	{"\\(", TK_LBT},					// left bracket
	{"\\)", TK_RBT},					// right bracket
	{"\\$[a-zA-Z]*[0-9]*" , TK_REG},	// register
	{"0[Xx][0-9a-fA-F]*",TK_HEX}, 		// hex number
	{"[0-9]*" , TK_DEC},			 	// decimal number								
	{"==", TK_EQ},						// bool equal
	{"!=" , TK_NEQ},					// bool not equal
	{"&&" , TK_LAND} ,					// logical and
															// TODO
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

static Token tokens[32] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

static bool make_token(char *e) {
  int position = 0;
  int i , len;
  regmatch_t pmatch;
  nr_token = 0;

  while (e[position] != '\0') {
  	bool flag = false;
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i++) {
    	if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        	char *substr_start = e + position;
        	int substr_len = pmatch.rm_eo;

        	//Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            //	i, rules[i].regex, position, substr_len, substr_len, substr_start);

			position += substr_len;

        	/* Now a new token is recognized with rules[i]. Add codes
         	* to record the token in the array `tokens'. For certain types
         	* of tokens, some extra actions should be performed.
         	*/

			switch (rules[i].token_type) {
				case TK_DEC :
					len = substr_len < 31 ? substr_len : 31;
					/* DO NOT USE 'strcpy' !
					* It may cause buffer overflow
					*/
					strncpy(tokens[nr_token].str , substr_start , len);
					tokens[nr_token].str[len] = '\0';
					//printf("\nNow we got %s\n" , tokens[nr_token].str);
					tokens[nr_token++].type = rules[i].token_type;
					break;
				case TK_REG:
					// Same as TK_DEC case , which just need to change start location
					len = substr_len < 32 ? substr_len : 31;
					/* DO NOT USE 'strcpy' !
					* It may cause buffer overflow
					*/
					strncpy(tokens[nr_token].str , substr_start , len);
					tokens[nr_token].str[len] = '\0';
					tokens[nr_token++].type = rules[i].token_type;
					break;
				case TK_HEX:
					len = substr_len < 33 ? substr_len : 31;
					/* DO NOT USE 'strcpy' !
					* It may cause buffer overflow
					*/
					strncpy(tokens[nr_token].str , substr_start + 2 , len - 2);
					tokens[nr_token].str[len] = '\0';
					tokens[nr_token++].type = rules[i].token_type;
					break;
				case TK_NOTYPE:
					break;
          		default: 
					//printf("\nIt seems like u got default branch\n");
					//printf("\nAnd the type could be %d\n", rules[i].token_type);
					tokens[nr_token++].type = rules[i].token_type;
					// Do nothing
					;
			}
			flag = true;
    	}
		//else printf("\nOOPs! Seems like no type got\n");
		if (flag) break;
	}
	//printf("\nNow i = %d , NR_REGEX = %d\n" , i , NR_REGEX);
    if (i == NR_REGEX) {
    	printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
    	return false;
    }
}
	nr_token--;
  	return true;
}

static bool check_parentheses(int p , int q) {
	//printf("\noops , Seems like u trap into check_parentheses function\n");
	//printf("\np is %d , q is %d\n" , p , q);
	// Check tokens list		
	if (tokens[p++].type != TK_LBT || tokens[q].type != TK_RBT)
		return false;
	// Simulate stack
	uint32_t left_count = 1;
	for (int i = p; i <= q ; i++) {
		switch(tokens[i].type) {
			case TK_LBT:
				left_count += 1;
				break;
			case TK_RBT:
				left_count -= 1;
				break;
			default:
				// Do nothing 
				;
		}
		if (left_count < 0) {
			printf("\ncheck_parentheses: Bad expression\n");
			assert(0);
		}
		if (left_count == 0 && i < q)  return false;
	}
	return true;
}

static uint32_t domain_find(uint32_t p , uint32_t q) {
	uint32_t domain = -1;
	for (int i = p ; i < q ; i++) {
		/* All of right brackets should be checked in below case  
		 * Otherwise it could be bad expression 
		 */
		if (tokens[i].type == TK_LBT) {
			uint32_t left_count = 1;	
			while(++i < q) {
				switch(tokens[i].type) {
					case TK_LBT:
						left_count += 1;
						break;
					case TK_RBT:
						left_count -= 1;
						break;
					default:
						// No domain token should be here
						// So just Do nothing
						;
				}
				if (left_count == 0) break;
			}
			// This should be invalid 
			if (i > q) assert(0);
		}	
		else if (tokens[i].type == TK_RBT)
			// Bad expression
			assert(0);
		else if (tokens[i].type == TK_DEC || tokens[i].type == TK_REG
					|| tokens[i].type == TK_HEX)
			//TODO: Add more number type
			continue;
		else {
			if (domain == -1 || tokens[domain].type <= tokens[i].type)
				domain = i;
			//else fo nothing
		}
	}
	return domain;
}


static uint32_t eval(int p , int q) {
	// Bad expression
  	if (p > q) {
		printf("\n eval: Bad erxpersion\n");
		assert(0);
  	}
	/* This could be decimal , hex , addr , register or dereference */
  	else if (p == q) {
		switch(tokens[p].type)
		{
			case TK_DEC:
				int dec_val;
				sscanf(tokens[p].str , "%d" , &dec_val);
				return dec_val;
			case TK_HEX:
				int hex_val;
				sscanf(tokens[p].str , "%x" , &hex_val);
				return hex_val;
			case TK_REG:
				bool success = true;
				word_t reg_val = isa_reg_str2val(tokens[p].str , &success);
				if (!success) {
					printf("\nNo reg:%s found\n" , tokens[p].str);
					return 0;
				}
				return reg_val;
			default:
				return 0;
		}

	}
  	else if (check_parentheses(p, q)) {
		//printf("\nHere we detected parentheses\n");
    	return eval(p + 1, q - 1);
  	}
  	else {

		/*
    	 * op = the position of "Domain OPERATION" in the token expression;
    	 * val1 = eval(p, op - 1);
    	 * val2 = eval(op + 1, q);
		 */

		//printf("\nI am finding domain from %d to %d\n" , p , q);
		uint32_t op = domain_find(p , q);
		//printf("\nThe domain OPERATION could be %d \n" , op);
		//printf("\nThe domain OPERATION could be %d \n" , tokens[op].type);
		if (op == -1) assert(0);
		//printf("\nI am eval %d to %d\n" , p , op - 1);
		uint32_t val1 = eval(p , op - 1);
		//printf("\nI am eval %d to %d\n" , op + 1 , q);
		uint32_t val2 = eval(op + 1 , q);
    	switch (tokens[op].type) {
      		case TK_PLUS:  return val1 + val2;
			case TK_MINUS: return val1 - val2; 
			case TK_MULTI: return val1 * val2;
			case TK_DIV:
				if (val2 == 0) {
					printf("\neval: SIGFPE \n");
					assert(0);
				}
				else return val1 / val2;
			case TK_MOD: return val1 % val2;
			case TK_NEQ: return (val1 != val2);
			case TK_EQ:  return (val1 == val2);
			case TK_LAND:return (val1 && val2);
      		default: 
				printf("\neval: Seems like u got default branch\nHuh , That is bad\n");
				assert(0);
    	}
	}
}

static bool certain_type (uint32_t type)
{
	switch(type)
	{
		case TK_LBT:
		case TK_PLUS:
		case TK_MINUS:
		case TK_MULTI:
		case TK_DIV:
		case TK_MOD:
			return true;
		default:
	}
	return false;
}

word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
	for (int i = 0; i < nr_token; i ++) {
		if (tokens[i].type == '*' && (i == 0 || certain_type(tokens[i - 1].type))) {
			tokens[i].type = TK_DEREF;
		}
	}
  return eval(0 , nr_token);
}
