/* frame.h */
#include "temp.h"
typedef struct F_frame_ *F_frame;
typedef struct F_access_ *F_access; //һ����ʽ������ֲ�����
typedef struct F_accessList_ *F_accessList;
struct F_accessList_ {F_access head; F_accessList tail;};

F_frame F_newFrame(Temp_label name, U_boolList formals);//Ϊ����name������ջ֡��formalsΪ��������bool�б�        
Temp_label F_name(F_frame f);       //����ĳ��ջ֡����ĺ����ĳ�ʼ��ַ���
F_accessList F_formals(F_frame f);  //����k�����ʵ��б�(callee�ӽ�),���view shift, ��Ŀ�������Լ���й�
F_access F_allocLocal(F_frame f, bool escape);  //��ջ֡Ϊһ����������λ�ã����������fp��Wλ��/�Ĵ�����

void F_print_access(F_access a);
void F_print_frame(F_frame f);
