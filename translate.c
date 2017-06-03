#include "util.h"
#include "tree.h"
#include "translate.h"

static Tr_level outermost = NULL;
void Tr_print_access(Tr_access a);
void Tr_print_level(Tr_level l);

static patchList PatchList(Temp_label *head, patchList tail);
static void doPatch(patchList tList, Temp_label label);
static patchList joinPatch(patchList first, patchList second);
static Tr_exp Tr_Ex(T_exp ex);  
static Tr_exp Tr_Nx(T_stm ex);
static Tr_exp Tr_Cx(patchList trues, patchList falses, T_stm stm);
static T_exp unEx(Tr_exp e);
static T_stm unNx(Tr_exp e);
static struct Cx unCx(Tr_exp e);
static Tr_exp Tr_StaticLink(Tr_level now, Tr_level def);
static T_expList Tr_expList_convert(Tr_expList l);

struct Tr_level_
{
    Temp_label      name; 
    Tr_level        parent;
    Tr_accessList   formals; 
    F_frame         frame;           
};

struct Tr_access_
{
    Tr_level    level;
    F_access    access;
};

Tr_access Tr_Access(Tr_level level, F_access access){
    Tr_access ta = checked_malloc(sizeof(*ta));
    ta->level = level;
    ta->access = access;
    return ta;
}

Tr_accessList Tr_AccessList(Tr_access head, Tr_accessList tail){
    Tr_accessList tal = checked_malloc(sizeof(*tal));
    tal->head = head;
    tal->tail = tail;
    return tal;
}

Tr_level Tr_newLevel(Tr_level parent, Temp_label name, U_boolList formals){
    Tr_level level = checked_malloc(sizeof(*level));
    level->name = name;
    level->parent = parent;
    //level->frame = F_newFrame(name, formals);
	level->frame = F_newFrame(name, U_BoolList(TRUE, formals)); //留第一个参数位置给静态链
    Tr_accessList head = NULL, tail = NULL;
    //solve with static link
    //F_accessList fal = F_formals(level->frame)->tail;
    F_accessList fal = F_formals(level->frame);
    for(;fal;fal = fal->tail){
        Tr_access ta = Tr_Access(level, fal->head);
        if(head){
            tail->tail = Tr_AccessList(ta, NULL);
            tail = tail->tail;
        }
        else{
            head = Tr_AccessList(ta, NULL);
            tail = head;
        }
    }
    level->formals = head;
    //Tr_print_level(level);
    return level;
}

Tr_level Tr_outermost(void){
    if(!outermost)
        outermost = Tr_newLevel(NULL, Temp_newlabel(), NULL);
    return outermost;
}

Tr_accessList Tr_formals(Tr_level level){
    return level->formals;
}

Tr_access Tr_allocLocal(Tr_level level, bool escape){
    Tr_access ta = Tr_Access(level, F_allocLocal(level->frame, escape));
    //Tr_print_access(ta);
    return ta;
}

void Tr_print_access(Tr_access a){
    printf("tr_level:%s\n",Temp_labelstring(a->level->name));
    F_print_access(a->access);
}

void Tr_print_level(Tr_level l){
    printf("level_name:%s\n",Temp_labelstring(l->name));
    if(l->parent)
        printf("level_parent:%s\n",Temp_labelstring(l->parent->name));
    else
        printf("level_parent:root\n");
    F_print_frame(l->frame);

}

static patchList PatchList(Temp_label *head, patchList tail) {
	patchList pal = checked_malloc(sizeof(*pal));
	pal->head = head;
	pal->tail = tail;
	return pal;
}

static void doPatch(patchList tList, Temp_label label) {
	for (; tList; tList = tList->tail) {
		*(tList->head) = label;
	}
}

//将第二条真值表加在第一条末尾
static patchList joinPatch(patchList first, patchList second) {
	if (!first) return second;
	patchList pl = first;
	for (; first->tail; first = first->tail);
	first->tail = second;
	return pl; //???
}

static Tr_exp Tr_Ex(T_exp ex) {
	Tr_exp te = checked_malloc(sizeof(*te));
	te->kind = Tr_ex;
	te->u.ex = ex;
	return te;
}

static Tr_exp Tr_Nx(T_stm ex) {
	Tr_exp te = checked_malloc(sizeof(*te));
	te->kind = Tr_nx;
	te->u.nx = ex;
	return te;
}
static Tr_exp Tr_Cx(patchList trues, patchList falses, T_stm stm) {
	Tr_exp te = checked_malloc(sizeof(*te));
	te->kind = Tr_cx;
	te->u.cx.trues = trues;
	te->u.cx.falses = falses;
	te->u.cx.stm = stm;
}

static T_exp unEx(Tr_exp e) {
	/*trans Tr_exp to T_exp*/
	switch (e->kind) {
	case Tr_ex:
		return e->u.ex;
	case Tr_nx:
		return T_Eseq(e->u.nx, T_Const(0));
	case Tr_cx: {
		Temp_temp r = Temp_newtemp(); /* temp for save exp-val */
		Temp_label t = Temp_newlabel(), f = Temp_newlabel(); /* actually ture-label & false-label added here */
		doPatch(e->u.cx.trues, t);
		doPatch(e->u.cx.falses, f);
		return T_Eseq(T_Move(T_Temp(r), T_Const(1)),
			T_Eseq(e->u.cx.stm,
				T_Eseq(T_Label(f),
					T_Eseq(T_Move(T_Temp(r), T_Const(0)),
						T_Eseq(T_Label(t), T_Temp(r))))));
	}
	}
	assert(0 && "only 3 condition");
}

static T_stm unNx(Tr_exp e) {
	/*trans Tr_exp to T_stm*/
	switch (e->kind) {
	case Tr_ex:
		return T_Exp(e->u.ex);
	case Tr_nx:
		return e->u.nx;
	case Tr_cx: {
		Temp_temp r = Temp_newtemp(); /*temp for save exp-val*/
		Temp_label t = Temp_newlabel(), f = Temp_newlabel(); /* ture-label & false-label added here */
		doPatch(e->u.cx.trues, t);
		doPatch(e->u.cx.falses, f);
		return T_Exp(T_Eseq(T_Move(T_Temp(r), T_Const(1)),
			T_Eseq(e->u.cx.stm,
				T_Eseq(T_Label(f),
					T_Eseq(T_Move(T_Temp(r), T_Const(0)),
						T_Eseq(T_Label(t), T_Temp(r)))))));

	}
	}
	assert(0);
}

static struct Cx unCx(Tr_exp e) {
	switch (e->kind) {
	case Tr_cx:
		return e->u.cx;
	case Tr_ex: {
		struct Cx cx;
		/*there is no real-label in patchList*/
		cx.stm = T_Cjump(T_ne, e->u.ex, T_Const(0), NULL, NULL);
		cx.trues = PatchList(&(cx.stm->u.CJUMP.true), NULL);
		cx.falses = PatchList(&(cx.stm->u.CJUMP.false), NULL);
		return cx;
	}
	case Tr_nx:
		assert(0); /*this should not occur*/
	}
	assert(0);
}



Tr_exp Tr_intExp(int consti) {
	T_exp te = T_Const(consti);
	return Tr_Ex(te);
}

static F_fragList stringFragList = NULL;
Tr_exp Tr_stringExp(string val) {
	Temp_label slabel = Temp_newlabel();
	F_frag fragment = F_StringFrag(slabel, val);
	stringFragList = F_FragList(fragment, stringFragList);
	return Tr_Ex(T_Name(slabel));
}

Tr_exp Tr_simpleVar(Tr_access ta, Tr_level tl) {
	T_exp addr = T_Temp(F_FP());
	while (tl != ta->level) { //追踪静态链
		F_access sl = F_formals(tl->frame)->head;
		addr = F_Exp(sl, addr);
		tl = tl->parent;
	}
	return Tr_Ex(F_Exp(ta->access, addr));
}

Tr_exp Tr_fieldVar(Tr_exp var, int offset) {
	return Tr_Ex(T_Mem(T_Binop(T_plus, unEx(var), T_Const(offset*FRAME_WORD_SIZE))));
}

Tr_exp Tr_subscriptVar(Tr_exp base, Tr_exp index) {
	return Tr_Ex(T_Mem(T_Binop(T_plus, unEx(base), T_Binop(T_mul,unEx(index),T_Const(FRAME_WORD_SIZE)))));
}

//算数操作 
Tr_exp Tr_arithopExp(A_oper oper, Tr_exp left, Tr_exp right) {
	switch (oper) {
	case A_plusOp:
		return Tr_Ex(T_Binop(T_plus, unEx(left), unEx(right)));
	case A_minusOp:
		return Tr_Ex(T_Binop(T_minus, unEx(left), unEx(right)));
	case A_timesOp:
		return Tr_Ex(T_Binop(T_mul, unEx(left), unEx(right)));
	case A_divideOp:
		return Tr_Ex(T_Binop(T_div, unEx(left), unEx(right)));
	}
}

//布尔操作&和|
Tr_exp Tr_logicExp(A_oper oper, Tr_exp left, Tr_exp right) {
	T_binOp op;
	struct Cx leftt = unCx(left);
	struct Cx rightt = unCx(right);
	switch (oper) {
	case A_andOp: {
		Temp_label z = Temp_newlabel();
		doPatch(leftt.trues, z);
		patchList falses = joinPatch(leftt.falses, rightt.falses);
		patchList trues = rightt.trues;
		return Tr_Cx(trues, falses, T_Seq(leftt.stm,
											T_Seq(T_Label(z),
													rightt.stm)));
	}
	case A_orOp: {
		Temp_label z = Temp_newlabel(); // jump to z if left is right 
		doPatch(leftt.falses, z);
		patchList falses = rightt.falses;
		patchList trues = joinPatch(leftt.trues, rightt.trues);
		return Tr_Cx(trues, falses, T_Seq(leftt.stm,
											T_Seq(T_Label(z),
													rightt.stm)));

	}
	}
}

Tr_exp Tr_eqopExp(A_oper oper, Tr_exp left, Tr_exp right) { //比较整型、数组、指针 均比较的是其值，如数组、记录比较的是地址
	T_relOp op;
	switch (oper) {
	case A_eqOp:
		op = T_eq; break;
	case A_neqOp:
		op = T_ne; break;
	case A_ltOp:
		op = T_lt; break;
	case A_leOp:
		op = T_le; break;
	case A_gtOp:
		op = T_gt; break;
	case A_geOp:
		op = T_ge; break;
	}
	T_stm ts = T_Cjump(op, unEx(left), unEx(right), NULL, NULL);
	patchList trues = PatchList(&ts->u.CJUMP.true, NULL);
	patchList falses = PatchList(&ts->u.CJUMP.false, NULL);
	Tr_exp te = Tr_Cx(trues, falses, ts);
	return te;
}

Tr_exp Tr_eqstringExp(A_oper oper, Tr_exp left, Tr_exp right) {
	
	T_exp resl = F_externalCall(String("stringEqual"), T_ExpList(unEx(left), T_ExpList(unEx(right), NULL)));
	if (oper == A_eqOp) return Tr_Ex(resl);
	else if (oper == A_neqOp) {
		T_exp e = (resl->kind == T_CONST && resl->u.CONST != 0) ? T_Const(0) : T_Const(1);
		return Tr_Ex(e);
	}
	else {
		if (oper == A_ltOp) return (resl->u.CONST < 0) ? Tr_Ex(T_Const(0)) : Tr_Ex(T_Const(1));
		else return (resl->u.CONST > 0) ? Tr_Ex(T_Const(0)) : Tr_Ex(T_Const(1));
	}
}


Tr_exp Tr_recordExp(Tr_expList list, int n) {
	//分配存储空间，地址存入临时变量r
	Temp_temp r = Temp_newtemp();
	T_stm alloc = T_Move(T_Temp(r), F_externalCall(String("initRecord"), T_ExpList(T_Const(n*FRAME_WORD_SIZE), NULL)));

	//将记录的每一个值域值存入空间
	T_stm seq = T_Move(T_Mem(T_Binop(T_plus,T_Temp(r),T_Const(--n*FRAME_WORD_SIZE))), unEx(list->head));
	for (list=list->tail; list; list = list->tail) {
		n--;
		seq = T_Seq(T_Move(T_Mem(T_Binop(T_plus, T_Temp(r), T_Const(n*FRAME_WORD_SIZE))), unEx(list->head)), seq);
	}

	//返回指向记录地址的r
	seq = T_Seq(alloc, seq);
	return Tr_Ex(T_Eseq(seq, T_Temp(r)));
}

Tr_exp Tr_arrayExp(Tr_exp init, Tr_exp size) {
	return Tr_Ex(F_externalCall(String("initArray"), T_ExpList(unEx(size), T_ExpList(unEx(init), NULL))));
}

//根据最后一个表达式是否有返回值，选择SEQ或者ESEQ
Tr_exp Tr_seqExp(Tr_expList trl) {
	Tr_exp reTr = trl->head;
	if (reTr->kind == Tr_nx) {
		T_stm seq = unNx(reTr);
		for (trl = trl->tail; trl; trl = trl->tail) {
			seq = T_Seq(unNx(trl->head), seq);
		}
		return Tr_Nx(seq);
	}
	else {
		T_exp eseq = unEx(reTr);
		for (trl = trl->tail; trl; trl = trl->tail) {
			T_stm ts = trl->head->kind == Tr_cx ? trl->head->u.cx.stm : unNx(trl->head);
			eseq = T_Eseq(unNx(trl->head), eseq);
		}
		return Tr_Ex(eseq);
	}
}

Tr_exp Tr_assignExp(Tr_exp left, Tr_exp right) {
	T_exp lval = unEx(left);
	T_exp exp = unEx(right);
	return Tr_Nx(T_Move(lval, exp));
}

Tr_exp Tr_ifExp(Tr_exp test, Tr_exp then, Tr_exp elsee) {
	struct Cx cond = unCx(test); 
	Temp_label t = Temp_newlabel(), f = Temp_newlabel();
	doPatch(cond.falses, f);
	doPatch(cond.trues, t);
	if (!elsee) {
		T_stm ts;
		if (then->kind == Tr_cx)
			ts = then->u.cx.stm;
		else
			ts = unNx(then);
		return Tr_Nx(T_Seq(cond.stm,
			T_Seq(T_Label(t),
				T_Seq(ts,
					T_Label(f)))));
	}
	else {
		Temp_label j = Temp_newlabel();
		T_stm tjump = T_Jump(T_Name(j), Temp_LabelList(j, NULL));
		//else和then均无结果（两者类型匹配），返回T_Seq
		if (then->kind == Tr_nx) {
			return Tr_Nx(T_Seq(cond.stm,
				T_Seq(T_Label(t),
					T_Seq(then->u.nx,
						T_Seq(tjump,
							T_Seq(T_Label(f),
								T_Seq(elsee->u.nx,
									T_Label(j))))))));
		}
		else {//else和then有结果的情况下：将else和then的值赋给临时变量，结果传给临时变量
			Temp_temp res = Temp_newtemp();
			T_exp tres = T_Temp(res);
			return Tr_Ex(T_Eseq(T_Seq(cond.stm,
				T_Seq(T_Label(t),
					T_Seq(T_Move(tres, unEx(then)),
						T_Seq(tjump,
							T_Seq(T_Label(f),
								T_Seq(T_Move(tres, unEx(elsee)),
									T_Label(j))))))), tres));
		}

		
	}
}

Tr_exp Tr_breakExp(Tr_exp breakk) {
	return Tr_Nx(T_Jump(T_Name(unEx(breakk)->u.NAME), Temp_LabelList(unEx(breakk)->u.NAME, NULL)));
}

Tr_exp Tr_doneLabel() {
	return Tr_Ex(T_Name(Temp_newlabel()));
}

Tr_exp Tr_whileExp(Tr_exp breakk, Tr_exp test, Tr_exp body) {
	struct Cx cond = unCx(test);
	Temp_label start = Temp_newlabel(), doo = Temp_newlabel(), done = unEx(breakk)->u.NAME;
	doPatch(cond.falses, done);
	doPatch(cond.trues, doo);
	return Tr_Nx(T_Seq(T_Label(start),
		T_Seq(cond.stm,
			T_Seq(T_Label(doo),
				T_Seq(unNx(body),
					T_Seq(T_Jump(T_Name(start), Temp_LabelList(start, NULL)),
						T_Label(done)))))));
}

Tr_exp Tr_varDec(Tr_exp lval, Tr_exp init) {
	return Tr_Nx(T_Move(unEx(lval), unEx(init)));
}

Tr_exp Tr_typeDec() {
	return Tr_Ex(T_Const(0));
}

Tr_exp Tr_callExp(Temp_label label, Tr_level fun, Tr_level call, Tr_expList l) {
	return Tr_Ex(T_Call(T_Name(label), T_ExpList(Tr_StaticLink(call, fun)->u.ex, Tr_expList_convert(l))));
}

//函数的转换先空着
Tr_exp Tr_funDec(Tr_expList bodylist) {
	T_stm stm;
	stm = T_Exp(T_Const(0));
	return Tr_Nx(stm);
}

Tr_expList Tr_ExpList(Tr_exp h, Tr_expList t) {
	Tr_expList trl = checked_malloc(sizeof(*trl));
	trl->head = h;
	trl->tail = t;
	return trl;
}

Tr_exp Tr_noExp() {
	return Tr_Ex(T_Const(0));
}

static Tr_exp Tr_StaticLink(Tr_level now, Tr_level def) {
	/* get call-function's static-link */
	T_exp addr = T_Temp(F_FP());/* frame-point */
	while (now && (now != def->parent)) { /* until find the level which def the function */
		F_access sl = F_formals(now->frame)->head;
		addr = F_Exp(sl, addr);
		now = now->parent;
	}
	return Tr_Ex(addr);
}

static T_expList Tr_expList_convert(Tr_expList l) {
	/*trans Tr_expList to T_expList*/
	T_expList h = NULL, t = NULL;
	for (; l; l = l->tail) {
		T_exp tmp = unEx(l->head);
		if (h) {
			t->tail = T_ExpList(tmp, NULL);
			t = t->tail;
		}
		else {
			h = T_ExpList(tmp, NULL);
			t = h;
		}
	}
	return h;
}

/*int main(){
    Tr_level l = Tr_newLevel(Tr_outermost(), Temp_newlabel(), U_BoolList(FALSE, U_BoolList(TRUE, NULL)));
    Tr_access a = Tr_allocLocal(l, TRUE);
    Tr_access a2 = Tr_allocLocal(l, FALSE);
    Tr_access a3 = Tr_allocLocal(l, FALSE);
    Tr_print_level(l);
    Tr_print_level(outermost);
    Tr_print_access(a);
    Tr_print_access(a2);
    Tr_print_access(a3);
    return 0;
}*/



