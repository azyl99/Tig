/* frame.h */
#include "temp.h"
typedef struct F_frame_ *F_frame;
typedef struct F_access_ *F_access; //一个形式参数或局部变量
typedef struct F_accessList_ *F_accessList;
struct F_accessList_ {F_access head; F_accessList tail;};

F_frame F_newFrame(Temp_label name, U_boolList formals);//为函数name创建新栈帧，formals为参数逃逸bool列表        
Temp_label F_name(F_frame f);       //返回某个栈帧代表的函数的初始地址标号
F_accessList F_formals(F_frame f);  //返回k个访问的列表(callee视角),解决view shift, 跟目标机调用约定有关
F_access F_allocLocal(F_frame f, bool escape);  //在栈帧为一个变量分配位置，返回相对于fp的W位移/寄存器名

void F_print_access(F_access a);
void F_print_frame(F_frame f);
