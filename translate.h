/* translate.h */
#ifndef __TRANSLATE_H_
#define __TRANSLATE_H_

#include "mipsframe.c"
#include "tree.h"


/* 虚的 Translate 模块*/
//typedef void *Tr_exp;
typedef struct Tr_level_ *Tr_level;
typedef struct Tr_access_ *Tr_access;
typedef struct Tr_accessList_ *Tr_accessList;
struct Tr_accessList_ {Tr_access head; Tr_accessList tail;};

Tr_accessList Tr_AccessList(Tr_access head, Tr_accessList tail);
Tr_level Tr_outermost(void);
Tr_level Tr_newLevel(Tr_level parent, Temp_label name, U_boolList formals);
Tr_accessList Tr_formals(Tr_level level);
Tr_access Tr_allocLocal(Tr_level level, bool escape);

typedef struct patchList_ *patchList;
struct patchList_ { Temp_label *head; patchList tail };

typedef struct Tr_exp_ *Tr_exp;
struct Cx { patchList trues; patchList falses; T_stm stm};
struct Tr_exp_ {
	enum {Tr_ex, Tr_nx, Tr_cx} kind;
	union {
		T_exp ex;
		T_stm nx;
		struct Cx cx;
	} u;
};

//构建各类表达式的Tiger中间表达式
Tr_exp Tr_intExp(int);  
Tr_exp Tr_stringExp(string val);
Tr_exp Tr_simpleVar(Tr_access ta, Tr_level tl);
Tr_exp Tr_fieldVar(Tr_access ta, Tr_level t1);
Tr_exp Tr_subscriptVar(Tr_exp base, Tr_exp index);
Tr_exp Tr_opExp(int ope, Tr_exp left, Tr_exp right);//对整型的二元操作符
Tr_exp Tr_op2Exp(int ope, Tr_exp left, Tr_exp right); //条件表达式
#endif