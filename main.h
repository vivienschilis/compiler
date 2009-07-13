#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#define false 0
#define true 1

/* deux macros pratiques, utilisées dans les allocations */

#define NEW(howmany, type) (type *) calloc((unsigned) howmany, sizeof(type))
#define nil(type) (type *) 0

#define BUFSIZE (20000)
#define MAXSIZE (2 * BUFSIZE)


/* Etiquettes pour les noeuds de l'arbre syntaxique (ou plus exactement
 * d'une structure equivalente pour les expressions
 */
typedef enum { Id, Cste, Aff, CONC, EQ, NEQ, GT, GE, LT, LE, ITE,
	       Or, And, Not, Put, Wh, String, New, Fct, Bloc, Relop, Self, Super } etiquette;
	
	
	
/* la structure d'un noeud de l'arbre */
typedef union {
        char *S;	  /* feuille : une variable */
        int E;	 	  /* feuille: une constante entiere */
		struct fonction *F;
        struct arbre *A;  /* noeud interne : un operateur et deux operandes */
		struct attribut *lattributs;
		
} NOEUD;


/*Structure d'un appel de methode*/
typedef struct fonction{
	char * name;//nom de la methode
	struct argument *largs;//list de paramètres.
	struct fonction * suiv;
} FONCTION, *PFONC;

/* la structure d'un noeud interne ... */
typedef struct arbre
{ char op;		/* une etiquette: voir l'enumeration ci-dessus */
  NOEUD gauche, droit;  /* deux noeuds: internes ou feuilles */
} ARBRE, *PARBRE;


	
/*Structure d'un attribut*/
typedef struct attribut{
	char * name;
	char * type;
	struct attribut *suiv; 
} ATTRIBUT, *PATT;

/*Structure d'une methode*/
typedef struct methode{
	char * name;//nom de la methode
	char * type;//type de retour
	struct attribut *lparams;//list de paramètres.
	struct attribut *lparams2;//list de paramètres.
	struct methode *suiv; 
	struct arbre *expression;//corps de la méthode
	int label;
} METHODE, *PMETH;

/*Structure d'une classe*/

typedef struct classe{
	char *name;//nom de la classe
	char *name_parent;//nom de la classe parente
	struct attribut *lattributs;//liste des attributs de la classe
	struct methode *lmethodes;//liste des méthodes de la classe
	struct classe * suiv;
	int index;
} CLASSE, *PCLASSE;


typedef struct expression {
	struct arbre *e;
	struct expression *suiv;
} EXPR, *PEXPR;

typedef struct bloc {
	struct attribut *lattributs;
	struct expression *lexpression;
} BLOC, *PBLOC;

/*Structure d'un attribut*/
typedef struct argument{
	struct arbre *expression;
	struct argument *suiv; 
} ARGUMENT, *PARG;


typedef union
{
  char C;
  PCLASSE class;
  PATT attribut;
  PMETH methode;
  PARG argument;
  PBLOC bloc;
  PEXPR expression;
  PARBRE A;
  PFONC f;
  int E;
  char *S;
} YYSTYPE;


PCLASSE definedClasses;//liste des classes définies dans le programme
PARBRE mainBloc;//bloc principal du programme
PATT AttributsEnvironnement;//environnement des attributs
PMETH MethodesEnvironnement;//environnement des méthodes
char * current_class_name;//nom de la classe en cours de traitement
char * parent_current_class_name;//nom de la super classe de la classe en cours de traitement
char * current_methode;//nom de la méthode en cours de traitement
char * current_methode_return_type;//type de retour de la méthode en cours de traitement

FILE* fcode;

int lbli;

int classi;

PMETH current_method;

#define YYSTYPE YYSTYPE
#define YYERROR_VERBOSE 1
