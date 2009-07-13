OBJ=Projet.o lex.o main.o
CC=gcc
CFLAGS=-c -I./ -g 
LDFLAGS= -g -lfl
Projet : $(OBJ)
	$(CC) -o Projet $(OBJ) $(LDFLAGS)

Projet.o : Projet.y 
	bison -v -b Projet -o Projet.c -d Projet.y
	$(CC) $(CFLAGS) -DYYDEBUG -c Projet.c

Projet.h: Projet.y main.h
	bison -b Projet -o Projet.c -d Projet.y

lex.o : Projet.l Projet.h main.h
	flex -olex.c Projet.l
	$(CC) $(CFLAGS) -c lex.c

testelex: lex.o teste-lex.o 
	$(CC) -o testelex lex.o teste-lex.o $(LDFLAGS)	


clean:	
	rm -f *.o lex.c Projet.c Projet.out Projet.h teste-lex.o

veryclean:
	rm -f *.o lex.c Projet.c Projet.out Projet.h teste-lex.o testelex Projet
