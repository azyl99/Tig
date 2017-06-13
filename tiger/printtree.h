/* function prototype from printtree.c */
#ifndef _PRINTTREE_H_
#define _PRINTTREE_H_

#include "frame.h"
void printStmList(FILE *out, T_stmList stmList);
void printExp(T_exp e, FILE * out);
void printStm(T_stm s, FILE * out);
void print_stringFrag(F_fragList fl, FILE * out);
void print_procFrag(F_fragList fl, FILE * out);
void print_procFrag2(F_fragList fl, char * f);

#endif // !PRINTTREE_H_


