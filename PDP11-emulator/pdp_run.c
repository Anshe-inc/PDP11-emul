#include "pdp_run.h"
#define pc reg[7]
#define sp reg[6]


extern byte mem;
extern byte is_traced;
word reg[REGSIZE];
byte psw = 0;				//NZVC, V = 0
byte snzvc; 				// for cco

word istat = 0177560;
word idata = 0177562;
word ostat = 0177564;
word odata = 0177566;
 
Arg ss, dd, nn, r, xx;

Command com[] = {									//contains all info about operators
	{ 0170000, 0010000, "mov", do_mov, HAS_SS | HAS_DD },
	{ 0170000, 0110000, "movb", do_mov, HAS_SS | HAS_DD },
	{ 0070000, 0060000, "add", do_add, HAS_SS | HAS_DD },
	{ 0170000, 0060000, "cmp", do_cmp, HAS_SS | HAS_DD },
	{ 0077000, 0077000, "sob", do_sob, HAS_R | HAS_NN },
	{ 0107700, 0005200, "inc", do_inc, HAS_DD },
	{ 0107700, 0105200, "incb", do_inc, HAS_DD },
	{ 0007700, 0005000, "clr", do_clr, HAS_DD },
	{ 0000700, 0000100, "jmp", do_jmp, HAS_DD },
	{ 0107700, 0005700, "tst", do_tst, HAS_DD },
	{ 0107700, 0105700, "tstb", do_tst, HAS_DD },
	{ 0007400, 0000400, "br", do_br, HAS_XX },
	{ 0007400, 0001400, "beq", do_beq, HAS_XX },
	{ 0001000, 0001000, "bne", do_bne, HAS_XX },
	{ 0100400, 0100400, "bmi", do_bmi, HAS_XX },
	{ 0100000, 0100000, "bpl", do_bpl, HAS_XX },
	{ 0007400, 0002400, "blt", do_blt, HAS_XX },
	{ 0007000, 0002000, "bge", do_bge, HAS_XX },
	{ 0007000, 0004000, "jsr", do_jsr, HAS_R | HAS_DD },
	{ 0000770, 0000200, "rts", do_rts, HAS_R },
	{ 0000760, 0000240, "", do_cco, HAS_NZVC },
	{ 0xffff, 0000000, "halt", do_halt, 0 }
};

void run(){										//test run
	dump(0200,0xc);
	info("\n------------------ run ----------------\n");
	pc = 01000;
	word w;
	word i;
	word len_com = sizeof(com)/ sizeof(com[0]);
	b_write(ostat, -1);
	while(1){
		w = w_read(pc);
		trace("%06o %06o:", pc, w);
		pc += 2;
		if(w == 0){
			do_halt();
		}
		
		for(i = 0; i < len_com; i++){
			if((w & com[i].mask) == com[i].opcode){
				trace(com[i].opname);
				if(com[i].params & HAS_R){
					if(com[i].params > HAS_R)
						r = get_r(w >> 6);
					else if(com[i].params == HAS_R)
						r = get_r(w);
				}
				if(com[i].params & HAS_SS){
					ss = get_mr(w >> 6 | ((w>>15)<<15));
				}
				if(com[i].params & HAS_DD){
					dd = get_mr(w);
				}
				if(com[i].params & HAS_NN){
					nn = get_nn(w);
				}
				if(com[i].params & HAS_NZVC){
					snzvc = w & 037;
				}
				if(w & HAS_XX){
					xx = get_xx(w);
				}
				com[i].do_func();
				break;
				
			}
			if(i == len_com - 1){
				do_nothing();
				break;
			}
		}
		trace("\n");
		if(is_traced)
			dump_reg();
		
	}
}

void do_halt(){
	trace("halt");
	info("\n---------------- halted ---------------\n");
	dump_reg();
	//dump(0x0200, 0x0010);
	dump(0177564, 0x0008);
	exit(0);
}


void do_mov(){									// MOVb
	if(dd.is_byte & 2){
		reg[dd.addr] = ss.val;
	}
	else if(dd.is_byte & 1){	
		b_write(dd.addr, ss.val & 0xff);
		psw = (ss.val >> 7) << 3;		// N000
	}
	else{
		w_write(dd.addr, ss.val);
		psw = (ss.val >> 15) << 3 ;		// N000
	}
	psw |= (ss.val == 0) << 2;			// xZ00
	
	if(dd.addr == odata){
		fprintf(stderr, "%c", b_read(odata));
	
	}
}


void do_add(){									// ADD
	word a, b;
	if(dd.is_byte & 2){
		a = reg[dd.addr];
		b = ss.val;
		reg[dd.addr] += ss.val;
	}
	else{
		a = dd.val;
		b = ss.val;
		w_write(dd.addr, a + b);				
		
	}
	psw = ((a+b) >> 15) << 3;			// Nxxx
	psw |= ((a+b) == 0) << 2;			// xZxx
	psw |= ((a != 0) && (b != 0)) && ((a >> 15) || (b >> 15));	// xxxC
}


void do_inc(){									// INC
	word a;
	if(dd.is_byte & 2){
		reg[dd.addr]++;
		a = reg[dd.addr];
	}
			
	if(dd.is_byte & 1){
		b_write(dd.addr, ++dd.val);
		psw = ((a+1) >> 7) << 3;		// Nxxx
	}
	else{
		a = dd.val;
		w_write(dd.addr, ++dd.val);
		psw = ((a+1) >> 15) << 3;		// Nxxx
	}
	psw |= ((a+1) == 0) << 2;			// xZxx
	psw |= (a != 0) && ((a >> 15));		// xxxC
}


void do_sob(){									// SOB
	int regist = r.addr;
	reg[regist]--;
	if(reg[regist] != 0){
		pc = pc - nn.val;
	}
	else
		return;
}


void do_clr(){									// CLR
	reg[dd.addr] = 0;
	w_write(dd.addr, 0);
	psw = 4;									// xZxx
}

void do_tst(){									// TST
	psw &= 12;							// NZxx
	if(dd.is_byte){
		psw = (dd.val >> 7) << 3;		// Nxxx
	}
	else{
		psw = (dd.val >> 15) << 3;		// Nxxx
	}
	psw |= (dd.val == 0) << 2;			// xZxx
}

void do_cmp(){									// CMPb
	word a, b, res;
	a = dd.val;
	b = ss.val;
	if(dd.is_byte == 2){
		a = reg[dd.addr];
	}
	
	res = a - b;
	
	psw = ((res) >> 15) << 3;			// Nxxx
	psw |= ((res) == 0) << 2;			// xZxx
	psw |= (a < b && a > 0);	// xxxC
}

void do_cco() {									// condition code op
	byte is_set = snzvc >> 4;
	byte mask = snzvc & 0xf;
	char cond[5] = "nzvc";
	
	if(mask == 0xf && is_set){
		trace("scc");
		psw = mask;			// NZVC = 1111
	}
	else if(mask == 0x0){
		trace("nop");
	}
	else if(mask == 0xf){
		trace("ccc");
		psw = ~mask;
	}
	else if(is_set){
		trace("se");
		psw |= mask;
	}else{
		trace("cl");
		psw &= ~mask;
	}
	
	trace("%c",cond[(int)(3.0/mask + 0.5)]);
	
}

void do_jmp() {									// JMP
	if(dd.is_byte & 2){
		pc = 2*dd.addr;
	}else{
		pc = dd.addr;
	}
}
void do_br(){									// BR
	pc += xx.addr * 2;
}
void do_beq(){				// z==1
	if(psw & 4){
		do_br();
	}
}
void do_bne(){				// z==0
	if((psw & 4) == 0){
		do_br();
	}
}
void do_bmi(){				// n==1
	if(psw & 8){
		do_br();
	}
}
void do_bpl(){				// n==0
	if((psw & 8) == 0){
		do_br();
	}
}
void do_blt(){				// n^v==1
	if((psw & 8) ^ (psw & 2)){
		do_br();
	}
}
void do_bge(){				// n^v==0
	if(((psw & 8) ^ (psw & 2)) == 0){
		do_br();
	}
}
void do_ble(){
	if((psw & 4) | ((psw & 8) ^ (psw & 2))){	// z|(n^v)==1
		do_br();
	}
}

void do_jsr(){									// JSR
	word temp = dd.addr;
	w_write(sp, reg[r.addr]);
	sp -= 2;
	reg[r.addr] = pc;
	pc = temp;
	
}

void do_rts(){										// RTS
	pc = reg[r.addr];
	sp += 2;
	reg[r.addr] = w_read(sp);
	trace("\n\n%06o\n\n", pc);
}

void do_nothing(){								// it does nothing
	trace("unknown");
}


// get numbers
Arg get_mr(word w){
	Arg res;
	word regist = w & 0007;			// reg number
	word mode = (w >> 3) & 7;		// mode number
	word is_byte = (w >> 15) & 1;
	res.is_byte = is_byte;
	word x;
	
	switch(mode){
		case 0:					// Rx
			res.is_byte |= 2;	// for mode 0 only
			res.addr = regist;
			res.val = reg[regist];
			trace(" R%o ", regist);
			break;
		case 1:					//(Rx)
			res.addr = reg[regist];
			if(is_byte)
				res.val = b_read(res.addr) && 0x00FF;
			else
				res.val = w_read(res.addr);
			trace(" (R%o) ", regist);
			break;
		case 2:					//(Rx)+
			res.addr = reg[regist];
			
			if(res.is_byte){		//byte or word + sp & pc
				res.val = b_read(res.addr);
				reg[regist] += (1 + (regist > 5)); //sum 2 if sp & pc
			}
			else{
				res.val = w_read(res.addr);
				reg[regist] += 2;
			}
			
			if(regist == 7){
				trace(" #%o ", res.val);
			}
			else{
				trace(" (R%o)+ ", regist);
			}
			break;
		case 3:				// @(Rx)+
			res.addr = reg[regist];
			res.addr = w_read(res.addr);
			res.val = w_read(res.addr);
			if(regist == 7){
				trace(" @#%o ", res.addr);
			}
			else{
				trace(" @(R%o)+ ", regist);
			}
			reg[regist] += 2;
			
			break;			
		case 4:					//-(Rx)
			if(res.is_byte){
				reg[regist] -= 1 + (regist > 5); // subst 2 if sp & pc
				res.addr = reg[regist];
				res.val = b_read(reg[regist]);
			}
			else{
				reg[regist] -= 2;
				res.addr = reg[regist];
				res.val = w_read(reg[regist]);
			}
			trace(" -(R%o) ", regist);
			break;
		case 5:
			reg[regist] -= 2;
			res.addr = reg[regist];
			res.addr = w_read(res.addr);
			res.val = w_read(res.addr);
			
			trace(" @-(R%o) ", regist);
			break;
		case 6:
			x = w_read(pc);
			pc += 2;
			res.addr = x + reg[regist];
			res.val = w_read(res.addr);
			if(regist == 7){
				trace(" %o ", res.addr);
			}
			else{
				trace(" %o(R%o) ", x, regist);
			}
			break;
		default:
			fprintf(stderr, "MODE %o NOT IMPLEMENTED YET", mode);
			exit(1);
	}
	return res;
}

Arg get_nn(word w){
	Arg res;
	word num = w & 077;
	
	res.val = 2 * num;
	
	trace(" #%o ", pc - res.val);
	
	return  res;
	
}

Arg get_r(word w){
	Arg res;
	word regist = w & 07;
	
	res.addr = regist;
	
	if(regist <= 6){
		trace(" R%o ", res.addr);
	}else{
		trace(" PC ");
	}
	
	return res;
	
}

Arg get_xx(word w){
	byte res = w;
	xx.addr = res;
	return xx;
}