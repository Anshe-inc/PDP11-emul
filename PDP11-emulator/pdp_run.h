#include "main.h"


#define NO_PARAMS 0			//for 'byte params'
#define HAS_DD 1
#define HAS_SS 2



void do_mov();
void do_halt();
void do_add();
void do_inc();
void do_nothing();

typedef struct{
	word mask;
	word opcode;
	char *opname;
	void (*do_func)(void);
	byte params;
} Command;

typedef struct{
	word addr;				//arg addr
	word val;
	byte mode;//arg value
} Arg;

Arg get_mr(word w);

void run();
