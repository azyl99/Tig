#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "errormsg.h"
#include "table.h"
#include "symbol.h"
#include "absyn.h"
#include "parse.h"
#include "prabsyn.h"
//
#include "types.h"
#include "env.h"
#include "translate.h"
#include "semant.h"

extern int yyparse(void);

int main(int argc, char **argv) {
 if (argc!=2) {fprintf(stderr,"usage: a.out filename\n"); exit(1);}
 A_exp a_exp= parse(argv[1]);
 // pr_exp(stdout, a_exp, 0);
 
 S_table base_tenv = E_base_tenv();
 S_table base_venv = E_base_venv();
 transExp(base_venv, base_tenv, a_exp);
 
 return 0;
}
