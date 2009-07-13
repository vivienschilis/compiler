


%token CLASS IS METHODS RETURNS EXTENDS BEG END IF THEN ELSE NW AFF ID CSTE RELOP STRING SELF SUPER

%left ELSE
%left AFF
%left '+' '-'
%left '*' '/'

%type <S> ID
%type <class> CLEXP LCLEXP S
%type <attribut> LDEXP FULLDEXP DEXP LNDEXP LPARAM
%type <A> EXPOBJET EXPNOBJET OBJET CIBLE AFFECTATION BLOC LNEXP FULLEXP EXP COND SUPER SELF
%type <methode> LMTEXP FULLMETEXP METEXP
%type <f> APPELFCT
%type <argument> LARG
%type <C> REL


%{#include "main.h"
extern PCLASSE MakeClasse();
extern PATT MakeAttribut();
extern PARBRE MakeId(), MakeCste(), MakeSelf(), MakeSuper(), MakeNoeud(), MakeWhere(), MakeString(), MakeAppel();
extern PARBRE MakeBloc(), check_bloc();
extern PATT add_attribut();
extern PMETH MakeMethode();
extern PMETH add_methode();
extern PMETH check_methodes();
extern PATT check_attributs();
extern void Decompile();
extern PCLASSE get_definedClasses();
extern PFONC MakeFonction();
extern PARG MakeArgument();
extern PARG add_argument();

%}

%%

/* ******************** 
    Définition d'une classe 
    ******************** */
S : LCLEXP BLOC { $1;  mainBloc = check_bloc($2,nil(ATTRIBUT),nil(METHODE),NULL,NULL); 
					/*Decompile();*/
					$$ = get_definedClasses();
					current_method = NULL;
					current_class_name = NULL;
					parent_current_class_name = NULL;
					writecodeln("Main:NOP");
					writecode_vtables();
					current_method = NULL;
					writecode_exp($2);
				}
;

LCLEXP: CLEXP LCLEXP
	{
		$1;
		$2;
		$$ = get_definedClasses();	
	}
	| CLEXP
	{
		$$ = $1;
	}
;

CLEXP : CLASS ID IS LDEXP LMTEXP
		{
			$$ = MakeClasse($2, nil(char), check_attributs($4,$2,nil(char)), check_methodes($5,$2,nil(char),$4), nil(ARBRE));			
			writecode_class( $$ ); 
			
		}
		| CLASS ID EXTENDS ID IS LDEXP LMTEXP
		{
			$$ = MakeClasse($2,$4,check_attributs($6,$2,$4),check_methodes($7,$2,$4,$6),nil(ARBRE));
			writecode_class( $$ );
			
		}
;


/* ****************************** 
  Liste de déclaration d'expression 
  ****************************** */

LDEXP : FULLDEXP { $$ = $1;}
	| {$$ = nil(ATTRIBUT);}
;

FULLDEXP : DEXP ';' FULLDEXP { $$ = add_attribut($1,$3); }
	| DEXP { $$ = add_attribut($1,nil(ATTRIBUT));}
;
 
DEXP : ID  ID { $$ = MakeAttribut($1,$2);}
;

/* Liste non-vide d'expression */

LNDEXP : FULLDEXP {$$ = $1;}
;

/* ******************************* 
     Liste de déclaration de méthodes 
   ******************************* */

LMTEXP : METHODS FULLMETEXP {$$ = $2;}
	| {$$ = nil(METHODE);}
;

FULLMETEXP : METEXP ';' FULLMETEXP {$$ = add_methode($1,$3);}
	| METEXP {$$ =$1;}
;

METEXP : ID '(' LPARAM ')' RETURNS ID IS EXP
 		{$$ = MakeMethode($1,$3,$6,$8);}
;

LPARAM : DEXP {$$ = add_attribut($1,nil(ATTRIBUT)); }
	| DEXP ',' LPARAM { $$ = add_attribut($1,$3); }
	|{ $$ = nil(ATTRIBUT); }
;

/* ******************************* 
     Liste de déclaration d'expressions 
    ******************************* */

FULLEXP : EXP ';' FULLEXP { $$ = MakeNoeud(';', $1, $3); }
	| EXP {$$ = $1}
/*	| EXP {$$ = MakeNoeud(';',$1,nil(ARBRE));}*/
;

/* Liste non-vide de déclarations d'expressions */

LNEXP : FULLEXP {$$ = $1}
;

/* ************************
    Définition des expressions 
   ************************ */
 
 BLOC : '{' LNEXP '}' {$$ = MakeBloc($2, nil(ATTRIBUT));}	
	|  '{' LNDEXP BEG LNEXP END '}' { $$ = MakeBloc($4,$2);}
;
 
/* Définition d'une expression */
EXP : EXPNOBJET { $$ = $1 }
	| OBJET { $$ = $1 }
	| EXPOBJET { $$ = $1 }
;


EXPNOBJET : BLOC { $$ = $1; }
	| AFFECTATION { $$ = $1; }
	| IF COND THEN EXP ELSE EXP { $$ = MakeNoeud(ITE, $2, MakeNoeud(CONC, $4, $6)); }
	| '(' EXPNOBJET ')' { $$ = $2; }
;	

EXPOBJET : EXPOBJET '+' OBJET { $$ = MakeNoeud('+', $1, $3); }
	| EXPOBJET '-' OBJET { $$ = MakeNoeud('-', $1, $3); }
	| EXPOBJET '*' OBJET { $$ = MakeNoeud('*', $1, $3); }
	| EXPOBJET '/' OBJET { $$ = MakeNoeud('/', $1, $3); }
	| OBJET '+' OBJET { $$ = MakeNoeud('+', $1, $3); }
	| OBJET '-' OBJET { $$ = MakeNoeud('-', $1, $3); }
	| OBJET '*' OBJET { $$ = MakeNoeud('*', $1, $3); }
	| OBJET '/' OBJET { $$ = MakeNoeud('/', $1, $3); }
;	

OBJET : ID { $$ = MakeId($1);}
	| NW ID { $$ = MakeNoeud(New, MakeId($2));}
	| CSTE { $$ = MakeCste(yylval.E); }
	| STRING { $$ = MakeString(yylval.S);}
	| '(' EXPOBJET ')' { $$ = $2; }
	| APPELFCT	{$$ = MakeAppel($1); }
	| OBJET '.' ID { $$ = MakeNoeud('.',$1, MakeId($3));; }
	| OBJET '.' APPELFCT { $$ = MakeNoeud('.',$1, MakeAppel($3));}
	| '(' OBJET ')' {$$ = $2}
	| SELF { $$ = MakeNoeud(Self,nil(ARBRE),nil(ARBRE)); }
	| SUPER '.' APPELFCT { $$ = MakeNoeud('.', MakeNoeud(Super,nil(ARBRE),nil(ARBRE)), MakeAppel($3)); }
;

/* Condition */
COND : EXP REL EXP { $$ = MakeNoeud($2,$1,$3); }
      | '(' COND ')' { $$ = $2; }
;

REL : RELOP { $$ = yylval.C;}

AFFECTATION : CIBLE AFF EXP {$$ = MakeNoeud(Aff, $1, $3); }
;

/* Variable ou champ d'un objet */
CIBLE : ID { $$ = MakeId($1); }
	| OBJET '.' ID { $$ = MakeNoeud('.',$1, MakeId($3)); }
;

/*OBJET : ID
	| APPELFCT
	| OBJET '.' ID
	| OBJET '.' APPELFCT
	| STRING
	| '(' NW OBJET ')'
	| CSTE
	| '(' OBJET ')'
;*/

/* Appel d'une méthode d'un objet */
/*APPELMET: OBJET '.' APPELFCT
	| APPELFCT
;*/

APPELFCT: ID '(' LARG ')' { $$ = MakeFonction ($1, $3); } 
;

/* Liste d'arguments lors de l'appel d'une méthode */
LARG : EXP {$$ = add_argument(MakeArgument($1),nil(PARG)); }
	| EXP ',' LARG {$$ = add_argument(MakeArgument($1), $3); }
	| {$$ = nil(ARGUMENT);}
;
%%
