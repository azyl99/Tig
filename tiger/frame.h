/* frame.h */
#ifndef _FRAME_H_
#define _FRAME_H_



#include "temp.h"
#include "tree.h"

const int MAX_REG;
const int FRAME_WORD_SIZE;

typedef struct F_frame_ *F_frame;
typedef struct F_access_ *F_access; //һ����ʽ������ֲ�����
typedef struct F_accessList_ *F_accessList;
struct F_accessList_ {F_access head; F_accessList tail;};

F_frame F_newFrame(Temp_label name, U_boolList formals);//Ϊ����name������ջ֡��formalsΪ��������bool�б�        
Temp_label F_name(F_frame f);       //����ĳ��ջ֡����ĺ����ĳ�ʼ��ַ���
F_accessList F_formals(F_frame f);  //����k�����ʵ��б�(callee�ӽ�),���view shift, ��Ŀ�������Լ���й�
F_access F_allocLocal(F_frame f, bool escape);  //��ջ֡Ϊһ����������λ�ã����������fp��Wλ��/�Ĵ�����

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
