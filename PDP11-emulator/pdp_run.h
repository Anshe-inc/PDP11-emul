#include "main.h"
#include "pdp11-mem.h"


#define NO_PARAMS 0			//for 'byte params'
#define HAS_DD 1
#define HAS_SS 2
#define HAS_NN 4
#define HAS_R 8
#define HAS_XX 16
#define HAS_NZVC 32




void do_mov();			// MOVB
void do_halt();			// halt
void do_add();			// ADD
void do_cmp();			// CMP
void do_inc();			// INCB
void do_clr();			// CLR
void do_sob();			// SOB
void do_jmp();			// JMP
void do_tst();			// TST *nzvc
void do_cco();			// reads condition code operators *nzvc
void do_br();			// all for branch
void do_beq();
void do_bne();
void do_bmi();
void do_bpl();
void do_blt();
void do_bge();
void do_ble();			// -------
void do_jsr();			//stack pointers
void do_rts();			// -------------
void do_nothing();

typedef struct{
	word mask;
	word opcode;
	char *opname;
	void (*do_func)(void);
	unsigned char params;
} Command;

typedef struct{
	word addr;				//arg addr
	word val;				//arg value
	byte is_byte;			//byte or word and mode0(2nd bit)
} Arg;

Arg get_mr(word w);
Arg get_nn(word w);
Arg get_xx(word w);
Arg get_r(word w);

//void psw(Arg dd, Arg ss);

void run();
