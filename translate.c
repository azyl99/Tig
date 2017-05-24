#include "translate.h"

static Tr_level outermost = NULL;
void Tr_print_access(Tr_access a);
void Tr_print_level(Tr_level l);

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



