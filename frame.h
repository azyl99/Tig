/* frame.h */
#ifndef _FRAME_H_
#define _FRAME_H_



#include "temp.h"
#include "tree.h"

const int MAX_REG;
const int FRAME_WORD_SIZE;

typedef struct F_frame_ *F_frame;
typedef struct F_access_ *F_access; //一个形式参数或局部变量
typedef struct F_accessList_ *F_accessList;
struct F_accessList_ {F_access head; F_accessList tail;};

F_frame F_newFrame(Temp_label name, U_boolList formals);//为函数name创建新栈帧，formals为参数逃逸bool列表        
Temp_label F_name(F_frame f);       //返回某个栈帧代表的函数的初始地址标号
F_accessList F_formals(F_frame f);  //返回k个访问的列表(callee视角),解决view shift, 跟目标机调用约定有关
F_access F_allocLocal(F_frame f, bool escape);  //在栈帧为一个变量分配位置，返回相对于fp的W位移/寄存器名

void F_print_access(F_access a, FILE *out);
void F_print_frame(F_frame f,FILE *out);

/****IR*****/
typedef struct F_frag_ *F_frag;

struct F_frag_ {
	enum{F_stringFrag,F_procFrag} kind;
	union {
		struct {
			Temp_label label;
			string str;
		} stringg;
		struct {
			T_stm body;
			F_frame frame;
		} proc;
	} u;
};
typedef struct F_fragList_ *F_fragList;
struct F_fragList_ {
	F_frag head;
	F_fragList tail;
};

F_frag F_StringFrag(Temp_label label, string str);
F_frag F_ProcFrag(T_stm body, F_frame frame);
F_fragList F_FragList(F_frag head, F_fragList tail);

Temp_temp F_FP(void);
Temp_temp F_RV(void);
extern const int F_wordSize;
T_exp F_Exp(F_access acc, T_exp framePtr);
T_exp F_externalCall(string s, T_expList args);

#endif // !_FRAME_H_
