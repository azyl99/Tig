a.exe: test.o y.tab.o lex.yy.o errormsg.o util.o table.o symbol.o absyn.o prabsyn1.o parse.o \
types.o env.o semant.o frame.o translate.o temp.o printtree1.o tree.o
	cc -g test.o y.tab.o lex.yy.o errormsg.o util.o table.o symbol.o absyn.o prabsyn1.o parse.o \
types.o env.o semant.o frame.o translate.o temp.o printtree1.o tree.o

printtree1.o: printtree1.c printtree.h
	cc -g -c printtree1.c

# printtree.o: printtree.c printtree.h
	# cc -g -c printtree.c

tree.o: tree.c tree.h temp.h
	cc -g -c tree.c
	
temp.o: temp.c temp.h
	cc -g -c temp.c

translate.o: translate.c translate.h frame.h tree.h absyn.h util.h
	cc -g -c translate.c

frame.o: mipsframe.c frame.h tree.h
	cc -g -c -o frame.o mipsframe.c

test.o: test.c errormsg.h util.h parse.h prabsyn.h
	cc -g -c test.c

semant.o:semant.c util.h errormsg.h symbol.h absyn.h types.h translate.h semant.h
	cc -g -c semant.c

env.o: env.c util.h symbol.h types.h env.h
	cc -g -c env.c

types.o: types.c util.h symbol.h types.h
	cc -g -c types.c
	
prabsyn1.o: prabsyn1.c util.h symbol.h absyn.h
	cc -g -c prabsyn1.c
	
# prabsyn.o: prabsyn1.c util.h symbol.h absyn.h
	# cc -g -c prabsyn1.c
	
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

lex.yy.c: tiger.lex
	flex tiger.lex

util.o: util.c util.h
	cc -g -c util.c

clean: 
	rm -f a.exe *.o y.* lex.yy.c
