#include <stdio.h>
#include "util.h"
#include "errormsg.h"
#include "symbol.h"
#include "absyn.h"
#include "types.h"
#include "env.h"
#include "translate.h"
#include "semant.h"


#include "prabsyn.h"
static Ty_tyList makeFormalTyList(S_table tenv, A_fieldList params);
static U_boolList makeFormalBoolList(A_fieldList params);

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

struct expty transVar(Tr_level level, S_table venv, S_table tenv, A_var v)
{
	switch (v->kind) {
	case A_simpleVar: {
		E_enventry x = S_look(venv, v->u.simple);
		if (!x || x->kind != E_varEntry) {
			EM_error(v->pos, "undefined variable %s", S_name(v->u.simple));
			return expTy(NULL, Ty_Int());// 未定义的变量默认是int类型
		}
		Tr_access ta = x->u.var.access;
		Tr_exp te = Tr_simpleVar(ta, level);
		return expTy(te, actual_ty(x->u.var.ty));
	}
	case A_fieldVar: {
		// a.b   (field.)   var sym
		E_enventry x = S_look(venv, v->u.field.var->u.simple);// Ty_record
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
		}

		return expTy(NULL, f->ty);
	}
	case A_subscriptVar: {
		// a[b]  (subscript.) var exp
		E_enventry x = S_look(venv, v->u.subscript.var->u.simple);
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
		return expTy(te, ty->u.array);
	}
	}
	// ...
	assert(0);
	struct expty e;
	return e;
}

struct expty transExp(Tr_level level, S_table venv, S_table tenv, A_exp a)
{
	switch (a->kind) {
	case A_varExp: {
		return transVar(level, venv, tenv, a->u.var);
	}
	case A_nilExp: {
		return expTy(NULL, Ty_Nil());
	}
	case A_voidExp: {
		return expTy(NULL, Ty_Void());
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
			return expTy(NULL, Ty_Int());	// 未定义的函数，默认Ty_Int()
		}
		Ty_tyList tl; A_expList al; int i;
		for (tl = x->u.fun.formals,al = a->u.call.args,i=1; al&&tl; al=al->tail,tl=tl->tail,i++) {
			// 查看各参数类型是否正确
			struct expty e = transExp(level, venv, tenv, al->head);
			if (e.ty->kind != tl->head->kind && !(e.ty->kind == Ty_nil && tl->head->kind == Ty_record))
				EM_error(a->pos, "assign incompatible type to argument %d of function '%s'", i, S_name(a->u.call.func));
		}
		if (al || tl) {
			EM_error(a->pos, "formal and actual arguments of function '%s' are not equal", S_name(a->u.call.func));
		}
		return expTy(NULL, x->u.fun.result);
	}
	case A_opExp: {
		A_oper oper = a->u.op.oper;
		struct expty left = transExp(level, venv, tenv, a->u.op.left);
		struct expty right = transExp(level, venv, tenv, a->u.op.right);
		switch (oper) {
		case A_plusOp: case A_minusOp: case A_timesOp: case A_divideOp: {
			if (left.ty->kind != Ty_int)
				EM_error(a->u.op.left->pos, "integer required for arithmetic operator");
			if (right.ty->kind != Ty_int)
				EM_error(a->u.op.right->pos, "integer required for arithmetic operator");
			Tr_exp te = Tr_opExp(oper, left.exp, right.exp);
			return expTy(NULL, Ty_Int());
		}
		case A_eqOp: case A_neqOp: case A_ltOp: case A_leOp: case A_gtOp: case A_geOp: {
			if (left.ty->kind != right.ty->kind && !(left.ty->kind == Ty_record && right.ty->kind == Ty_nil) && !(left.ty->kind == Ty_nil && right.ty->kind == Ty_record))
				EM_error(a->u.op.left->pos, "comparation between incompatible types");
			Tr_exp te = Tr_op2Exp(oper - 6, left.exp, right.exp);
			return expTy(NULL, Ty_Int());//返回0或1
		}
		case A_andOp: case A_orOp: {
			Tr_exp te = Tr_opExp(oper, left.exp, right.exp);
			return expTy(NULL, Ty_Int());//返回0或1
		}
		default: assert(0);
		}
		//...
	}
	case A_recordExp: {
	// node{name="zyl", age=19}
		Ty_ty ty = S_look(tenv, a->u.record.typ);// ty: Ty_record
		if (!ty)
			EM_error(a->pos, "undefined type %s", S_name(a->u.record.typ));
		if (ty->kind != Ty_record)
			EM_error(a->pos, "type %s is not record type", S_name(a->u.record.typ));
		
		A_efieldList al;
		Ty_fieldList tl;
		for (al = a->u.record.fields, tl = ty->u.record; al&&tl; al=al->tail,tl=tl->tail) {		
			// 查看字段名称是否正确
			if (!S_isEqual(al->head->name, tl->head->name))
				EM_error(a->pos, "record '%s' expect field name '%s', not '%s'", 
					S_name(a->u.record.typ), S_name(tl->head->name), S_name(al->head->name));
			// 查看字段值类型是否正确
			struct expty e = transExp(level, venv, tenv, al->head->exp);
			if (e.ty->kind != tl->head->ty->kind && !(e.ty->kind == Ty_nil && tl->head->ty->kind == Ty_record))
				EM_error(a->pos, "assign incompatible type to field '%s'", S_name(tl->head->name));
		}
		return expTy(NULL, ty);
	}
	case A_seqExp: {
		struct expty e;
		A_expList l = a->u.seq;
		for ( ; l; l = l->tail) {
			e = transExp(level, venv, tenv, l->head);//中间代码丢失
		}
		return e;
	}
	case A_assignExp: {
		struct expty left = transVar(level, venv, tenv, a->u.assign.var);
		struct expty right = transExp(level, venv, tenv, a->u.assign.exp);
		if (left.ty->kind != right.ty->kind && !(left.ty->kind == Ty_record && right.ty->kind == Ty_nil)) {
			//之所以用u.assign.var的位置是因为u.assign.exp的位置已经是下一行的了
			EM_error(a->u.assign.var->pos, "assign incompatible type to left value");//有可能是其他左值（如 a.b[1] ），无法打印
		}
		return expTy(NULL, Ty_Void());
	}
	case A_ifExp: {
		struct expty test = transExp(level, venv, tenv, a->u.iff.test);
		struct expty then = transExp(level, venv, tenv, a->u.iff.then);
		if (a->u.iff.elsee) {
			struct expty elsee = transExp(level, venv, tenv, a->u.iff.elsee);
			if (then.ty->kind != elsee.ty->kind && !(then.ty->kind==Ty_nil && elsee.ty->kind ==Ty_record) && !(then.ty->kind==Ty_record && elsee.ty->kind ==Ty_nil))
				EM_error(a->u.iff.then->pos, "in if-then-else statement, body of then and else require same type");
			return expTy(NULL, then.ty);
		}
		else {
			if (then.ty->kind != Ty_void)
				EM_error(a->u.iff.then->pos, "body of if-then statement require void type");
			return expTy(NULL, then.ty);
		}
	}
	case A_whileExp: {
	// while exp1 do exp2
	// struct {A_exp test, body;} whilee;
		transExp(level, venv, tenv, a->u.whilee.test);
		struct expty body = transExp(level, venv, tenv, a->u.whilee.body);
		if (body.ty->kind != Ty_void) 
			EM_error(a->u.whilee.body->pos, "body of while expresion require void type");
		return expTy(NULL, body.ty);
	}
	case A_forExp: {
	// for id := exp1 to exp2 ex[3]
	// struct {S_symbol var; A_exp lo,hi,body; bool escape;} forr;
		S_beginScope(venv);
			/*S_enter(venv, a->u.forr.var, E_VarEntry(Ty_Int()));*/
            transDec(level, venv, tenv, A_VarDec(a->pos, a->u.forr.var, S_Symbol("int"), a->u.forr.lo));
			struct expty lo = transExp(level, venv, tenv, a->u.forr.lo);
			struct expty hi = transExp(level, venv, tenv, a->u.forr.hi);
			if (lo.ty->kind != Ty_int)
				EM_error(a->u.forr.lo->pos, "lo of loop statement should be int type");
			if (hi.ty->kind != Ty_int)
				EM_error(a->u.forr.hi->pos, "hi of loop statement should be int type");
			// 还要不可以对var赋值
			// ...
			struct expty body = transExp(level, venv, tenv, a->u.forr.body);
		S_endScope(venv);
		return expTy(NULL, body.ty);
	}
	case A_letExp: {
		struct expty e;
		A_decList d;
		S_beginScope(venv);
		S_beginScope(tenv);
		for (d = a->u.let.decs; d; d = d->tail)
			transDec(level, venv, tenv, d->head);
		e = transExp(level, venv, tenv, a->u.let.body);
		S_endScope(tenv);
		S_endScope(venv);
		return e;
	}
	case A_arrayExp: {
	// intArr [5] of 0
	// typ size init
		Ty_ty ty = S_look(tenv, a->u.array.typ);// ty: Ty_record
		if (!ty)
			EM_error(a->pos, "undefined type %s", S_name(a->u.array.typ));
		ty = actual_ty(ty);
		struct expty size = transExp(level, venv, tenv, a->u.array.size);
		struct expty init = transExp(level, venv, tenv, a->u.array.init);
		if (size.ty->kind != Ty_int) 
			EM_error(a->pos, "integer required for array size");
		if (init.ty->kind != ty->u.array->kind)
			EM_error(a->pos, "assign incompatible type to array element");
		return expTy(NULL, ty);
	} 
	}
	assert(0);
	struct expty e;
	return e;
}

Tr_exp transDec(Tr_level level, S_table venv, S_table tenv, A_dec d)
{
	switch (d->kind) {
	case A_varDec: {
	// var a:int := 12
	//   var typ   init
		struct expty right = transExp(level, venv, tenv, d->u.var.init);//初始化用的表达式
        //printf("var:%s\n", S_name(d->u.var.var));
        Tr_access ta = Tr_allocLocal(level, d->u.var.escape);
		if (d->u.var.typ == NULL) {		// 没有指定类型，由初始表达式类型来决定
			if (right.ty->kind == Ty_nil)	// 待填坑: Ty_nil,Ty_void...
				EM_error(d->pos, "type required for nil variable '%s'", S_name(d->u.var.var));
			S_enter(venv, d->u.var.var, E_VarEntry(ta, right.ty));
		}
		else {
			Ty_ty ty = S_look(tenv, d->u.var.typ); 
			if (!ty)
				EM_error(d->pos, "undefined type '%s'", S_name(d->u.var.typ));
			// right.ty 是右边的表达式类型（有可能是Nil），ty->kind 是指定的
			if (right.ty->kind != ty->kind && right.ty->kind != Ty_nil)
				EM_error(d->pos, "assign incompatible type to variable '%s'", S_name(d->u.var.var));
			S_enter(venv, d->u.var.var, E_VarEntry(ta, ty));//right.ty（有可能是Nil）
		}
		return Tr_varDec(ta, right.exp);
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
			struct expty e = transExp(funcEntry->u.fun.level, venv, tenv, fd->body);//在这个环境下处理函数体
			S_endScope(venv);
			
			E_enventry x = S_look(venv, fd->name);
			resultTy = x->u.fun.result;
			if (resultTy->kind == Ty_void && e.ty->kind != Ty_void)
				EM_error(fd->pos, "function '%s' require its body of type 'void'", S_name(fd->name));
			else if (resultTy->kind != e.ty->kind)
				EM_error(fd->pos, "function '%s' require its body of type '%s'", S_name(fd->name), S_name(fd->result));//建立在有返回值的基础上

		}
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
bool innerIndentifiers(S_symbol name) {
	if (name == S_Symbol("int") || name == S_Symbol("string") || name == S_Symbol("nil") || name == S_Symbol("void")) {
		return TRUE;
	}
	else
		return FALSE;
}
