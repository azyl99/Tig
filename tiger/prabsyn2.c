/*
 * prabsyn.c - Print Abstract Syntax data structures. Most functions 
 *           handle an instance of an abstract syntax rule.
 */

#include <stdio.h>
#include "util.h"
#include "symbol.h" /* symbol table data structures */
#include "absyn.h"  /* abstract syntax data structures */
#include "prabsyn.h" /* function prototype */

/* local function prototypes */
static void pr_var(FILE *out, A_var v, int d);
static void pr_dec(FILE *out, A_dec v, int d);
static void pr_ty(FILE *out, A_ty v, int d);
static void pr_field(FILE *out, A_field v, int d);
static void pr_fieldList(FILE *out, A_fieldList v, int d);
static void pr_expList(FILE *out, A_expList v, int d);
static void pr_fundec(FILE *out, A_fundec v, int d);
static void pr_fundecList(FILE *out, A_fundecList v, int d);
static void pr_decList(FILE *out, A_decList v, int d);
static void pr_namety(FILE *out, A_namety v, int d);
static void pr_nametyList(FILE *out, A_nametyList v, int d);
static void pr_efield(FILE *out, A_efield v, int d);
static void pr_efieldList(FILE *out, A_efieldList v, int d);

static void indent(FILE *out, int d) {
 int i;
 for (i = 0; i <= d; i++) fprintf(out, " ");
}

/* Print A_var types. Indent d spaces. */
static void pr_var(FILE *out, A_var v, int d) {
 indent(out, d);
 switch (v->kind) {
 case A_simpleVar:
   fprintf(out, "[simpleVar [%s]]", S_name(v->u.simple)); 
   break;
 case A_fieldVar:
   fprintf(out, "%s", "[fieldVar ");
   pr_var(out, v->u.field.var, 0);
   indent(out, 0); fprintf(out, "[%s]]", S_name(v->u.field.sym));
   break;
 case A_subscriptVar:
   fprintf(out, "%s", "[subscriptVar ");
   pr_var(out, v->u.subscript.var, 0);
   pr_exp(out, v->u.subscript.exp, 0); fprintf(out, "%s", "]");
   break;
 default:
   assert(0); 
 } 
}

static char str_oper[][12] = {
   "PLUS", "MINUS", "TIMES", "DIVIDE", "AND","OR",
   "EQUAL", "NOTEQUAL", "LESSTHAN", "LESSEQ", "GREATTHAN", "GREATEQ"};
 
static void pr_oper(FILE *out, A_oper d) {
  fprintf(out, "[%s]", str_oper[d]);
}

/* Print A_var types. Indent d spaces. */
void pr_exp(FILE *out, A_exp v, int d) {
 indent(out, d);
 switch (v->kind) {
 case A_varExp:
   fprintf(out, "[varExp");
   pr_var(out, v->u.var, 0); 
   fprintf(out, "%s", "]");
   break;
 case A_nilExp:
   fprintf(out, "[nilExp]");
   break;
 case A_voidExp:
   fprintf(out, "[voidExp]");
   break;
 case A_intExp:
   fprintf(out, "[intExp %d]", v->u.intt);
   break;
 case A_stringExp:
   fprintf(out, "[stringExp [%s]]", v->u.stringg);
   break;
 case A_callExp:
   fprintf(out, "[callExp [%s]", S_name(v->u.call.func));
   pr_expList(out, v->u.call.args, 0); fprintf(out, "]");
   break;
 case A_opExp:
   fprintf(out, "[opExp ");
   pr_oper(out, v->u.op.oper);
   pr_exp(out, v->u.op.left, 0);
   pr_exp(out, v->u.op.right, 0); fprintf(out, "]");
   break;
 case A_recordExp:
   fprintf(out, "[recordExp [%s]", S_name(v->u.record.typ)); 
   pr_efieldList(out, v->u.record.fields, 0); fprintf(out, "]");
   break;
 case A_seqExp:
   fprintf(out, "[seqExp"); 
   pr_expList(out, v->u.seq, 0); fprintf(out, "]");
   break;
 case A_assignExp:
   fprintf(out, "[assignExp");
   pr_exp(out, v->u.assign.exp, 0); fprintf(out, "]");
   break;
 case A_ifExp:
   fprintf(out, "[iffExp");
   pr_exp(out, v->u.iff.test, 0);
   pr_exp(out, v->u.iff.then, 0);
   if (v->u.iff.elsee) { /* else is optional */
      pr_exp(out, v->u.iff.elsee, 0);
   }
   fprintf(out, "]");
   break;
 case A_whileExp:
   fprintf(out, "[whileExp");
   pr_exp(out, v->u.whilee.test, 0);
   pr_exp(out, v->u.whilee.body, 0); fprintf(out, "]");
   break;
 case A_forExp:
   fprintf(out, "[forExp [%s]", S_name(v->u.forr.var)); 
   pr_exp(out, v->u.forr.lo, 0);
   pr_exp(out, v->u.forr.hi, 0);
   pr_exp(out, v->u.forr.body, 0);
   indent(out, 0); fprintf(out, "[%s]", v->u.forr.escape ? "TRUE]" : "FALSE]");
   break;
 case A_breakExp:
   fprintf(out, "[breakExp]");
   break;
 case A_letExp:
   fprintf(out, "[letExp");
   pr_decList(out, v->u.let.decs, 0);
   pr_exp(out, v->u.let.body, 0); fprintf(out, "]");
   break;
 case A_arrayExp:
   fprintf(out, "[arrayExp [%s]", S_name(v->u.array.typ));
   pr_exp(out, v->u.array.size, 0);
   pr_exp(out, v->u.array.init, 0); fprintf(out, "]");
   break;
 default:
   assert(0); 
 } 
}

static void pr_dec(FILE *out, A_dec v, int d) {
 indent(out, d);
 switch (v->kind) {
 case A_functionDec:
   fprintf(out, "[functionDec"); 
   pr_fundecList(out, v->u.function, 0); fprintf(out, "]");
   break;
 case A_varDec:
   fprintf(out, "[varDec [%s]", S_name(v->u.var.var));
   if (v->u.var.typ) {
     fprintf(out, " [%s]", S_name(v->u.var.typ)); 
   }
   pr_exp(out, v->u.var.init, 0);
   fprintf(out, " [%s]", v->u.var.escape ? "TRUE]" : "FALSE]");
   break;
 case A_typeDec:
   fprintf(out, "[typeDec");
   pr_nametyList(out, v->u.type, 0); fprintf(out, "]");
   break;
 default:
   assert(0); 
 } 
}

static void pr_ty(FILE *out, A_ty v, int d) {
 indent(out, d);
 switch (v->kind) {
 case A_nameTy:
   fprintf(out, "[nameTy [%s]]", S_name(v->u.name));
   break;
 case A_recordTy:
   fprintf(out, "[recordTy");
   pr_fieldList(out, v->u.record, 0); fprintf(out, "]");
   break;
 case A_arrayTy:
   fprintf(out, "[arrayTy [%s]]", S_name(v->u.array));
   break;
 default:
   assert(0); 
 } 
}

static void pr_field(FILE *out, A_field v, int d) {
 indent(out, d);
 fprintf(out, "[field [%s]", S_name(v->name));
 fprintf(out, "[%s] ", S_name(v->typ));
 fprintf(out, "[%s]", v->escape ? "TRUE]" : "FALSE]");
}

static void pr_fieldList(FILE *out, A_fieldList v, int d) {
 indent(out, d);
 if (v) {
   fprintf(out, "[fieldList");
   pr_field(out, v->head, 0);
   pr_fieldList(out, v->tail, 0); fprintf(out, "]");
 }
 else fprintf(out, "[fieldList [NULL]]");
}

static void pr_expList(FILE *out, A_expList v, int d) {
 indent(out, d);
 if (v) {
   fprintf(out, "[expList");
   pr_exp(out, v->head, 0);
   pr_expList(out, v->tail, 0);
   fprintf(out, "]");
 }
 else fprintf(out, "[expList [NULL]]"); 

}

static void pr_fundec(FILE *out, A_fundec v, int d) {
 indent(out, d);
 fprintf(out, "[fundec [%s]", S_name(v->name));
 pr_fieldList(out, v->params, 0);
 if (v->result) {
   indent(out, 0); fprintf(out, "[%s]", S_name(v->result));
 }
 pr_exp(out, v->body, 0); fprintf(out, "]");
}

static void pr_fundecList(FILE *out, A_fundecList v, int d) {
 indent(out, d);
 if (v) {
   fprintf(out, "[fundecList");
   pr_fundec(out, v->head, 0);
   pr_fundecList(out, v->tail, 0); fprintf(out, "]");
 }
 else fprintf(out, "[fundecList [NULL]]");
}

static void pr_decList(FILE *out, A_decList v, int d) {
 indent(out, d);
 if (v) {
   fprintf(out, "[decList");
   pr_dec(out, v->head, 0);
   pr_decList(out, v->tail, 0);
   fprintf(out, "]");
 }
 else fprintf(out, "[decList [NULL]]"); 

}

static void pr_namety(FILE *out, A_namety v, int d) {
 indent(out, d);
 fprintf(out, "[namety [%s]", S_name(v->name)); 
 pr_ty(out, v->ty, 0); fprintf(out, "]");
}

static void pr_nametyList(FILE *out, A_nametyList v, int d) {
 indent(out, d);
 if (v) {
   fprintf(out, "[nametyList");
   pr_namety(out, v->head, 0);
   pr_nametyList(out, v->tail, 0); fprintf(out, "]");
 }
 else fprintf(out, "[nametyList [NULL]]");
}

static void pr_efield(FILE *out, A_efield v, int d) {
 indent(out, d);
 if (v) {
   fprintf(out, "[efield [%s]]", S_name(v->name));
 }
 else fprintf(out, "[efield]");
}

static void pr_efieldList(FILE *out, A_efieldList v, int d) {
 indent(out, d);
 if (v) {
   fprintf(out, "[efieldList");
   pr_efield(out, v->head, 0);
   pr_efieldList(out, v->tail, 0); fprintf(out, "]");
 }
 else fprintf(out, "[efieldList [NULL]]");
}
