/*
*	Compilateur du Quadrinome :
*	LOMBARD Felix , SCHILIS Vivien
*	TOPCU Julien, Flandre Guillaume
*/

#include <fcntl.h>
#include <stdio.h>
#include "main.h"
 
extern int yylineno;

int yychar;
FILE *fd = nil(FILE);
char *fname;

char* check_type(PARBRE, char *,PATT);
PCLASSE get_class(char * name);
PCLASSE MakeClasse(char *name, char *name_parent, PATT lattributs, PMETH lmethodes);
PMETH MakeMethode(char *name, PATT params,char *type, PARBRE expression);


void opencodefile();
void closecodefile();
void writecode(char* str);
void writecodeln(char* str);
void writecodei(int i);
void writecodeiln(int i);
int index_att(char*,char* );

int newlbl();
char* lbl(int l);

void writecode_exp(PARBRE arbre, PATT super_fin_env);

main(int argc, char **argv) {
  int fi;

  if (argc < 2) { fprintf(stderr, "Fichier programme manquant\n"); exit(1);}
  if ((fi = open(argv[1], O_RDONLY)) == -1) {
    fprintf(stderr, "Je n'arrive pas a ouvrir: %s\n", argv[1]); exit(1);
  }

  definedClasses = NULL;
  mainBloc = NULL;
  AttributsEnvironnement = NULL;
  MethodesEnvironnement = NULL;
  current_class_name = NULL;
  parent_current_class_name = NULL;
  current_methode = NULL;
  current_methode_return_type = NULL;
  lbli=0;
  classi=0;
  /* redirige l'entree standard sur le fichier... */
  close(0); dup(fi); close(fi);

  if (argc >= 3) {
    fname = argv[2];
    if ((fd = fopen(fname,  "r")) == NULL)
      { perror(fname); exit(1); }
  }

  opencodefile();
  writecodeln("START");
  writecodeln("JUMP Main");


  if (yyparse()) { /* en cas d'erreur lexicale ou syntaxique */
    fprintf(stderr, "Programme Incorrect \n"); exit(2); 
  }

  writecodeln("STOP");
  closecodefile();
  
  fprintf(stdout,"out: code.txt\ndone.\n");
}


//
/* Affiche le contenu d'un programme
 */
Decompile() {
	PCLASSE lc = definedClasses;
	PATT la = NULL;
	PARBRE lb = NULL;
	PMETH lm = NULL;
	
  printf("Resultat du programme:\n");	  
  while (lc) { 
	  printf("Classe : %s\n", lc->name);
	  la = lc->lattributs;
	  printf("Attributs : \n");
	  while (la) { 
		  printf("\t %s %s\n", la->type, la->name);
		  la = la->suiv;
	  }

	  lm = lc->lmethodes;
	  printf("Methodes : \n");
	  while (lm) { 
		  printf("\t %s ", lm->name);
		  printf("Paramètres : ");
		  la = lm->lparams;
		  while (la) { 
			  printf("%s %s ", la->type, la->name);
			  la = la->suiv;
		  }
		  printf("Retourne %s\n", lm->type);		  
		  lm = lm->suiv;
	  }	  
	  lc = lc->suiv; 
  }

  lb = mainBloc;
  if(lb){
  		printf("Bloc :\n");
  		PATT la2 = lb->droit.lattributs;
  		printf("\tAttributs : \n");
  		while (la2) { 
  		  printf("\t\t %s %s\n", la2->type, la2->name);
  		  la2 = la2->suiv;
  		} 
  }

}

/**
 * Enrichit l'environnement des attributs avec les attributs de la hierarchie de la 
 * classe parente donnée en paramètre et les variables locales.
 * 
 * Renvoie un pointeur vers le début de la liste des variables globales, ce qui permettra
 * entre autre de "desenchérir" l'environnement lors de la remonté d'un bloc ou d'une méthode. 
 */
PATT enrichissement_att_environnement(char * class_parent_name, PATT lattributs_locaux){
	PATT env = AttributsEnvironnement;
	PCLASSE class_parent;
	PATT fin_env = NULL;

	//ajout des attributs de la hiérarchie
	if(class_parent_name!=NULL){
		class_parent = get_class(class_parent_name);
		if(env){
			while(env->suiv){
				env = env->suiv;
			}
			if(class_parent){
				env->suiv= NEW(1,ATTRIBUT);
				memcpy(env->suiv,class_parent->lattributs,sizeof(ATTRIBUT));
				enrichissement_att_environnement(class_parent->name_parent,NULL);//parcours de la hiérarchie
			}
		}else{
			if(class_parent == NULL){
				fprintf(stderr,"Classe %s inconnue\n",class_parent_name);
				exit(4);
			}
			if(class_parent->lattributs){
				AttributsEnvironnement = NEW(1, ATTRIBUT);
				memcpy(AttributsEnvironnement,class_parent->lattributs,sizeof(ATTRIBUT));
			}
		}
	}
	
	env = AttributsEnvironnement;
	//ajout des variables locales.
	if(lattributs_locaux){
		if(env){
			while(env->suiv){
				env = env->suiv;
			}
			env->suiv= NEW(1,ATTRIBUT);	
			fin_env = env->suiv;
			memcpy(env->suiv,lattributs_locaux,sizeof(ATTRIBUT));
		}else{
			AttributsEnvironnement = NEW(1, ATTRIBUT);
			fin_env = AttributsEnvironnement;
			memcpy(AttributsEnvironnement,lattributs_locaux,sizeof(ATTRIBUT));		
		}
	}
	return fin_env;
}


PCLASSE MakeClasseEntier()
{
	PMETH methodes = MakeMethode("imprimer",NULL,"Chaine",NULL);
	PCLASSE c = MakeClasse ("Entier",NULL,NULL,methodes);
	return (c);
}

PCLASSE MakeClasseChaine()
{
	PMETH methodes = MakeMethode("imprimer",NULL,"Chaine",NULL);
	PCLASSE c = MakeClasse ("Chaine",NULL,NULL,methodes);
	return (c);
}

/**
 * Vérifie l'intégrité d'un bloc
 * 
 */
PARBRE check_bloc(PARBRE a, PATT env_att, PMETH env_meth, char * class_parent_name, char * class_name) {
	current_class_name = class_name;
	parent_current_class_name = class_parent_name;
	//création de l'environnement
	if(env_att){
		AttributsEnvironnement = NEW(1,ATTRIBUT);
		memcpy(AttributsEnvironnement,env_att,sizeof(ATTRIBUT));
	}
	if(env_meth){
		MethodesEnvironnement = NEW(1,METHODE);
		memcpy(MethodesEnvironnement,env_meth,sizeof(METHODE));
	}
	enrichissement_att_environnement(class_parent_name,NULL);
	
	//parsage du bloc et récupération du type de retour de celui-ci
	char * type = check_type (a,NULL,NULL);
	AttributsEnvironnement = NULL;
	MethodesEnvironnement = NULL;
	
	return a;
}

/**
 * Notifie d'une erreur d'identifiant inconnu
 */
void id_not_found(char * id){
	fprintf(stderr, "Erreur : Variable inconnue %s\n",id);
	exit(4);
}

/**
 * Notifie d'une erreur d'une méthode inconnue
 */
void fct_not_found(char * id){
	fprintf(stderr, "Erreur : Methode inconnue %s\n",id);
	exit(4);
}
/**
 * Appauvrit l'environnement des variables locales.
 * 
 */
void desenrichissement_att_environnement(PATT fin_env){
	PATT env = AttributsEnvironnement;
	if(env){
		while(env->suiv!=fin_env && env->suiv)
		{
			env = env->suiv;
		}
		env->suiv = NULL;
	}
}
/**
 * Vérifie si un arbre représente un appel de méthode ou non
 */
int is_methode_call(PARBRE arbre){
	if(arbre->op == Fct){
		return 1;
	}else if(arbre->op == '.'){
		return is_methode_call(arbre->droit.A);
	}else return 0;
}

/**
 * Vérifie le type d'une expression dans l'environnement.
 * L'environnement parent est délimité par super_env_fin, ce qui signifie
 * que ce pointeur pointe dans l'environnement des attributs vers la première
 * variable locale d'un certain noeud de l'arbre.
 */
char* check_type(PARBRE arbre, char * type, PATT super_fin_env) {
	char * typeg = NULL;
	char * typed = NULL;
	PATT local_fin_env = NULL;
	char * id = arbre->gauche.S;

	if(arbre)
	switch (arbre->op)
	{
		case New :
				return arbre->gauche.A->gauche.S;
				break;
				
		case Bloc : 
				//enrichissement de l'environnement avec les variables locales du bloc
				local_fin_env = enrichissement_att_environnement(NULL,arbre->droit.lattributs);
				//parsage et récupréation du type du bloc dans l'environnement
				char * t = check_type(arbre->gauche.A, NULL,local_fin_env);
				//appauvrissement de l'environnement pour remonter d'un noeud.
				desenrichissement_att_environnement(local_fin_env);
				return t; 
				break;

		case Self :
			return current_class_name;
			break;

		case Super :
			if(parent_current_class_name==NULL){
				fprintf(stderr, "Utilisation de super impossible : \n"); 
				fprintf(stderr, "La classe %s n'a pas de classe parente\n",current_class_name); 				    
				exit(2); 
			}
			return parent_current_class_name;
			break;
				
		case Id : {
			if(type == NULL)  {			// variable 

				PATT la = super_fin_env;//recherche locale
				while(la){
					if(strcmp(id,la->name) == 0) {
						//printf("attribut local %s",la->name);
						return (la->type);
					}
					la = la->suiv;
				}		
				if(current_methode!=NULL){//recherche dans les paramètres de la méthode
					//si on est dans une méthode
					PMETH meth = MethodesEnvironnement;
					int found = 0;
					while(meth && !found){
						if(!strcmp(meth->name,current_methode)){
							found =1;
							la = meth->lparams;
							while(la){
								if(!strcmp(id,la->name)){
									return la->type;
								}
								la = la->suiv;
							}
						}else meth = meth->suiv;
					}
				}
					
				la = AttributsEnvironnement;//recheche globale		
				while(la && la!=super_fin_env){
					if(strcmp(id,la->name) == 0) {
						
						return (la->type);
					}
					la = la->suiv;
				}
				id_not_found(id);
				return NULL;
			}
			else if (current_class_name && strcmp(type,current_class_name) == 0) // apres un self.
			{			
				PATT la = AttributsEnvironnement;//recherche globale
				while(la && la!=super_fin_env){
					if(strcmp(id,la->name) == 0) {
						return (la->type);
					}
					la = la->suiv;
				}
				id_not_found(id);
				return NULL;
				
			}
			else{		// attribut	d'une classe	
				PCLASSE lc = get_class(type);
				if(lc == NULL){ fprintf(stderr,"Classe %s inconnue\n",type); exit(4);}
				PATT la = lc->lattributs;
				while (la) {
					if(strcmp(la->name,id) == 0) {
							return la->type;	
					}
					la = la->suiv;
				}
				id_not_found(id);
				return NULL;
			}
		} break;
        
		case Fct :{	
			
			//Gestion d'un appel de methode
			PFONC f = arbre->gauche.F;
			if( type != NULL && ((!strcmp( type, "Entier" ) || !strcmp( type, "Chaine" )))
				 && !strcmp( f->name, "imprimer" )){
				if( f->largs == NULL ) return type;
				else{ printf("Imprimer ne prend pas de parametres\n"); exit(4); }
			}

			PMETH lm = NULL; 
			PATT am = NULL;
			PARG af = NULL;

			char * param_type;
			
			if(current_methode)
			if(!strcmp(f->name,current_methode)){
				return current_methode_return_type;
			}
			
			if(type) {
				//récupération de la classe
				PCLASSE lc = get_class(type);
				if(lc){//récupération des méthodes de la classe
					lm = lc->lmethodes;
				}else{
					if(type && current_class_name)
					if(strcmp(type,current_class_name) == 0) {
						lm = MethodesEnvironnement;
					}
				}
			}else{
				lm = MethodesEnvironnement;
			}
			PCLASSE super_class = NULL;
			//parcours des méthodes de la classe et vérification de la conformité
			//des appels
			
			do{
				while (lm) {
					if(strcmp(lm->name,f->name) == 0) {
						//on a trouvé la methode de l'appel
						am = lm->lparams;
						af = f->largs;

						
						int i = 0;
						//Vérification des paramètres de l'appel
						while(am!=NULL && am!=super_fin_env && af!=NULL){
							//verification des types des paramètres
							param_type = check_type(af->expression,NULL,super_fin_env);
							if(param_type){
								if(strcmp(param_type,am->type)!=0) {
									int found = 0;
									PCLASSE parent = get_class(param_type);
									parent = get_class(parent->name_parent);
									while(!found && parent){
										if(strcmp(parent->name,am->type)==0)
											found = 1;
										parent = get_class(parent->name_parent);										
									}
									if(!found){
										fprintf(stderr, "Erreur : type incompatible dans l'appel de fonction %s : param %s devrait être %s \n", f->name, param_type,am->type);
										exit(4);
									}
								}
							}
							i++;
							am = am->suiv;
							af = af->suiv;
						}

						if((am!=NULL && am!=super_fin_env) || af!=NULL){
							fprintf(stderr, "Erreur : nombre de paramètre incorrect dans l'appel de fonction %s\n", f->name);
							exit(4);
						}
						return lm->type;
					}
					lm = lm->suiv;
				}
				//recherche dans la hiérarchie
				if(super_class == NULL){
					if(type == NULL){
						super_class = get_class(parent_current_class_name);
					}else{
						if(current_class_name){
							if(!strcmp(current_class_name,type))
								super_class = get_class(parent_current_class_name);
							
						}else{ super_class = get_class(type);
							if(super_class){
								super_class = get_class(super_class->name_parent);
							}
						}
					}
				}else{
					super_class = get_class(super_class->name_parent);
				}

				if(super_class){ 
					lm = super_class->lmethodes;
				}

			}while(super_class);
			fct_not_found(f->name);//aucune methode ne correspond à cet appel
			return NULL;
		} break;
		
		case Aff: {
			//Affectation
			typeg = check_type(arbre->gauche.A, NULL,super_fin_env);

			typed = check_type(arbre->droit.A, NULL,super_fin_env);			
			
			int found = 0;
			if (strcmp(typeg,typed) != 0) 
			{	//recherche des types hiérarchiques
				PCLASSE lc = get_class(typed);
				char * sclass = NULL;
				if(lc){
					sclass= lc->name_parent;
				}else if(!strcmp(typed,current_class_name)){
					sclass = parent_current_class_name;
				}
				while(sclass && !found){
					if (strcmp(sclass,typeg) == 0){
						found = 1;
					}
					lc = get_class(sclass);
					if(lc){
						sclass = lc->name_parent;
					}else sclass = NULL;
				}
			}else found =1;
			if(!found){
				fprintf(stderr, "type incompatible : %s %s %s \n", typeg, ":=", typed);
				exit(4);
			}	
			
			
		}
		return typed;
		break;
			
		case ';':
			//Reconnaissance Expression
			typeg = check_type(arbre->gauche.A, NULL,super_fin_env);
			typed = check_type(arbre->droit.A, NULL,super_fin_env);
			return typed;
			
			break;
			
		case '.' :
			//Accès sur un objet
			typeg = check_type(arbre->gauche.A, NULL,super_fin_env);

			if(typeg == NULL) {
				fprintf(stderr,	 "Type Inconnu\n");
				exit(4);
			}

			int is_methode = is_methode_call(arbre->droit.A);
			int forbidden = 0;
			//Si on tente d'accéder à un attribut, on vérifie si on en a le droit
			if(!is_methode){
				forbidden = 1;
				if(current_class_name!=NULL)
				if(strcmp(typeg,current_class_name)!=0){
					PCLASSE parent = get_class(typeg);
					parent = get_class(parent->name_parent);
					//recherche de l'attribut dans la hiérarchie de l'objet
					while(parent && forbidden){
						if(strcmp(parent->name,current_class_name)==0){
							forbidden = 0;
						}
						parent = get_class(parent->name_parent);
					}
				}else forbidden = 0;
				
				if(forbidden && parent_current_class_name!=NULL)
				if(strcmp(typeg,parent_current_class_name)!=0){
					//l'objet fait partie de la hiérarchie de la classe courante
					PCLASSE parent = get_class(parent_current_class_name);
					parent = get_class(parent->name_parent);
					while(parent && forbidden){
						if(strcmp(parent->name,typeg)==0){
							forbidden = 0;
						}
						parent = get_class(parent->name_parent);					
					}
				}else forbidden = 0;				
			}
			if(forbidden){
				fprintf(stderr, "Acces d'attribut interdit sur : %s\n", typeg);
				exit(4);				
			}

			typed = check_type(arbre->droit.A, typeg,super_fin_env);
			return typed;
			break;
			
		case Cste:			
			//Reconnaissance d'une constante
			return "Entier";
			break;
			
		case String:
			//Reconnaissance d'une Chaine
			return "Chaine";
			break;
		
		
 		case '+':
 //			Operation d'addition
			typeg = check_type(arbre->gauche.A,NULL,super_fin_env);
			typed = check_type(arbre->droit.A,NULL,super_fin_env);
			if(typed!=NULL && typeg!=NULL)			
			if(	strcmp(typeg,typed) != 0) {
				fprintf(stderr, "type incompatible : %s %c %s \n", typeg, '+', typed);
				exit(4);
			}
			return typeg;
		    
		 case '-': 			
			 //			Operation de soustraction		 
			typeg = check_type(arbre->gauche.A,NULL,super_fin_env);
			typed = check_type(arbre->droit.A,NULL,super_fin_env);
			
			if(typed!=NULL && typeg!=NULL)
			if(	strcmp(typeg,typed) != 0) {
				fprintf(stderr, "type incompatible : %s - %s \n", typeg, typed);
				exit(4);
			}
			return typeg;
			
		 case '*':
			typeg = check_type(arbre->gauche.A,NULL,super_fin_env);
			typed = check_type(arbre->droit.A,NULL,super_fin_env);
			
			if(typed!=NULL && typeg!=NULL)
			if(	strcmp(typeg,typed) != 0) {
				fprintf(stderr, "type incompatible : %s * %s \n", typeg, typed);
				exit(4);
			}	
		    return typeg;
		
		 case '/':			
			typeg = check_type(arbre->gauche.A,NULL,super_fin_env);
			typed = check_type(arbre->droit.A,NULL,super_fin_env);
			
			if(typed!=NULL && typeg!=NULL)
			if(	strcmp(typeg,typed) != 0) {
				fprintf(stderr, "type incompatible : %s / %s \n", typeg, typed);
				exit(4);
			}			
			return typeg;
		
		case ITE :
			//if then else
			typeg = check_type(arbre->gauche.A,NULL,super_fin_env);
			typed = check_type(arbre->droit.A,NULL,super_fin_env);
			return typed;
			break;
		
		case CONC : 
			//partie then else d'un if
			typeg = check_type(arbre->gauche.A,NULL,super_fin_env);
			typed = check_type(arbre->droit.A,NULL,super_fin_env);
			
			//recherche de la classe parente commune au renvoi du then et celui du else
			if( typeg == NULL || typed == NULL || strcmp(typed,typeg) != 0) {
				int found = 0;							
				if(typeg!=NULL && typed!=NULL){
					PCLASSE pereG = NULL;
					char * nomG = NULL;
					if(!strcmp(typeg,current_class_name)){
						nomG = current_class_name;
					}else {
						pereG = get_class(typeg);
						if(pereG)
						nomG = pereG->name;
					}
					PCLASSE pereD = NULL;
					char * nomD = NULL;
					while(nomG && !found){
						if(!strcmp(typed,current_class_name)){
							nomD = current_class_name;
						}else{
							pereD = get_class(typed);
							if(pereD)
							nomD = pereD->name;
						}
						while(!found && nomD){
							if(!strcmp(nomG,nomD)){
								found =1;
							}
							if(pereD){
								pereD = get_class(pereD->name_parent);	
							}else{
								pereD = get_class(parent_current_class_name);
							}
							if(pereD){
								nomD = pereD->name;
							}else nomD = NULL;
						}							
						
						if(pereG){
							pereG = get_class(pereG->name_parent);	
						}else{
							pereG = get_class(parent_current_class_name);
						}
						if(pereG){
							nomG = pereG->name;
						}else nomG = NULL;
					}
				}
				if(!found){
					fprintf(stderr, "type du then (%s) incompatible avec le type du else (%s) \n", typeg,typed);
					exit(4);
				}
			}
			return typed;
			break;
			
			case NEQ : case EQ : case GT : case LE : case LT : case GE :
		//Operation relationelle
			typeg = check_type(arbre->gauche.A,NULL,super_fin_env);
			if(strcmp("Entier",typeg) != 0) {
				fprintf(stderr, "type incompatible avec l'operateur relationnel : %s \n", typeg);
				fprintf(stderr, "type attendu : Entier \n");
				exit(4);
			}
			
			typed = check_type(arbre->droit.A,NULL,super_fin_env);
			if(strcmp("Entier",typed	) != 0) {
				fprintf(stderr, "type incompatible avec l'operateur relationnel : %s \n", typed);
				fprintf(stderr, "type attendu : Entier \n");
				exit(4);
			}
			
			return typed;
			break;
		
		default: return NULL;
	}
	return NULL;
}



int Evalue(PARBRE arbre) {
	int g,d;
		
	switch (arbre->op)
	{
		case Cste:
	    	return(arbre->gauche.E);
	
 		case '+':	
		   g = Evalue(arbre->gauche.A);
		   d = Evalue(arbre->droit.A);
		    return (g + d);
		
		 case '-':
		   g = Evalue(arbre->gauche.A);
		   d = Evalue(arbre->droit.A);
		   return (g - d);
		
		 case '*':
		   g = Evalue(arbre->gauche.A);
		   d = Evalue(arbre->droit.A);
		   return (g * d);
		
		 case '/':
		   g = Evalue(arbre->gauche.A);
		   d = Evalue(arbre->droit.A);
		   if (d != 0)  return (g / d);
		   else { fprintf(stderr, "Division par Zero\n"); exit(3); }
		
		 default: fprintf(stderr, "Cas non prevu dans Evaluation: %c\n", arbre->op);
		    exit(4);
	}
}

int Evalue2(PARBRE arbre) {
	int g,d;
		
	switch (arbre->op)
	{
		case Cste:
		printf("%s ",arbre->gauche.E);   break;
		case String:                         
		printf("%s ",arbre->gauche.S);   break;
		case Id :
			printf("%s ",arbre->gauche.S);   break;
		
		 default: fprintf(stderr, "Cas non prevu dans Evaluation: %c\n", arbre->op);
		    exit(4);
	}
}

/**
 * enregistre une classe dans la liste des classes définies
 * 
 */
void store_next(PCLASSE clazz){
	PCLASSE lc = definedClasses;
	if(lc!=NULL){
		while(lc->suiv){
			lc = lc->suiv;
		}
		lc->suiv = NEW(1,CLASSE); 
		memcpy(lc->suiv,clazz,sizeof(CLASSE));

	}else{

		definedClasses= clazz;
	}
}


/**
 * Vérifie si une liste d'attribut contient une variable donnée par son
 * identifiant
 */
int contains_variable(char * id, PATT lattributs){
	int found = 0;
	PATT att = lattributs;
	while(!found && att!=NULL){
		if(!strcmp(id,att->name)){
			found = 1;
		}
		att = att->suiv;
	}	
	return found;
}

/**
 * Retourne la classe associée au nom name
 */
PCLASSE get_class(char * name){
	PCLASSE lc = definedClasses;
	int find = 0;
	
	if(name == NULL)
		return NULL;
	
	while(lc && !find){
		if(!strcmp(lc->name,name)){
			find = 1;
		}else lc = lc ->suiv;
	}
	
	return lc;
}

/**
 * Vérifie si une classe ou un type a été déclaré
 */
void check_type_declaration(char * name, char * current_class_name){
	
	PCLASSE lc = definedClasses;
	int find = 0;
	
	if(current_class_name!=NULL)
	if(!strcmp(current_class_name, name)){
		find = 1;
	}
	if(! strcmp("Entier", name) ||  !strcmp("Chaine", name)){
				find = 1;
	}
	
	while(!find && lc){
		if(! strcmp(lc->name, name)){
			find = 1;
		}
		lc = lc ->suiv;
	}
	
	if(!find){
	      fprintf(stderr, "Erreur! type %s inconnue\n", name);
	      exit(3); 		
	}
}


PARG MakeArgument(PARBRE expression) {
  PARG res;

  res = NEW(1, ARGUMENT); 
  res->expression = expression;
  res->suiv = nil(ARGUMENT);
  return(res);
}

PARG add_argument(PARG a, PARG larguments)
{
	PARG la = larguments;
	
	if(la){	
		while(la->suiv) {
			la = la->suiv;
		}  
		la->suiv = a;
	}else{
		return a;
	}
	
	return larguments;
}


PARBRE MakeAppel(PFONC f)
{
	PARBRE res;
	
	res = NEW(1,ARBRE);
	res->op = Fct;
	res->gauche.F = f; res->droit.A = nil(ARBRE);
	return (res);
}

PFONC MakeFonction(char * name, PARG args)
{
	PFONC res;
	
  	res = NEW(1, FONCTION); 
  	res->name = name; 
	res->largs = args;
  	return(res);
}


/**
 * Création d'une Classe
 */
PCLASSE MakeClasse(char *name, char *name_parent, PATT lattributs, PMETH lmethodes) {
  PCLASSE res;
  PCLASSE lv = definedClasses;

  //une classe ne peut héritée de elle même:
  if(name_parent != NULL )
  if(! strcmp(name, name_parent) || !strcmp("Entier", name_parent) || !strcmp("Chaine", name_parent)){
      fprintf(stderr, "Erreur! %s ne peut être une classe parente de la classe %s \n", name_parent, name);
      exit(3);   
   }else{
	   check_type_declaration(name_parent,NULL);
   }
  /* On ne peut que déclarer une seule classe ayant le nom name.
   */
  while(lv) {
    if (! strcmp(name, lv->name)) {
      fprintf(stderr, "Erreur! double declaration de Classe: %s\n", name);
      exit(3);
    }
    else lv = lv->suiv;
  }

  res = NEW(1, CLASSE); 
  res->name = name; 
  res->name_parent = name_parent;
  res->lattributs = lattributs;
  res->lmethodes = lmethodes; 
  res->suiv = nil(CLASSE);
  res->index = classi;
  classi++;
  store_next(res);
  return(res);
}

/**
 * Création d'un Attribut
 */
PATT MakeAttribut(char *type, char *name) {
  PATT res;

  res = NEW(1, ATTRIBUT); 
  res->name = name; 
  res->type = type; 
  res->suiv = nil(ATTRIBUT);
  return(res);
}

/**
 * Ajout d'un attribut
 */

PATT add_attribut(PATT att, PATT lattributs){
	PATT lv = lattributs;
	  
	  if(lv!=NULL){
		  /* On ne peut que déclarer un seul attribut ayant le nom name.
		   */
		  while(lv) {
		    if (! strcmp(att->name, lv->name)) {
		      fprintf(stderr, "Erreur! double declaration d'attribut: %s\n", att->name);
		      exit(3);
		    }
		    else lv = lv->suiv;
		  }
		  
		  //ajout de l'attribut
		  lv = lattributs;
		  while(lv->suiv) {
			  lv = lv->suiv;
		  }	
		  
		  lv->suiv = att;
	  }else{
		  lattributs = att;
	  }
	  
	  return lattributs;
}

PARBRE MakeBloc(PARBRE g, PATT la) {
	PARBRE res;

	res = NEW(1, ARBRE);
	res->droit.lattributs = la;
	res->op = Bloc;
	res->gauche.A = g;
	
	return (res);
}



/**
 * Création d'une Methode
 */
PMETH MakeMethode(char *name, PATT params,char *type, PARBRE expression) {
  PMETH res;
  
  res = NEW(1, METHODE); 
  res->name = name; 
  res->lparams = params;
  res->type = type; 
  res->suiv = nil(METHODE);
  res->expression = expression;
    
  return(res);
}


/**
 * Ajout d'une Methode
 */

PMETH add_methode(PMETH meth, PMETH lmethodes){
	PMETH lv = lmethodes;
	  
	  if(lv!=NULL){
		  /* La surcharge est interdite dans une même classe.
		   */
		  while(lv) {
		    if (! strcmp(meth->name, lv->name)) {
		      fprintf(stderr, "Erreur! Surcharge interdite: %s\n", meth->name);
		      exit(3);
		    }
		    else lv = lv->suiv;
		  }
		  
		  //ajout de la méthode
		  lv = lmethodes;
		  while(lv->suiv) {
			  lv = lv->suiv;
		  }	
		  
		  lv->suiv = meth;
	  }else{
		  lmethodes = meth;
	  }
	  return lmethodes;
}
/**
 * Vérifie le type des attributs de la classe
 */
PATT check_attributs(PATT attributs, char *class_name,char * class_parent_name){
	PATT la = attributs;
	
	PATT env;
	PCLASSE super_class = get_class(class_parent_name);
	while(la){
		while(super_class){
			env = super_class->lattributs;
			//Vérification de la déclaration du type de l'attribut
			while(env){
				if(!strcmp(env->name,la->name)){
					fprintf(stderr,"Erreur Impossibilité de masquer l'attribut de la classe parente %s %s\n",env->type,env->name);
					exit(4);
				}
				env = env->suiv;
			}
			super_class = get_class(super_class->name_parent);
		}
		check_type_declaration(la->type,class_name);
		la = la ->suiv;
	}
	AttributsEnvironnement = NULL;
	return attributs;
}
/**
 * Vérifie si une redéfinition est bonne ou non
 */
void check_superclass_declaration(PMETH lm, PCLASSE super_class){
	PMETH super_methodes;
	int find = 0;
	PATT child_att;
	PATT parent_att;
	if(super_class!=NULL){
		super_methodes = super_class->lmethodes;
		while(!find && super_methodes){
			if(!strcmp(super_methodes->name,lm->name)){
				find = 1;
				if(!strcmp(super_methodes->type,lm->type)){
					child_att = lm->lparams;
					parent_att = super_methodes->lparams;
					while(child_att!=NULL && parent_att!=NULL){
						if(strcmp(child_att->type,parent_att->type)!=0){
							fprintf(stderr, "Erreur! Redéfinition erronée : ");
							fprintf(stderr, "Paramètre %s %s incompatible dans la méthode % s ",child_att->type, child_att->name,lm->name);
							fprintf(stderr, "avec la méthode de la super classe %s  : %s %s\n",super_class->name,parent_att->type, parent_att->name );	
							exit(3);
						}
						child_att = child_att->suiv;
						parent_att = parent_att->suiv;						
					}
					
					if(child_att!=NULL || parent_att!=NULL){
						fprintf(stderr, "Erreur! Redéfinition erronée : ");
						fprintf(stderr, "Mauvais nombre de paramètres dans la méthode %s ",lm->name);
						fprintf(stderr, "d'après la super classe %s \n",super_class->name);	
						exit(3);
					}
				}else{
				      fprintf(stderr, "Erreur! Redéfinition erronée : ");
				      fprintf(stderr, "Le type de retour de la methode %s est %s ",lm->name, lm->type);
				      fprintf(stderr, "alors qu'il devrait être %s comme spécifié dans sa superclasse %s \n",super_methodes->type, super_class->name);				      
				      exit(3);					
				}
			}
			super_methodes = super_methodes->suiv;
		}
		//Si on n'a pas trouvé , on recherche dans la super classe de la super classe
		if(!find){
			check_superclass_declaration(lm,get_class(super_class->name_parent));
		}
	}
}
/**
 * Vérifie si le type de retour d'une fonction existe bien
 * Ainsi que le type de ses paramètres.
 */


PMETH check_methodes(PMETH methodes, char * class_name, char * super_class_name, PATT lattributs){
	PMETH lm = methodes;
	PCLASSE super_class = get_class(super_class_name); 
	
	current_class_name = class_name;
	parent_current_class_name = super_class_name;
	  if(super_class_name != NULL )
	  if(! strcmp(class_name, super_class_name) || !strcmp("Entier",super_class_name) || !strcmp("Chaine", super_class_name)){
	      fprintf(stderr, "Erreur! %s ne peut être une classe parente de la classe %s \n", super_class_name, class_name);
	      exit(3);   
	   }	
	  
	
	while(lm){

		current_methode = lm->name;
		//Vérification du type de retour
		check_type_declaration(lm->type,class_name);
		current_methode_return_type = lm->type;		
		//Vérification des paramètres
		check_attributs(lm->lparams,class_name,NULL);
		 if(lattributs){
		  			AttributsEnvironnement = NEW(1,ATTRIBUT);
		  			memcpy(AttributsEnvironnement,lattributs,sizeof(ATTRIBUT));
		  	}else AttributsEnvironnement = NULL;
		  		
		  	if(methodes){
		  			MethodesEnvironnement = NEW(1,METHODE);
		  			memcpy(MethodesEnvironnement, methodes,sizeof(METHODE));
		  } else MethodesEnvironnement = NULL;
		PARBRE a = lm->expression;
		
		PATT local_fin_env = enrichissement_att_environnement(super_class_name,lm->lparams);
		char * type_exp = check_type (a,NULL,local_fin_env);
		desenrichissement_att_environnement(local_fin_env);
		if(type_exp==NULL) 
		{			
			fprintf(stderr,"Methode %s \n",lm->name);
			fprintf(stderr,"Type de retour incorrect\n");
			exit(4);
		}
		
		int found = 0;
		if (strcmp(type_exp,lm->type) != 0) 
		{
			PCLASSE lc = get_class(type_exp);
			
			char * sclass = NULL;
			if(lc){
				sclass= lc->name_parent;
			}else if(!strcmp(type_exp,current_class_name)){
				sclass = super_class_name;
			}
			while(sclass && !found){
				if (strcmp(sclass,lm->type) == 0){
					found = 1;
				}
				lc = get_class(sclass);
				if(lc){
					sclass = lc->name_parent;
				}else sclass = NULL;
			}
		}else found =1;
		if(!found){
			fprintf(stderr, "Erreur de retour (%s): ",type_exp);
			fprintf(stderr, "type attendu %s\n", lm->type);
			exit(4);
		}

		check_superclass_declaration(lm,super_class);
		AttributsEnvironnement = NULL;
		MethodesEnvironnement = NULL;
		lm = lm->suiv;
	}
	current_methode = NULL;
	current_methode_return_type = NULL;
	return methodes;
		
}


PARBRE MakeId(char *var) {
  PARBRE res;

  res = NEW(1, ARBRE);
  res->op = Id; res->gauche.S = var; res->droit.A = nil(ARBRE);
  return(res);
}

PARBRE MakeCste(int val) {
  PARBRE res;

  res = NEW(1, ARBRE);
  res->op = Cste; res->gauche.E = val; res->droit.A = nil(ARBRE);
  return(res);
}

PARBRE MakeString(char *var) {
  PARBRE res;

  res = NEW(1, ARBRE);
  res->op = String; res->gauche.S = var; res->droit.A = nil(ARBRE);
  return(res);
}

PARBRE MakeNoeud(char op, PARBRE g, PARBRE d) {
 PARBRE res;

  res = NEW(1, ARBRE); res->op = op; res->gauche.A = g; res->droit.A = d;
  return(res);
}

PARBRE MakeWhere(PARBRE g, char *nom, PARBRE d) {
  PARBRE res;

  res = NEW(1, ARBRE);
  res->op = Wh; res->gauche.A = g;
  res->droit.A = NEW(1, ARBRE);
  res->droit.A->gauche.S = nom;
  res->droit.A->droit.A = d;
  return res;
}


yyerror(char *s) {
 printf("Ligne %d, %s\n", yylineno, s);
}


lire() { /* voir le cas GET dans Evalue */
  char buf[50]; int res;

  if (fd == nil(FILE)) {
    fprintf(stderr, "Fichier de donnees manquant\n");
    exit(1);
  }
  fgets(buf, 50, fd);
  sscanf(buf, "%d", &res);
  return(res);
}


/**
 * Retourne la liste des classes définies
 */
PCLASSE get_definedClasses(){ return definedClasses;}


void opencodefile(){
fcode = fopen("code.txt", "w");
if(fcode==NULL){
	fprintf(stderr,"Error: ecriture impossible : code.txt\n");
	exit(1);
}
}

void writecode(char* str){
fprintf(fcode,str);
}

void writecodei(int i){
fprintf(fcode,"%d",i);
}

void writecodeiln(int i){
fprintf(fcode,"%d",i);
fprintf(fcode,"\n");
}

void writecodeln(char* str){
fprintf(fcode,str);
fprintf(fcode,"\n");
}

void closecodefile(){
fclose(fcode);
}

char* lbl(int l){
char* temp = NEW(10,char);
sprintf(temp,"LBL%d",l);
return temp;
}

int newlbl(){
lbli++;
return lbli;
}



int index_att(char* name, char* id){
	int index = -1;
	PCLASSE lc = get_class(name);
	while(lc){
		PATT att = lc->lattributs;
		while(att){
			index++;
			if( strcmp(id,att->name)==0 ) return index;
			att = att->suiv;
		}
		lc = get_class(lc->name_parent);
	}
	return -1;
}


int get_meth_index(char* classname, char* id){

	if(classname==NULL) return -1;
	
	PCLASSE c = get_class(classname);	
	int index = get_meth_index(c->name_parent,id);
	if(index != -1) return index; 
	PMETH lm = c->lmethodes;
	index = count_methods(c->name_parent) - 1;
	while(lm){
		if( get_meth_index(c->name_parent,lm->name) == -1 ) index++;
		if( strcmp(lm->name,id)==0 ) return index;
		lm = lm->suiv;
	}
	return -1;
}

int get_var_index(PATT att, char* var){
	int i = 0;
	int index = -1;
	while(att){
		if( strcmp( att->name , var ) == 0 ) index = i;
		i++;
		att = att->suiv;
	}
	return index;
}

int index_param(PMETH m, char* id){
	int index = 0;
	
	if(m) {
		PATT la = m->lparams2;
		while(la){
			index++;
			if( strcmp(id,la->name)==0 ) return index;
			la = la->suiv;
		}
	}
	return 0;
}

int count_params(PMETH m){
	int index = 0;
	if(m) {
		PATT la = m->lparams2;
		while(la){
			index++;
			la = la->suiv;
		}
	}
	return index;
}

void writecode_class( PCLASSE cl ){

	PMETH lm = cl->lmethodes;
	current_class_name = cl->name;
	parent_current_class_name = cl->name_parent;
	

	while(lm){
		PATT m = lm->lparams;
		lm->lparams2 = NULL;
		PATT m2 = lm->lparams2;
		PATT prec = NULL;
		while(m){
			m2 = NEW(1,ATTRIBUT);
			if( prec == NULL ) lm->lparams2 = m2;
			if( prec != NULL ) prec->suiv = m2;
  			memcpy(m2,m,sizeof(ATTRIBUT));
			m2->suiv = NULL;
			prec = m2;
			m2 = m2->suiv;
			m = m->suiv;
		}
		
		 m = lm->lparams2;
		while(m){
			m = m->suiv;
		}

		 int label = newlbl();
		 writecode(lbl(label));
		 writecodeln(":NOP");
		 current_method = lm;

		  if(cl->lattributs){
		  			AttributsEnvironnement = NEW(1,ATTRIBUT);
		  			memcpy(AttributsEnvironnement,cl->lattributs,sizeof(ATTRIBUT));
		  	}else AttributsEnvironnement = NULL;		
		
		PATT local_fin_env = enrichissement_att_environnement(cl->name_parent,lm->lparams);
	    writecode_exp(lm->expression, local_fin_env );
		desenrichissement_att_environnement(local_fin_env);

		
	    lm->label = label;
	    writecode("STOREL -");
	    int c = count_params(lm);	
	    writecodeiln(2+c);	
	    writecodeln("RETURN");
	    lm = lm->suiv;	
	}
	
	current_class_name = NULL;
	parent_current_class_name = NULL;
}

void writecode_exp(PARBRE arbre, PATT super_fin_env) {
	char * typeg = NULL;
	char * typed = NULL;
	PATT local_fin_env = NULL;
	char * id = arbre->gauche.S;

	if(arbre)
	switch (arbre->op)
	{

		case New : {
				writecodeln("--NEW");
				
				int c=0;
				writecode("ALLOC ");
				PCLASSE lc = get_class(arbre->gauche.A->gauche.S);
				while(lc){
					PATT att = lc->lattributs;
					while(att){
						c++;
						att = att->suiv;
					}
					lc = get_class(lc->name_parent);
				}
				writecodeiln(1+c);
				writecodeln("DUPN 1"); 
				writecode("PUSHG ");
				writecodeiln( (get_class(arbre->gauche.A->gauche.S))->index );
				writecodeln("STORE 0"); writecodeln("--NEWEND");
				}
				break;
				
		case Bloc : { writecodeln("--BLOC");
				local_fin_env = enrichissement_att_environnement(NULL,arbre->droit.lattributs);

				//Allouer les variables
				PATT att = arbre->droit.lattributs;
				int c = 0;
				while(att){
					c++;
					att = att->suiv;
				}
				writecode("PUSHN ");
				writecodeiln(c);
				writecode_exp( arbre->gauche.A, local_fin_env );
				int i;
				for(i=0;i<c;i++){
					writecodeln("SWAP");
					writecodeln("POPN 1");
				}
				desenrichissement_att_environnement(local_fin_env);
				writecodeln("--BLOCEND");
				} 
				break;

		case Self :
			writecodeln("PUSHL -1");
			//return current_class_name;
			break;
				
		case Id : {
			int index = get_var_index( super_fin_env, id );
			if( current_method == NULL ){ //Main
				writecode("PUSHL ");
				writecodeiln(index + count_classes(definedClasses));
			}else{ 
				if( index > -1 ){
					writecode("PUSHL ");
					writecodeiln(index);				
				}else{
					int indexparam = index_param( current_method , id );
					int n = count_params( current_method );
					if( indexparam ){
						writecode("PUSHL -");
						writecodeiln( n + 2 - indexparam );
					}else{
						int indexatt = index_att( current_class_name , id );
						if( indexatt >=0 ){
							writecodeln("PUSHL -1");
							writecode("LOAD ");
							writecodeiln(indexatt + 1);
						}
					}
				
				}
				
			}
			
			}
			break;
        
		case Fct :{ writecodeln("--APPEL");
			writecodeln("PUSHN 1"); //Pour le retour de la fonction
			
			//Met self au dessus de la pile
			writecodeln("PUSHL -1");
			
			PFONC f = arbre->gauche.F;
			PARG arg = f->largs;
			int c = 0;
			while(arg){
				writecode_exp( arg->expression, super_fin_env );
				writecodeln("SWAP");
				c++;
				arg = arg->suiv;
			}

			writecodeln("DUPN 1");
			writecodeln("LOAD 0");	
			writecode("LOAD ");
			writecodeiln( (get_meth_index( check_type(arbre->gauche.A , NULL, super_fin_env), f->name) ) );
			writecodeln("CALL");
			writecode("POPN ");
			writecodeiln( c + 1 ); //Dépile le destinataire et les paramètres
			writecodeln("--APPELEND");	
		} break;
		
		case Aff: { writecodeln("--AFF");
			writecode_exp(arbre->droit.A, super_fin_env);
			writecodeln("DUPN 1");
			if( arbre->gauche.A->op == '.' ){ //C'est le champ d'un objet
				writecode_exp(arbre->gauche.A->gauche.A, super_fin_env);
				int indexatt = index_att( check_type( arbre->gauche.A->gauche.A, NULL, super_fin_env ) ,
								arbre->gauche.A->droit.S );
				writecodeln("SWAP");
				writecode("STORE ");	
				writecodeiln(indexatt + 2);
			}else { //C'est un id
				
				int index = get_var_index( super_fin_env, arbre->gauche.A->gauche.S );
				if( current_method == NULL ){ //Main
					writecode("STOREL ");
					writecodeiln(index + count_classes(definedClasses));
				}else{
					if( index > -1 ){
						writecode("STOREL ");
						writecodeiln(index);				
					}else{
						int indexparam = index_param( current_method , arbre->gauche.A->gauche.S );
						if( indexparam ){
							int n = count_params( current_method );
							writecode("STOREL -");
							writecodeiln( n + 2 - indexparam );					
						}else{ 
							int indexatt = index_att( current_class_name , arbre->gauche.A->gauche.S );
							if( indexatt >= 0 ){
								writecodeln("PUSHL -1");
								writecodeln("SWAP");
								writecode("STORE ");
								writecodeiln(indexatt + 1);
							}
						}

					}

				}				
			}writecodeln("--AFFEND");
		}

		break;
			
		case ';':
		
			if( arbre->droit.A != NULL ){
				writecode_exp( arbre->gauche.A, super_fin_env );
				writecodeln("POPN 1");
				writecode_exp( arbre->droit.A, super_fin_env );
			}
			break;
			
		case '.' : {
			
			if( arbre->droit.A->op == Id ){	
				writecode_exp( arbre->gauche.A, super_fin_env );
				int index = index_att( check_type( arbre->gauche.A, NULL, super_fin_env ) , arbre->droit.A->gauche.S );
				writecode("LOAD ");
				writecodeiln(index + 2);
				
			}else if( arbre->droit.A->op == Fct ){
				int imprimer = 0;
				PFONC f = arbre->droit.A->gauche.F;
				if( strcmp(f->name,"imprimer")==0 ){
					if( strcmp(check_type( arbre->gauche.A, NULL, super_fin_env ),"Entier")==0 ){
						writecode_exp( arbre->gauche.A, super_fin_env );
						writecodeln("DUPN 1");
						writecodeln("WRITEI");
						imprimer=1;
					}
					if( strcmp(check_type( arbre->gauche.A, NULL, super_fin_env ),"Chaine")==0  ){
						writecode_exp( arbre->gauche.A, super_fin_env );
						writecodeln("DUPN 1");
						writecodeln("WRITES");
						imprimer=1;
					}
				}

				if( !imprimer && arbre->gauche.A->op == Super ){ writecodeln("--APPEL");
						writecodeln("PUSHN 1"); //Pour le retour de la fonction

						PARG arg = f->largs;
						int c = 0;
						while(arg){
							writecode_exp( arg->expression, super_fin_env );
							c++;
							arg = arg->suiv;
						}

						writecodeln("PUSHL -1");
						
						int index = get_class(parent_current_class_name)->index;
						writecode("PUSHG ");
						writecodeiln(index);	
						writecode("LOAD ");
						writecodeiln( (get_meth_index( check_type(arbre->gauche.A, NULL ,super_fin_env), f->name) ) );
						writecodeln("CALL");
						writecode("POPN ");
						writecodeiln( c + 1 ); //Dépile le destinataire et les paramètres
						writecodeln("--APPELEND");
				}else if(!imprimer){		
					writecodeln("--APPEL");	
					writecodeln("PUSHN 1"); //Pour le retour de la fonction
					writecode_exp( arbre->gauche.A, super_fin_env );
				
					PFONC f = arbre->droit.A->gauche.F;
					PARG arg = f->largs;
					int c = 0;
					while(arg){
						writecode_exp( arg->expression, super_fin_env );
						writecodeln("SWAP");
						c++;
						arg = arg->suiv;
					}

					writecodeln("DUPN 1");
					writecodeln("LOAD 0");	
					writecode("LOAD ");
					writecodeiln( (get_meth_index( check_type(arbre->gauche.A, NULL, super_fin_env), f->name) ) );
					writecodeln("CALL");
					writecode("POPN ");
					writecodeiln( c + 1 ); //Dépile le destinataire et les paramètres
					writecodeln("--APPELEND");
				}
			}
			}
			break;
			
		case Cste:	
			writecode("PUSHI ");
			writecodeiln(arbre->gauche.E);
			break;
			
		case String:
			writecode("PUSHS ");
			writecodeln(arbre->gauche.S);
			break;
		
 		case '+':
			writecode_exp(arbre->gauche.A, super_fin_env);
			writecode_exp(arbre->droit.A, super_fin_env);
			writecodeln("ADD");
			break;
		    
		 case '-': 			
			writecode_exp(arbre->gauche.A, super_fin_env);
			writecode_exp(arbre->droit.A, super_fin_env);
			writecodeln("SUB");
			break;
			
		 case '*':
			writecode_exp(arbre->gauche.A, super_fin_env);
			writecode_exp(arbre->droit.A, super_fin_env);
			writecodeln("MUL");
			break;
		
		 case '/':					
			writecode_exp(arbre->gauche.A, super_fin_env);
			writecode_exp(arbre->droit.A, super_fin_env);
			writecodeln("DIV");
			break;

		case ITE :	
			writecode_exp(arbre->gauche.A, super_fin_env);
			int lblelse = newlbl();
			int lblend = newlbl();
			writecode("JZ ");
			writecodeln(lbl(lblelse));
			writecode_exp( arbre->droit.A->gauche.A, super_fin_env );
			writecode("JUMP ");
			writecodeln(lbl(lblend));
			writecode(lbl(lblelse));
			writecodeln(": NOP");
			writecode_exp( arbre->droit.A->droit.A, super_fin_env );
			writecode(lbl(lblend));
			writecodeln(": NOP");
			break;
			
		case LT :	
			writecode_exp(arbre->gauche.A, super_fin_env);
			writecode_exp(arbre->droit.A, super_fin_env);
			writecodeln("INF");
			break;

		case LE :
			writecode_exp(arbre->gauche.A, super_fin_env);
			writecode_exp(arbre->droit.A, super_fin_env);
			writecodeln("INFEQ");
			break;

		case GT :
			writecode_exp(arbre->gauche.A, super_fin_env);
			writecode_exp(arbre->droit.A, super_fin_env);
			writecodeln("SUP");
			break;

		case GE :
			writecode_exp(arbre->gauche.A, super_fin_env);
			writecode_exp(arbre->droit.A, super_fin_env);
			writecodeln("SUPEQ");
			break;

		case EQ :
			writecode_exp(arbre->gauche.A, super_fin_env);
			writecode_exp(arbre->droit.A, super_fin_env);
			writecodeln("EQUAL");
			break;

		case NEQ :
			writecode_exp(arbre->gauche.A, super_fin_env);
			writecode_exp(arbre->droit.A, super_fin_env);
			writecodeln("EQUAL");
			writecodeln("NOT");
			break;
	}
}

int count_classes(PCLASSE lc){
	if(lc == NULL) return 0;
	return 1 + count_classes(lc->suiv);
}

int count_methods(char* classname){
	int c = 0;
	PCLASSE lc = get_class(classname);
	while(lc){
		PMETH lm = lc->lmethodes;
		while(lm){
			int index = get_meth_index( lc->name_parent, lm->name );
			if(index==-1) c++;
			lm = lm->suiv;
		}
		lc = get_class(lc->name_parent);
	}
	return c;
}

void writecode_vtables(){
	PCLASSE lc = definedClasses;
	while(lc){
		
		int n = count_methods(lc->name);
		writecode("ALLOC ");
		writecodeiln(n);
		
		int written[n];
		int i;
		for(i=0;i<n;i++) written[i]=0;
		
		PCLASSE lc2 = lc;
		while(lc2){
			PMETH lm = lc2->lmethodes;
			while(lm){
				int index = get_meth_index(lc->name,lm->name);
				if(!written[index]){
					writecodeln("DUPN 1");
					writecode("PUSHA ");
					writecodeln(lbl(lm->label));
					writecode("STORE ");
					writecodeiln(index);
					written[index] = 1;
				}
				lm = lm->suiv;
			}
			lc2 = get_class(lc2->name_parent);
		}
		lc = lc->suiv;
	}	
}
