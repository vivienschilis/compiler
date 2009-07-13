echo " ";
echo "Execution du jeu de test non conforme pour la Grammaire";
echo " ";echo " ";
for i in `ls testsFailGram`; do echo $i; ./Projet "testsFailGram/$i"; done;
echo " ";echo " ";echo " ";
echo "Execution du jeu de test conforme pour la vérification";
echo " ";echo " ";
for i in `ls testsOKVerif/`; do echo $i; ./Projet "testsOKVerif/$i"; done;
echo " ";echo " ";echo " ";
echo "Execution du jeu de test non conforme pour la vérification";echo " ";echo " ";
for i in `ls testsFailVerif/`;do echo $i; ./Projet "testsFailVerif/$i"; echo " "; done

