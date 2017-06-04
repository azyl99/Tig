#include <stdio.h>
#include "util.h"
#include "errormsg.h"
#include "symbol.h"
#include "absyn.h"
#include "types.h"
#include "env.h"
#include "semant.h"
#include "translate.h"


#include "prabsyn.h"
static Ty_tyList makeFormalTyList(S_table tenv, A_fieldList params);
static U_boolList makeFormalBoolList(A_fieldList params);
static bool ty_match(Ty_ty tt, Ty_ty ee);

// 跳过Ty_Name类型
static Ty_ty actual_ty(Ty_ty ty) {
	while (ty->kind == Ty_name) {
		ty = ty->u.name.ty;
	}
	return ty;
}


struct expty expTy(Tr_exp exp, Ty_ty ty)
{
	struct expty e; e.exp = exp; e.ty = ty;
	return e;
}

struct expty transVar(Tr_level level, Tr_exp breakk, S_table venv, S_table tenv, A_var v)
{
	switch (v->kind) {
	case A_simpleVar: {
		E_enventry x = S_look(venv, v->u.simple);
		if (!x || x->kind != E_varEntry) {
			EM_error(v->pos, "undefined variable %s", S_name(v->u.simple));
			return expTy(Tr_noExp(), Ty_Int());// 未定义的变量默认是int类型
		}
		Tr_access ta = x->u.var.access;
		Tr_exp te = Tr_simpleVar(ta, level);
		return expTy(te, actual_ty(x->u.var.ty));
	}
	case A_fieldVar: {
		// a.b   (field.)   var sym
		/*E_enventry x = S_look(venv, v->u.field.var->u.simple);// Ty_record
		if (!x || x->kind != E_varEntry) {
			EM_error(v->pos, "undefined record %s", S_name(v->u.field.var->u.simple));
			return expTy(NULL, Ty_Int());// 未定义的变量默认是int类型
		}
		Ty_ty ty = x->u.var.ty;
		if (ty->kind != Ty_record) {
			EM_error(v->pos, "variable %s require record type", S_name(v->u.field.var->u.simple));
			return expTy(NULL, Ty_Int());// 默认是int类型
		}
		Ty_fieldList fl; Ty_field f;
		for (fl=ty->u.record; fl; fl=fl->tail) {
			f=fl->head;
			if (S_isEqual(f->name, v->u.field.sym))
				break;
		}
		if (!fl) {
			EM_error(v->pos, "record variable '%s' do not have field named '%s'", S_name(v->u.field.var->u.simple), S_name(v->u.field.sym));
			return expTy(NULL, Ty_Int());
		}*/

		//a.b.c形式的fieldVar
		struct expty var= transVar(level, breakk, venv, tenv, v->u.field.var);
		if (var.ty->kind != Ty_record) {
			EM_error(v->pos, "not a record type"); 
		}
		Ty_fieldList fieldList;
		int num = 0;
		for (fieldList = var.ty->u.record; fieldList; fieldList = fieldList->tail) {
			if (fieldList->head->name == v->u.field.sym) {
				return expTy(Tr_fieldVar(var.exp, num),actual_ty(fieldList->head->ty));
			}
			num++;
		}
		EM_error(v->pos, "no corresponding field of the variable");
		return expTy(Tr_noExp(), Ty_Int());
	}
	case A_subscriptVar: {
		// a[b]  (subscript.) var exp
		/*E_enventry x = S_look(venv, v->u.subscript.var->u.simple);
		if (!x || x->kind != E_varEntry) {
			EM_error(v->pos, "undefined variable '%s'", S_name(v->u.subscript.var->u.simple));
			return expTy(NULL, Ty_Int());// 未定义的变量默认是int类型
		}
		Ty_ty ty = x->u.var.ty; // Ty_array
		if (ty->kind != Ty_array) {
			EM_error(v->pos, "variable '%s' require array type", S_name(v->u.subscript.var->u.simple));
			return expTy(NULL, Ty_Int());// 默认是int类型
		}
		struct expty e = transExp(level, venv, tenv, v->u.subscript.exp);
		if (e.ty->kind != Ty_int) {
			EM_error(v->pos, "index of array '%s' require integer type", S_name(v->u.subscript.var->u.simple));
			return expTy(NULL, Ty_Int());// 默认是int类型
		}
		Tr_exp base = Tr_simpleVar(x->u.var.access, level);
		Tr_exp index = e.exp;
		Tr_exp te = Tr_subscriptVar(base, index);
		return expTy(te, ty->u.array);*/

		struct expty var = transVar(level, breakk, venv, tenv, v->u.subscript.var);
		if (var.ty->kind != Ty_array) {
			EM_error(v->pos, "not an array type");
			return expTy(Tr_noExp(), Ty_Int());// 默认是int类型
		}
		else {
			struct expty exp= transExp(level, breakk, venv, tenv, v->u.subscript.exp);
			if (exp.ty->kind != Ty_int) {
				EM_error(v->pos, "int required");
				return expTy(Tr_noExp(), Ty_Int());// 默认是int类型
			}
			else {
				return expTy(Tr_subscriptVar(var.exp, exp.exp), actual_ty(var.ty->u.array));
			}
		}

	}
	default:
		assert(0);
	}
}

struct expty transExp(Tr_level level, Tr_exp breakk, S_table venv, S_table tenv, A_exp a)
{
	switch (a->kind) {
	case A_varExp: {
		return transVar(level, breakk, venv, tenv, a->u.var);
	}
	case A_nilExp: {
		return expTy(Tr_noExp(), Ty_Nil());
	}
	case A_voidExp: {
		return expTy(Tr_noExp(), Ty_Void());
	}
	case A_intExp: {
		Tr_exp te = Tr_intExp(a->u.intt);
		return expTy(te, Ty_Int());
	}
	case A_stringExp: {
		Tr_exp te = Tr_stringExp(a->u.stringg);
		return expTy(te, Ty_String());
	}
	case A_callExp: {

	// f(a, b)
	// func args :   call   S_symbol func, A_expList args
		E_enventry x = S_look(venv, a->u.call.func);
		if (!x || x->kind != E_funEntry) {
			EM_error(a->pos, "undefined function %s", S_name(a->u.call.func));
		
			return expTy(Tr_noExp(), Ty_Void());	// 未定义的函数，默认Ty_Int()
		}
		Ty_tyList tl; A_expList al; int i;
		Tr_expList head = NULL, tail = NULL;
		for (tl = x->u.fun.formals,al = a->u.call.args,i=1; al&&tl; al=al->tail,tl=tl->tail,i++) {
			// 查看各参数类型是否正确
			struct expty e = transExp(level, breakk, venv, tenv, al->head);
			if (!ty_match(e.ty,tl->head))
				EM_error(a->pos, "assign incompatible type to argument %d of function '%s'", i, S_name(a->u.call.func));
			if (head) {
				tail->tail = Tr_ExpList(e.exp,NULL);
				tail = tail->tail;
			}
			else {
				head= Tr_ExpList(e.exp, NULL);
				tail = head;
			}
		}
		if (al || tl) {
			EM_error(a->pos, "formal and actual arguments of function '%s' are not equal", S_name(a->u.call.func));
		}
		else {
			if (x->u.fun.result)
				return expTy(Tr_callExp(x->u.fun.label, x->u.fun.level, level, head), x->u.fun.result);
			else
				return expTy(Tr_callExp(x->u.fun.label, x->u.fun.level, level, head), Ty_Void());
		}
		return expTy(Tr_noExp(), x->u.fun.result);
	}
	case A_opExp: {
		A_oper oper = a->u.op.oper;
		struct expty left = transExp(level, breakk, venv, tenv, a->u.op.left);
		struct expty right = transExp(level, breakk, venv, tenv, a->u.op.right);
		switch (oper) {
		case A_plusOp: case A_minusOp: case A_timesOp: case A_divideOp: { //要求整型操作数
			if (left.ty->kind != Ty_int)
				EM_error(a->u.op.left->pos, "integer required for arithmetic operator");
			else if (right.ty->kind != Ty_int)
				EM_error(a->u.op.right->pos, "integer required for arithmetic operator");
			else {
				Tr_exp te = Tr_arithopExp(oper, left.exp, right.exp);
				return expTy(te, Ty_Int());
			}
		}
		case A_eqOp: case A_neqOp:{
			if (ty_match(left.ty, right.ty)) {
				if (left.ty->kind == Ty_int) {
					return expTy(Tr_eqopExp(oper, left.exp, right.exp), Ty_Int());
				}
				else if (left.ty->kind == Ty_record || right.ty->kind == Ty_record) {
					return expTy(Tr_eqopExp(oper, left.exp, right.exp), Ty_Int());
				}
				else if (left.ty->kind == Ty_array) {
					return expTy(Tr_eqopExp(oper, left.exp, right.exp), Ty_Int());
				}
				else if (left.ty->kind == Ty_string) {
					return expTy(Tr_eqstringExp(oper, left.exp, right.exp), Ty_Int());
				}
			}
		}
		case A_ltOp: case A_leOp: case A_gtOp: case A_geOp:{
			if (ty_match(left.ty, right.ty)) {
				if (left.ty->kind == Ty_int) {
					return expTy(Tr_eqopExp(oper, left.exp, right.exp), Ty_Int());
				}
				else if (left.ty->kind == Ty_string) {
					return expTy(Tr_eqstringExp(oper, left.exp, right.exp), Ty_Int());
				}
			}
		}
		case A_andOp: case A_orOp: {
			return expTy(Tr_logicExp(oper,left.exp,right.exp) , Ty_Int());//返回0或1
		}
		default: 
			EM_error(a->pos, "The operator %d is not accepted", a->u.op.oper);
		}
		return expTy(Tr_noExp(), Ty_Int());
	}
	case A_recordExp: {
	// node{name="zyl", age=19}
		Ty_ty ty = actual_ty(S_look(tenv, a->u.record.typ));// ty: Ty_record
		if (!ty)
			EM_error(a->pos, "undefined type %s (debug recordExp)", S_name(a->u.record.typ));
		else {
			if (ty->kind != Ty_record)
				EM_error(a->pos, "type %s is not record type", S_name(a->u.record.typ));
			else {
				A_efieldList al;
				Ty_fieldList tl;
				for (al = a->u.record.fields, tl = ty->u.record; al&&tl; al = al->tail, tl = tl->tail) {
					// 查看字段名称是否正确
					if (!S_isEqual(al->head->name, tl->head->name)) {
						EM_error(a->pos, "record '%s' expect field name '%s', not '%s'",
							S_name(a->u.record.typ), S_name(tl->head->name), S_name(al->head->name));
						return expTy(Tr_noExp(), Ty_Int());
					}
					// 查看字段值类型是否正确
					struct expty e = transExp(level, breakk, venv, tenv, al->head->exp);
					if (e.ty->kind != tl->head->ty->kind && !(e.ty->kind == Ty_nil && tl->head->ty->kind == Ty_record)) {
						EM_error(a->pos, "assign incompatible type to field '%s'", S_name(tl->head->name));
						return expTy(Tr_noExp(), Ty_Int());
					}
				}
				Tr_expList trl = NULL;
				int num = 0;
				//将字段值表达式转换成Tr_expList列表
				for (al = a->u.record.fields; al; al = al->tail) {
					num++;
					struct expty e = transExp(level, breakk, venv, tenv, al->head->exp);
					trl = Tr_ExpList(e.exp, trl);
				}
				return expTy(Tr_recordExp(trl, num), ty);
			}
		}
		return expTy(Tr_noExp(), Ty_Int());
	}
	case A_seqExp: {
		struct expty e;
		A_expList l = a->u.seq;
		Tr_expList trl = NULL;
		for ( ; l; l = l->tail) {
			e = transExp(level, breakk, venv, tenv, l->head);//中间代码丢失
			trl = Tr_ExpList(e.exp, trl);
		}
		return expTy(Tr_seqExp(trl),e.ty);
	}
	case A_assignExp: {
		struct expty left = transVar(level, breakk, venv, tenv, a->u.assign.var);
		struct expty right = transExp(level, breakk, venv, tenv, a->u.assign.exp);
		if (left.ty->kind != right.ty->kind && !(left.ty->kind == Ty_record && right.ty->kind == Ty_nil)) {
			//之所以用u.assign.var的位置是因为u.assign.exp的位置已经是下一行的了
			EM_error(a->u.assign.var->pos, "assign incompatible type to left value");//有可能是其他左值（如 a.b[1] ），无法打印
		}
		else {
			return expTy(Tr_assignExp(left.exp, right.exp), Ty_Void());
		}
		return expTy(Tr_noExp(), Ty_Void());
	}
	case A_ifExp: {
		struct expty test = transExp(level, breakk, venv, tenv, a->u.iff.test);
		struct expty then = transExp(level, breakk, venv, tenv, a->u.iff.then);
		struct expty elsee = expTy(NULL, NULL);
		if (a->u.iff.elsee) {
			elsee = transExp(level, breakk, venv, tenv, a->u.iff.elsee);
			if (!ty_match(then.ty, elsee.ty)) {
				EM_error(a->pos, "else and then don't have same type");
			}
			else if (test.ty->kind != Ty_int) {
				EM_error(a->u.iff.test->pos, "int required for test");
			}
			else {
				return expTy(Tr_ifExp(test.exp, then.exp, elsee.exp), then.ty);
			}
		}

		return expTy(Tr_noExp(), then.ty);
	}
	case A_whileExp: {
	// while exp1 do exp2
	// struct {A_exp test, body;} whilee;
		struct expty e = transExp(level, breakk, venv, tenv, a->u.whilee.test);
		if (e.ty->kind != Ty_int) {
			EM_error(a->u.whilee.test->pos, "body of test expresion require int type");
		}
		else {
			Tr_exp done = Tr_doneLabel();
			struct expty body = transExp(level, done, venv, tenv, a->u.whilee.body);
			if (body.ty->kind != Ty_void)
				EM_error(a->u.whilee.body->pos, "body of while expresion require void type");
			else {
				return expTy(Tr_whileExp(done, e.exp, body.exp), Ty_Void());
			}
		}
		return expTy(Tr_noExp(),Ty_Void());
	}
	case A_forExp: {
	// for id := exp1 to exp2 ex[3]
	// struct {S_symbol var; A_exp lo,hi,body; bool escape;} forr;
		//将for构建成let,while结合的抽象语法树
		A_decList letDec = A_DecList(A_VarDec(a->pos, a->u.forr.var, S_Symbol("int"), a->u.forr.lo),
			A_DecList(A_VarDec(a->pos, S_Symbol("limit"), S_Symbol("int"), a->u.forr.hi), NULL));
		A_exp whilee = A_WhileExp(a->pos, A_OpExp(a->pos, A_leOp, A_VarExp(a->pos, A_SimpleVar(a->pos, a->u.forr.var)),
			A_VarExp(a->pos, A_SimpleVar(a->pos, S_Symbol("limit")))),
			A_SeqExp(a->pos, A_ExpList(a->u.forr.body,
				A_ExpList(A_AssignExp(a->pos,
					A_SimpleVar(a->pos, a->u.forr.var),
					A_OpExp(a->pos, A_plusOp,
						A_VarExp(a->pos, A_SimpleVar(a->pos, a->u.forr.var)),
						A_IntExp(a->pos, 1))), NULL))));
		A_exp letExp = A_LetExp(a->pos, letDec, whilee);
		return transExp(level, breakk, venv, tenv, letExp);
	}
	case A_letExp: {
		A_decList d;
		Tr_expList trl = NULL;
		struct expty t;
		S_beginScope(venv);
		S_beginScope(tenv);
		for (d = a->u.let.decs; d; d = d->tail) {
			trl = Tr_ExpList(transDec(level, breakk, venv, tenv, d->head),trl);
		}
		t = transExp(level, breakk, venv, tenv, a->u.let.body);
		trl = Tr_ExpList(t.exp, trl);
		S_endScope(tenv);
		S_endScope(venv);
		return expTy(Tr_seqExp(trl),t.ty);
	}
	case A_arrayExp: {
	// intArr [5] of 0
	// typ size init
		Ty_ty ty = actual_ty(S_look(tenv, a->u.array.typ));// ty: Ty_record
		if (!ty)
			EM_error(a->pos, "undefined type %s", S_name(a->u.array.typ));
		else {
			struct expty size = transExp(level, breakk, venv, tenv, a->u.array.size);
			struct expty init = transExp(level, breakk, venv, tenv, a->u.array.init);
			if (size.ty->kind != Ty_int)
				EM_error(a->pos, "integer required for array size");
			else if(init.ty->kind != ty->u.array->kind)
				EM_error(a->pos, "assign incompatible type to array element");
			else {
				return expTy(Tr_arrayExp(init.exp, size.exp), ty);
			}
		}
		return expTy(NULL, ty);
	} 
	case A_breakExp: {
		if (!breakk) {
			EM_error(a->pos, "Break should be used in while or for loop");
		}
		else {
			return expTy(Tr_breakExp(breakk), Ty_Void());
		}
		return expTy(Tr_noExp(), Ty_Void());
	}
	default:assert(0); 
	}
}

Tr_exp transDec(Tr_level level, Tr_exp breakk, S_table venv, S_table tenv, A_dec d)
{
	switch (d->kind) {
	case A_varDec: {
	// var a:int := 12
	//   var typ   init
		struct expty right = transExp(level, breakk, venv, tenv, d->u.var.init);//初始化用的表达式
        //printf("var:%s\n", S_name(d->u.var.var));
        Tr_access ta = Tr_allocLocal(level, d->u.var.escape);
		if (d->u.var.typ == NULL) {		// 没有指定类型，由初始表达式类型来决定
			if (right.ty->kind == Ty_nil)	// 待填坑: Ty_nil,Ty_void...
				EM_error(d->pos, "type required for nil variable '%s'", S_name(d->u.var.var));
			S_enter(venv, d->u.var.var, E_VarEntry(ta, right.ty));
		}
		else {
			Ty_ty ty = actual_ty(S_look(tenv, d->u.var.typ));
			if (!ty)
				EM_error(d->pos, "undefined type '%s'", S_name(d->u.var.typ));
			// right.ty 是右边的表达式类型（有可能是Nil），ty->kind 是指定的
			if (!ty_match(ty,right.ty))
				EM_error(d->pos, "assign incompatible type to variable '%s'", S_name(d->u.var.var));
			S_enter(venv, d->u.var.var, E_VarEntry(ta, ty));//right.ty（有可能是Nil）
		}
		return Tr_varDec(Tr_simpleVar(ta,level), right.exp);
	}
	case A_typeDec: {

	// type INT= int   type list = {first:int, rest:list}
	// A_namety A_Namety(S_symbol name, A_ty ty);
	// A_nametyList A_NametyList(A_namety head, A_nametyList tail)
		A_nametyList n;		// typedefList
		// 把所有的type头送入类型表里面，假装它们已经定义
		for (n = d->u.type; n; n = n->tail) {
			if (innerIndentifiers(n->head->name)) {  //如果定义int ,或者string
				EM_error(n->head->ty->pos, "predefined type %s", S_name(n->head->name));
				continue;
			}
			S_enter(tenv, n->head->name, Ty_Name(n->head->name, NULL));
		}
		// 把各record的类型由Ty_name修改为Ty_fieldList
		for (n = d->u.type; n; n = n->tail) {
			S_enter(tenv, n->head->name, transTy(tenv, n->head->ty));
		}
		// 把record的field类型为Ty_nameTy的部分由全部改成Ty_fieldList
		// 已验证正确，去掉下面的语句，则定义自递归的时候会出错: type list = {first:int, rest:list}
		for (n = d->u.type; n; n = n->tail) {
			Ty_ty x = S_look(tenv, n->head->name);
			if (x->kind == Ty_name)
				S_enter(tenv, n->head->name, transTy(tenv, n->head->ty));
		}
		return Tr_typeDec();
		break;

	}
	case A_functionDec: {// 没有返回值是Ty_void类型
	// function f(a:int, b:tree): string = body -->[fd]
	// A_Fundec(A_pos pos, S_symbol name, A_fieldList params, S_symbol result, A_exp body)
	// A_FundecList(head, tail)
		A_fundecList a;		
		A_fundec fd;
		Ty_ty resultTy;
		Ty_tyList formalTys;
		// 把所有的函数名送入类型表里面，假装它们已经定义
		for (a = d->u.function; a; a = a->tail) {
			fd = a->head;
            Temp_label funcLabel = Temp_newlabel();
           // printf("function:%s\n", S_name(fd->name));
            Tr_level tl = Tr_newLevel(level, funcLabel, makeFormalBoolList(fd->params));  //新建level
			formalTys = makeFormalTyList(tenv, fd->params); // 参数类型表
			if (fd->result) {// 函数有返回值
				resultTy = S_look(tenv, fd->result);
				if (!resultTy)
					EM_error(fd->pos, "undefined type %s", S_name(fd->result));
				S_enter(venv, fd->name, E_FunEntry(tl, funcLabel, formalTys, resultTy));	// 把函数加入全局函数表
			}
			else { // 函数没有返回值（此时是过程）
				S_enter(venv, fd->name, E_FunEntry(tl, funcLabel, formalTys, Ty_Void()));	// 把函数加入全局函数表
			}
		}
		// 处理函数参数以及函数体，Tiger语言在函数定义时就进行类型检查
		for (a = d->u.function; a; a = a->tail) {
			fd = a->head;
			formalTys = makeFormalTyList(tenv, fd->params); // 耗时，可优化
            E_enventry funcEntry = S_look(venv, fd->name);     //拿到函数的信息
            Tr_accessList al = Tr_formals(funcEntry->u.fun.level);
			S_beginScope(venv);
			{//把定义限制在这个范围内
				A_fieldList af; Ty_tyList tt;
				for (af=fd->params, tt=formalTys; af&&tt&&al; af=af->tail, tt=tt->tail, al=al->tail) {
					S_enter(venv, af->head->name, E_VarEntry(al->head, tt->head));//tt->head是Ty_ty类型
				}
			}
			struct expty e = transExp(funcEntry->u.fun.level, breakk, venv, tenv, fd->body);//在这个环境下处理函数体
			E_enventry x = S_look(venv, fd->name);
			resultTy = x->u.fun.result;
			if (!ty_match(resultTy, e.ty))
				EM_error(fd->pos, "incorrect return type for function  %s", S_name(fd->name));
			Tr_procEntryExit(x->u.fun.level, e.exp, al);
			S_endScope(venv);
		}
		return Tr_noExp();
		break;
	}
	default:
		assert(0);
		break;
	}
}

Ty_ty transTy (S_table tenv, A_ty a)
{
	switch (a->kind) {
	case A_nameTy: {
	// type INT = int
	// a= Ty_Name(A_Namety("int"), Ty_Int())
		Ty_ty x = S_look(tenv, a->u.name);// x = tenv["int"] = Ty_Int()
		if (!x) {
			EM_error(a->pos, "undefined type %s", S_name(a->u.name));
			return Ty_Int();//未定义的类型默认是int类型
		} 
		if (x->kind == Ty_name && x->u.name.ty != NULL ) {// 提前定义了一个未定义的类型
			// type a = b    type b = a
			// x = Ty_Name("b", NULL)
			EM_error(a->pos, "undefined type %s", S_name(x->u.name.sym));
			return Ty_Int();//未定义的类型默认是int类型
		}
		return x;
	}
	case A_recordTy: {
	// {key:int, s:string}
	// a=A_RecordTy(A_FieldList(A_Field("key", "int"), tail))
		A_fieldList af = NULL;
		Ty_fieldList head = Ty_FieldList(NULL, NULL), p = head;
		for (af = a->u.record; af; af = af->tail) {
			// af->head = A_Field("key", "int")
			Ty_ty x = S_look(tenv, af->head->typ);
			if (!x) {// x = tenv["int"] = Ty_Int()
				EM_error(a->pos, "undefined type %s", S_name(af->head->typ));
				continue;
			}
			p->tail = Ty_FieldList(Ty_Field(af->head->name, x), NULL);
			p = p->tail;
		}
		return Ty_Record(head->tail);
	}
	case A_arrayTy: {
	// = array of int
	// a=A_ArrayTy(S_Symbol("int"))
		Ty_ty x = S_look(tenv, a->u.array);// x = tenv["int"] = Ty_Int()
		if (!x) {
			EM_error(a->pos, "undefined type %s", S_name(a->u.name));
			return Ty_Int();//未定义的类型默认是int类型
		}
		if (x->kind == Ty_name) {// 提前定义了一个未定义的类型，同case A_nameTy
			EM_error(a->pos, "undefined type %s", S_name(x->u.name.sym));
			return Ty_Int();//未定义的类型默认是int类型
		}
		return Ty_Array(x);
	}
	}
	
	assert(0);
	Ty_ty ty;
	return ty;
}



static Ty_tyList makeFormalTyList(S_table tenv, A_fieldList params)
{
	Ty_tyList head = Ty_TyList(NULL, NULL), p = head;
	A_fieldList af;	A_field field;
	for (af = params; af; af = af->tail) {
		field = af->head;
		//顺序有误
		Ty_ty ty = S_look(tenv, field->typ);
		if (!ty)
			EM_error(field->pos, "undefined type %s", S_name(field->name));
		
		p->tail = Ty_TyList(ty, NULL);
		p = p->tail;
	}
	return head->tail;
}

static U_boolList makeFormalBoolList(A_fieldList params){
    U_boolList head = NULL, tail = NULL;
    A_fieldList p = NULL;
    for(p = params; p; p = p->tail){
        if(head){
            tail->tail = U_BoolList(p->head->escape, NULL);
            tail = tail->tail;
        }
        else{
            head = U_BoolList(p->head->escape, NULL);
            tail = head;
        }
    }
    return head;
}

static bool ty_match(Ty_ty tt, Ty_ty ee) {
	Ty_ty t = actual_ty(tt);
	Ty_ty e = actual_ty(ee);
	int tk = t->kind;
	int ek = e->kind;

	return (((tk == Ty_record || tk == Ty_array) && t == e) ||
		(tk == Ty_record && ek == Ty_nil) ||
		(ek == Ty_record && tk == Ty_nil) ||
		(tk != Ty_record && tk != Ty_array && tk == ek));
}

bool innerIndentifiers(S_symbol name) {
	if (name == S_Symbol("int") || name == S_Symbol("string") || name == S_Symbol("nil") || name == S_Symbol("void")) {
		return TRUE;
	}
	else
		return FALSE;
}
