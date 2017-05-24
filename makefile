a.exe: test.o y.tab.o lex.yy.o errormsg.o util.o table.o symbol.o absyn.o prabsyn.o parse.o types.o env.o semant.o
	cc -g test.o y.tab.o lex.yy.o errormsg.o util.o table.o symbol.o absyn.o prabsyn.o parse.o types.o env.o semant.o

test.o: test.c errormsg.h util.h parse.h prabsyn.h
	cc -g -c test.c

semant.o:semant.c util.h errormsg.h symbol.h absyn.h types.h translate.h semant.h
	cc -g -c semant.c

env.o: env.c util.h symbol.h types.h env.h
	cc -g -c env.c

types.o: types.c util.h symbol.h types.h
	cc -g -c types.c
	
prabsyn.o: prabsyn.c util.h symbol.h absyn.h
	cc -g -c prabsyn.c
	
parse.o: parse.c errormsg.h util.h symbol.h absyn.h
	cc -g -c parse.c	
	
absyn.o: absyn.c util.h symbol.h 
	cc -g -c absyn.c	
	
symbol.o: symbol.c util.h table.h
	cc -g -c symbol.c
	
table.o: table.c util.h
	cc -g -c table.c
	
y.tab.o: y.tab.c
	cc -g -c y.tab.c

y.tab.c: tiger.grm
	bison --yacc -dv tiger.grm

y.tab.h: y.tab.c
	echo "y.tab.h was created at the same time as y.tab.c"

errormsg.o: errormsg.c errormsg.h util.h
	cc -g -c errormsg.c

lex.yy.o: lex.yy.c y.tab.h errormsg.h util.h
	cc -g -c lex.yy.c

lex.yy.c: tiger.lex y.tab.h
	lex tiger.lex

util.o: util.c util.h
	cc -g -c util.c

clean: 
	rm -f a.exe *.o y.* lex.yy.c
