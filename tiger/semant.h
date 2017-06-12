/*  */
#ifndef _SEMANT_H_
#define _SEMANT_H_

struct expty {
	Tr_exp exp; // 已转换的中间代码表达式
	Ty_ty ty;	// 该表达式的类型
};


struct expty transVar(Tr_level level, Tr_exp breakk, S_table venv, S_table tenv, A_var v);
struct expty transExp(Tr_level level, Tr_exp breakk, S_table venv, S_table tenv, A_exp a);
Tr_exp       transDec(Tr_level level, Tr_exp breakk, S_table venv, S_table tenv, A_dec d);
Ty_ty transTy(S_table tenv, A_ty a);

bool innerIndentifiers(S_symbol name);

#endif // !SEMANT_H_
