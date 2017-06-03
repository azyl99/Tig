/* translate.h */
#ifndef __TRANSLATE_H_
#define __TRANSLATE_H_

#include "frame.h"
#include "absyn.h"


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
struct patchList_ { Temp_label *head; patchList tail; };

typedef struct Tr_exp_ *Tr_exp;
typedef struct Tr_expList_ *Tr_expList;
struct Tr_expList_ {
	Tr_exp head;
	Tr_expList tail;
};
Tr_expList Tr_ExpList(Tr_exp h, Tr_expList t);


struct Cx { patchList trues; patchList falses; T_stm stm; };
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
Tr_exp Tr_fieldVar(Tr_exp var, int offset);
Tr_exp Tr_subscriptVar(Tr_exp base, Tr_exp index);
Tr_exp Tr_arithopExp(A_oper oper, Tr_exp left, Tr_exp right);//对整型的二元操作符
Tr_exp Tr_logicExp(A_oper oper, Tr_exp left, Tr_exp right);
Tr_exp Tr_eqopExp(A_oper oper, Tr_exp left, Tr_exp right);
Tr_exp Tr_eqstringExp(A_oper oper, Tr_exp left, Tr_exp right);
Tr_exp Tr_recordExp(Tr_expList list, int n);
Tr_exp Tr_arrayExp(Tr_exp init, Tr_exp size);
Tr_exp Tr_seqExp(Tr_expList trl);
Tr_exp Tr_assignExp(Tr_exp left, Tr_exp right);
Tr_exp Tr_ifExp(Tr_exp test, Tr_exp then, Tr_exp elsee);
Tr_exp Tr_noExp();
Tr_exp Tr_breakExp(Tr_exp breakk);
Tr_exp Tr_doneLabel();
Tr_exp Tr_whileExp(Tr_exp breakk, Tr_exp cond, Tr_exp body);
Tr_exp Tr_callExp(Temp_label label, Tr_level fun, Tr_level call, Tr_expList l);

Tr_exp Tr_varDec(Tr_exp lval, Tr_exp init);
Tr_exp Tr_typeDec();
Tr_exp Tr_funDec(Tr_expList bodylist);


#endif