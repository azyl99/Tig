/*
 * printtree.c - functions to print out intermediate representation (IR) trees.
 *
 */
#include <stdio.h>
#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "tree.h"
#include "printtree.h"

/* local function prototype */
static void pr_tree_exp(FILE *out, T_exp exp, int d);

static void indent(FILE *out, int d) {
 int i;
 for (i = 0; i <= d; i++) fprintf(out, " ");
}

static char bin_oper[][12] = {
   "PLUS", "MINUS", "TIMES", "DIVIDE", 
   "AND", "OR", "LSHIFT", "RSHIFT", "ARSHIFT", "XOR"};

static char rel_oper[][12] = {
  "EQ", "NE", "LT", "LE", "GT", "GE", "ULT", "ULE", "UGT", "UGE"};
 
static void pr_stm(FILE *out, T_stm stm, int d)
{
  switch (stm->kind) {
  case T_SEQ:
    indent(out,d);
    fprintf(out, "[SEQ"); pr_stm(out, stm->u.SEQ.left,0);
    pr_stm(out, stm->u.SEQ.right,0); fprintf(out, "]");
    break;
  case T_LABEL:
    indent(out,d); fprintf(out, "[LABEL [%s]]", S_name(stm->u.LABEL));
    break;
  case T_JUMP:
    indent(out,d); fprintf(out, "[JUMP"); pr_tree_exp(out, stm->u.JUMP.exp,0); 
    fprintf(out, "]");
    break;
  case T_CJUMP:
    indent(out,d); fprintf(out, "[CJUMP[%s]", rel_oper[stm->u.CJUMP.op]);
    pr_tree_exp(out, stm->u.CJUMP.left,0);
    pr_tree_exp(out, stm->u.CJUMP.right,0);
    indent(out,0); fprintf(out, "[%s]", S_name(stm->u.CJUMP.true));
    fprintf(out, "[%s]", S_name(stm->u.CJUMP.false)); fprintf(out, "]");
    break;
  case T_MOVE:
    indent(out,d); fprintf(out, "[MOVE"); pr_tree_exp(out, stm->u.MOVE.dst,0); 
    pr_tree_exp(out, stm->u.MOVE.src,0); fprintf(out, "]");
    break;
  case T_EXP:
    indent(out,d); fprintf(out, "[EXP"); pr_tree_exp(out, stm->u.EXP,0); 
    fprintf(out, "]");
    break;
  }
}

static void pr_tree_exp(FILE *out, T_exp exp, int d)
{
  switch (exp->kind) {
  case T_BINOP:
    indent(out,d); fprintf(out, "[BINOP[%s]", bin_oper[exp->u.BINOP.op]); 
    pr_tree_exp(out, exp->u.BINOP.left,0);
    pr_tree_exp(out, exp->u.BINOP.right,0); fprintf(out, "]");
    break;
  case T_MEM:
    indent(out,d); fprintf(out, "[MEM ");
    pr_tree_exp(out, exp->u.MEM,0); fprintf(out, "]");
    break;
  case T_TEMP:
    indent(out,d); fprintf(out, "[TEMP_t%s]", 
			   Temp_look(Temp_name(), exp->u.TEMP));
    break;
  case T_ESEQ:
    indent(out,d); fprintf(out, "[ESEQ"); pr_stm(out, exp->u.ESEQ.stm,0); 
    pr_tree_exp(out, exp->u.ESEQ.exp,0); fprintf(out, "]");
    break;
  case T_NAME:
    indent(out,d); fprintf(out, "[NAME[%s]]", S_name(exp->u.NAME));
    break;
  case T_CONST:
    indent(out,d); fprintf(out, "[CONST_%d]", exp->u.CONST);
    break;
  case T_CALL:
    {T_expList args = exp->u.CALL.args;
     indent(out,d); fprintf(out, "[CALL"); pr_tree_exp(out, exp->u.CALL.fun,0);
     for (;args; args=args->tail) {
       pr_tree_exp(out, args->head,1);
     }
     fprintf(out, "]");
     break;
   }
  } /* end of switch */
}

void printStmList (FILE *out, T_stmList stmList) 
{
  for (; stmList; stmList=stmList->tail) {
    pr_stm(out, stmList->head,0);
  }
}

void printExp(T_exp e, FILE * out) {
	pr_tree_exp(out, e, 0);
}

void printStm(T_stm s, FILE * out) {
	pr_stm(out, s, 0);
}

void print_stringFrag(F_fragList fl, FILE * out) {
	while (fl) {
		F_frag f = fl->head;
		fprintf(out, "%s : %s\n", S_name(f->u.stringg.label), f->u.stringg.str);
		fl = fl->tail;
	}
}

void print_procFrag(F_fragList fl, FILE * out) {
	while (fl) {
		F_frag f = fl->head;
		// fprintf(out, "/****procFrag body*********/\n");
		fprintf(out, "\n");
		printStm(f->u.proc.body, out);
		// fprintf(out, "\n/****procFrag frame*********/\n");
		// F_print_frame(f->u.proc.frame,out);
		fl = fl->tail;
	}
}
#include <string.h>
void print_procFrag2(F_fragList fl, char* basename) {
	char filename[100];
	int i = 0;
	while (fl) {
		F_frag f = fl->head;
		sprintf(filename, "%s_func%d.txt\0", basename, ++i);
		FILE* out = fopen(filename, "w");
		printStm(f->u.proc.body, out);
		fl = fl->tail;
	}
}