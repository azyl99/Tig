#include <stdio.h>
#include "frame.h"

const int MAX_REG = 6;
const int FRAME_WORD_SIZE = 4;
void F_print_access(F_access a);
void F_print_frame(F_frame f);

struct F_access_
{
    enum {inFrame, inReg} kind;
    union {
        int         offset; /* InFrame, at offset X from the fp */
        Temp_temp   reg;    /* InReg, the register name */
    } u;
};

struct F_frame_
{
    Temp_label      name;               /* the label at which the function¡¯s machine code is to begin */
    F_accessList    formals;            /* the locations of all the formals */
    int             frame_size;         /* the number of locals allocated so far */
    //ÔÝÎª¿Õ                            /* instructions required to implement the ¡°view shift¡± */
    
};

F_accessList F_AccessList(F_access head, F_accessList tail){
    F_accessList f = checked_malloc(sizeof(*f));
    f->head = head;
    f->tail = tail;
}

static F_access InFrame(int offset){
    F_access access = checked_malloc(sizeof(*access));
    access->kind = inFrame;
    access->u.offset = offset;
    return access;
    
}

static F_access InReg(Temp_temp reg){
    F_access access = checked_malloc(sizeof(*access));
    access->kind = inReg;
    access->u.reg = reg;
    return access;
}

F_frame F_newFrame(Temp_label name, U_boolList formals){
    F_frame frame = checked_malloc(sizeof(*frame));
    frame->name = name;
    frame->frame_size = 0;
    F_accessList head = NULL, tail = NULL;
    F_access access = NULL;
    int i;
    for(i = 0;formals;formals = formals->tail,i++){
        if(i < MAX_REG && !formals->head){
            access = InReg(Temp_newtemp());
        }
        else{   //solve with static link
            access = InFrame(i * FRAME_WORD_SIZE);
        }
        if(head){
            tail->tail = F_AccessList(access, NULL);
            tail = tail->tail;
        }
        else{
            head = F_AccessList(access, NULL);
            tail = head;
        }        
    }
    frame->formals = head;
    return frame;
}
       
Temp_label F_name(F_frame f){
    return f->name;
}

F_accessList F_formals(F_frame f){
    return f->formals;
}

F_access F_allocLocal(F_frame f, bool escape){
    f->frame_size++;
    if(escape)  //solve with static link
        return InFrame(-1 * f->frame_size * FRAME_WORD_SIZE);
    else 
        return InReg(Temp_newtemp());
}

void F_print_access(F_access a){
    switch(a->kind){
        case inReg: printf("\tvariable_reg:%d\n",Temp_tempint(a->u.reg));break;
        case inFrame: printf("\tvariable_offset:%d\n",a->u.offset);break;
        default: printf("variable not in reg or frame\n");break;
    }
}

void F_print_frame(F_frame f){
    printf("frame_name:%s\n",Temp_labelstring(f->name));
    printf("frame_size:%d\n",f->frame_size);
    printf("frame_formals:\n");
    F_accessList p = NULL;
    for(p = f->formals;p;p = p->tail){
        F_print_access(p->head);
    }

}

/*int main(){
    F_frame f = F_newFrame(Temp_namedlabel("f"), U_BoolList(FALSE, U_BoolList(TRUE, NULL)));
    F_access a = F_allocLocal(f, TRUE);
    F_access a2 = F_allocLocal(f, FALSE);
    F_access a3 = F_allocLocal(f, FALSE);
    F_print_frame(f);
    F_print_access(a);
    F_print_access(a2);
    F_print_access(a3);
    return 0;
}*/




