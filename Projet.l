Chiffre [0-9]
Entier {Chiffre}+
Lettre	[a-zA-Z]
LC 	({Lettre}|{Chiffre})
Delim	[ \n\t\r]
Symbole [+\-*/(){};,."]
Char	[^"\\]
Guill		"\""
Star  		"*"
NotStar		[^*]
NotStarPar      [^*/]
InComment       ({NotStar}|({Star}+{NotStarPar}+))*{Star}*
Comment 	"/*"{InComment}"*/"
BS      \\.
PasBSG [^\\"]
IntString ({BS}|{PasBSG})*

%{
#include <string.h>
#include "main.h"
#include "Projet.h"

extern YYSTYPE yylval, yyval;

int TesteMotClef(char *s) {
  if (! strcasecmp(s, "begin")) return(BEG);
  if (! strcasecmp(s, "end")) return(END);
  if (! strcasecmp(s, "if")) return(IF);
  if (! strcasecmp(s, "then")) return(THEN);
  if (! strcasecmp(s, "else")) return(ELSE);
  if (! strcasecmp(s, "class")) return(CLASS);
  if (! strcasecmp(s, "is")) return(IS);
  if (! strcasecmp(s, "methods")) return(METHODS);
  if (! strcasecmp(s, "returns")) return(RETURNS);
  if (! strcasecmp(s, "extends")) return(EXTENDS);
  if (! strcasecmp(s, "new")) return(NW);
  if (! strcasecmp(s, "self")) return(SELF);
  if (! strcasecmp(s, "super")) return(SUPER);


  yylval.S = strdup(yytext);
  return(ID);
}

%}

%%
\/\*([^*]|(\*+([^*/])))*\*+\/        /* ignore comments */; 
{Entier} 	{ yylval.E = atoi(yytext); return(CSTE); }
{Lettre}{LC}*	{ return TesteMotClef(yytext); }
{Symbole}	{ return(yytext[0]); }
:=		{ return(AFF); }
=		{ yylval.C = EQ; return(RELOP); }
\<>		{ yylval.C = NEQ; return(RELOP); }
\<=		{ yylval.C = LE; return(RELOP); }
>=		{ yylval.C = GE; return(RELOP); }
>		{ yylval.C = GT; return(RELOP); }
\<		{ yylval.C = LT; return(RELOP); }
\"{IntString}\" { yylval.S = strdup(yytext); return STRING; }
{Delim}+	;
.		printf("Caractere errone: %c\n", yytext[0]);
%%
