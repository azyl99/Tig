%{
#include <string.h>
#include "util.h"
#include "errormsg.h"
#include "table.h" 
#include "symbol.h" 
#include "absyn.h"
#include "y.tab.h"//解决方法：把y.tab.h依赖的几个.h文件放在前面

int charPos=1;

int yywrap(void)/*yywrap() 这一函数在文件(或输入)的末尾调用。如果函数的返回值是1,就停止解析*/
{
 charPos=1;
 return 1;
}


void adjust(void)
{
 EM_tokPos=charPos;/* charPos是当前字符在整个文件中的位置 */
 charPos+=yyleng;
}

void adjust2(void)
{
 charPos+=yyleng;	
}

// for string
#define MAX_STR_LENGTH 1024
char string_buf[MAX_STR_LENGTH];
char *string_buf_ptr;

%}

%x comment comment2 string

%%

"/*"					{adjust(); BEGIN(comment);}
<comment>"*/"			{adjust(); BEGIN(INITIAL);}
<comment>\n				{adjust(); EM_newline();}
<comment>.				{adjust();}

"//"					{adjust(); BEGIN(comment2);}
<comment2>"\n"			{adjust(); BEGIN(INITIAL); EM_newline();}
<comment2>.				{adjust();}

"\""      				{adjust(); BEGIN(string); string_buf_ptr = string_buf;}
<string>"\""			{adjust2(); BEGIN(INITIAL); *string_buf_ptr = '\0'; yylval.sval = String(string_buf); /*创建一个自己的字符串*/ return STRING;}
<string>"\n"			{adjust2(); EM_error(EM_tokPos, "unterminated string constant");}
<string>\\[0-9]{1,3}	{adjust2();
        int result;
        (void) sscanf( yytext + 1, "%d", &result );
        if ( result > 0xff )
            EM_error(EM_tokPos, "constant is out-of-bounds");
        *string_buf_ptr++ = result;
        }
<string>"\\n"			{adjust2(); *string_buf_ptr++ = '\n';}
<string>"\\t"			{adjust2(); *string_buf_ptr++ = '\t';}
<string>"\\r"			{adjust2(); *string_buf_ptr++ = '\r';}
<string>"\\b"			{adjust2(); *string_buf_ptr++ = '\b';}
<string>"\\f"			{adjust2(); *string_buf_ptr++ = '\f';}
<string>"\\\""			{adjust2(); *string_buf_ptr++ = '\"';}
<string>"\\(.|\n)"  	{adjust2(); *string_buf_ptr++ = yytext[1];/* 转义无意义，忽略这个反斜杠'\' */}
<string>[^\\\n\"]+		{adjust2(); /* common case */
		strcpy(string_buf_ptr, yytext);
		string_buf_ptr += yyleng;
		}

\n	 		{adjust(); EM_newline(); /* 顺序很重要！匹配不到会有warning */}//continue;
[[:space:]]	{adjust();}

","			{adjust(); return COMMA;}
":"			{adjust(); return COLON;}
";"			{adjust(); return SEMICOLON;}
"("			{adjust(); return LPAREN;}
")"			{adjust(); return RPAREN;}
"["			{adjust(); return LBRACK;}
"]"			{adjust(); return RBRACK;}
"{"			{adjust(); return LBRACE;}
"}"			{adjust(); return RBRACE;}
"."			{adjust(); return DOT;}
"+"			{adjust(); return PLUS;}
"-"			{adjust(); return MINUS;}
"*"			{adjust(); return TIMES;}
"/"			{adjust(); return DIVIDE;}
"="			{adjust(); return EQ;}
"<>"		{adjust(); return NEQ;}
"<"			{adjust(); return LT;}
"<="		{adjust(); return LE;/* 最长匹配，可以放在下面 */}
">"			{adjust(); return GT;}
">="		{adjust(); return GE;}
"&"			{adjust(); return AND;}
"|"			{adjust(); return OR;}
":="		{adjust(); return ASSIGN;}

array		{adjust(); return ARRAY;}
if			{adjust(); return IF;}
then		{adjust(); return THEN;}
else		{adjust(); return ELSE;}
while		{adjust(); return WHILE;}
for			{adjust(); return FOR;}
to          {adjust(); return TO;}
do    		{adjust(); return DO;}
let         {adjust(); return LET;}
in			{adjust(); return IN;}
end			{adjust(); return END;}
of          {adjust(); return OF;}
break		{adjust(); return BREAK;}
nil			{adjust(); return NIL;}
function	{adjust(); return FUNCTION;}
var			{adjust(); return VAR;}
type		{adjust(); return TYPE;}


[0-9]+	 					{adjust(); yylval.ival=atoi(yytext); return INT;}
[_[:alpha:]][_[:alnum:]]*	{adjust(); yylval.sval=String(yytext); return ID;/*yytext不能直接赋值，否则有可能消失*/}
.							{adjust(); EM_error(EM_tokPos,"illegal token");}
