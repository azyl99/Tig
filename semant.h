
/*  */

struct expty {
	Tr_exp exp; // 已转换的中间代码表达式
	Ty_ty ty;	// 该表达式的类型
};


struct expty transVar(S_table venv, S_table tenv, A_var v);
struct expty transExp(S_table venv, S_table tenv, A_exp a);
void         transDec(S_table venv, S_table tenv, A_dec d);
       Ty_ty transTy (              S_table tenv, A_ty a);


