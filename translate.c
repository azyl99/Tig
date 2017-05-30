#include "translate.h"

static Tr_level outermost = NULL;
void Tr_print_access(Tr_access a);
void Tr_print_level(Tr_level l);

static patchList PatchList(Temp_label *head, patchList tail);
static doPatch(patchList tList, Temp_label label);
static joinPatch(patchList first, patchList second);
static Tr_exp Tr_Ex(T_exp ex);  
static Tr_exp Tr_Nx(T_stm ex);
static Tr_exp Tr_Cx(patchList trues, patchList falses, T_stm stm);
static T_exp unEx(Tr_exp e);
static T_stm unNx(Tr_exp e);
static struct Cx unCx(Tr_exp e);

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
    level->frame = F_newFrame(name, formals);
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
	patchList pal = check_malloc(sizeof(*pal));
	pal->head = head;
	pal->tail = tail;
	return pal;
}

static doPatch(patchList tList, Temp_label label) {
	for (; tList; tList = tList->tail) {
		*(tList->head) = label;
	}
}
static joinPatch(patchList first, patchList second) {
	if (!first) return second;
	for (; first->tail; first = first->tail);
	first->tail = second;
	return first; //???
}

static Tr_exp Tr_Ex(T_exp ex) {
	Tr_exp te = check_malloc(sizeof(*te));
	te->kind = Tr_ex;
	te->u.ex = ex;
	return te;
}

static Tr_exp Tr_Nx(T_stm ex) {
	Tr_exp te = check_malloc(sizeof(*te));
	te->kind = Tr_nx;
	te->u.nx = ex;
	return te;
}
static Tr_exp Tr_Cx(patchList trues, patchList falses, T_stm stm) {
	Tr_exp te = check_malloc(sizeof(*te));
	te->kind = Tr_cx;
	te->u.cx.trues = trues;
	te->u.cx.falses = falses;
	te->u.cx.stm = stm;
}

static T_exp unEx(Tr_exp e) {
	switch (e->kind) {
	case Tr_ex:
		return e->u.ex;
	case Tr_cx: {
		Temp_temp r = Temp_newtemp();
		Temp_label t = Temp_newlabel(), f = Temp_newlabel();
		doPatch(e->u.cx.trues, t);
		doPatch(e->u.cx.falses, f);
		return T_Eseq(T_Move(T_Temp(r), T_Const(1)),
			T_Eseq(e->u.cx.stm,
				T_Eseq(T_Label(f),
					T_Eseq(T_Move(T_Temp(r), T_Const(0)),
						T_Eseq(T_Label(t),
							T_Temp(r))))));
	}
	case Tr_nx:
		return T_Eseq(e->u.nx, T_Const(0));
	}
	assert(0);
}

static T_stm unNx(Tr_exp e) {
	switch (e->kind) {
	case Tr_ex:
		return T_Exp(e->u.ex);
	case Tr_cx: {
		return T_Exp(unEx(e));
	}
	case Tr_nx:
		return e->u.nx;
	}
	assert(0);
}
static struct Cx unCx(Tr_exp e) {
	switch (e->kind) {
	case Tr_ex: {
		T_stm te = T_Cjump(T_ne, T_Const(0), e->u.ex, NULL, NULL);
		patchList trues = PatchList(&te->u.CJUMP.true, NULL);
		patchList falses = PatchList(&te->u.CJUMP.false, NULL);
		struct Cx cx;
		cx.falses = falses;
		cx.trues = trues;
		cx.stm = te;
		return cx;
	}
	case Tr_cx:
		return e->u.cx;
	}
	assert(0);
}


Tr_exp Tr_intExp(int consti) {
	T_exp te = T_Const(consti);
	return Tr_Ex(te);
}

Tr_exp Tr_stringExp(string val) {
	Temp_label name = Temp_namedlabel(val);
	T_exp te = T_Name(name);
	return Tr_Ex(te);
}

Tr_exp Tr_simpleVar(Tr_access ta, Tr_level tl) {
	T_exp temp = T_Const(0);
	T_exp te = T_Mem(temp);
	return Tr_Ex(te);
}

Tr_exp Tr_fieldVar(Tr_access ta, Tr_level t1) {

}

Tr_exp Tr_subscriptVar(Tr_exp base, Tr_exp index) {
	return Tr_Ex(T_Mem(T_Binop(T_plus, base->u.ex, T_Binop(T_mul,index->u.ex,T_Const(FRAME_WORD_SIZE)))));
}

Tr_exp Tr_opExp(int ope, Tr_exp left, Tr_exp right) {
	T_exp te = T_Binop(ope, left->u.ex, right->u.ex);
	return Tr_Ex(te);
}

Tr_exp Tr_op2Exp(int ope, Tr_exp left, Tr_exp right) {  
	T_stm ts = T_Cjump(ope, left->u.ex, right->u.ex,NULL,NULL);
	patchList trues = PatchList(&ts->u.CJUMP.true,NULL);
	patchList falses = PatchList(&ts->u.CJUMP.false, NULL);
	Tr_exp te = Tr_Cx(trues, falses, ts);
	return te;
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



