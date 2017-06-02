/* function prototype from printtree.c */
#ifndef _PRINTTREE_H_
#define _PRINTTREE_H_

void printStmList(FILE *out, T_stmList stmList);
void printExp(T_exp e, FILE * out);
void printStm(T_stm s, FILE * out);

#endif // !PRINTTREE_H_


