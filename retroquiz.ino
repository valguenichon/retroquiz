// ============================================================
//  3615 RETROQUIZ
//  Plateforme : Minimit (ESP32) + Minitel physique
//  Librairie  : Minitel1B_Hard (version Minimit)
//  Stockage   : LittleFS
// ============================================================

#include <Minitel1B_Hard.h>
#include "donnees.h"

// ============================================================
// CONFIGURATION MATERIELLE
// ============================================================
Minitel minitel(Serial2); // Le Minimit utilise toujours Serial2

// ============================================================
// MACHINE A ETATS
// ============================================================
enum Ecran {
    ACCUEIL,
    JEU,
    CONFIRMATION_ABANDON,
    RESULTATS,
    LEADERBOARD,
    ADMIN_MDP,
    ADMIN_MENU,
    ADMIN_AJOUT_ENONCE,
    ADMIN_AJOUT_CHOIX,
    ADMIN_AJOUT_REPONSE,
    ADMIN_LISTE_QUESTIONS,
    ADMIN_CONFIRM_RESET
};

Ecran ecranCourant = ACCUEIL;

// ============================================================
// ETAT DU JEU
// ============================================================
Question    banqueQuestions[MAX_BANQUE_QUESTIONS];
int         totalBanque        = 0;
int         indicesPartie[NB_QUESTIONS_PARTIE];
int         questionEnCours    = 0;  // 0..9
int         scoreJoueur        = 0;
char        reponseSelectionnee = 0; // 'A'..'D' ou 0 si rien
char        pseudoJoueur[MAX_PSEUDO + 1] = "";

// Etat admin
char        mdpSaisi[8]        = "";
int         mdpPos             = 0;
Question    nouvelleQuestion;
int         etapeAjout         = 0; // 0=enonce, 1=choix, 2=reponse
int         champChoixActif    = 0; // 0..3 pour les 4 choix
String      champEnCours       = "";

// ============================================================
// UTILITAIRES D'AFFICHAGE
// ============================================================

// Centre un texte dans 40 colonnes
String centrer(const String& texte, int largeur = 40) {
    int padding = (largeur - texte.length()) / 2;
    if (padding < 0) padding = 0;
    String result = "";
    for (int i = 0; i < padding; i++) result += " ";
    result += texte;
    return result;
}

// Affiche une ligne horizontale de separateurs
void ligneHorizontale(char c = '-', int largeur = 40) {
    String ligne = "";
    for (int i = 0; i < largeur; i++) ligne += c;
    minitel.println(ligne);
}

// Bandeau titre en haut de chaque ecran
void bandeauTitre(const String& titre, int score = -1) {
    minitel.moveCursorXY(1, 1);
    minitel.attributs(FOND_BLEU);
    minitel.attributs(CARACTERE_JAUNE);
    String ligne = "3615 RETROQUIZ";
    if (score >= 0) {
        char compteur[10];
        sprintf(compteur, "[%02d/10]", score);
        // Remplir jusqu'a la colonne 34 puis afficher le compteur
        while ((int)ligne.length() < 33) ligne += " ";
        ligne += compteur;
    } else {
        while ((int)ligne.length() < 40) ligne += " ";
    }
    minitel.print(ligne);
    minitel.attributs(FOND_NOIR);
    minitel.attributs(CARACTERE_BLANC);
}

// Pied de page (ligne 23-24)
void piedPage(const String& ligne1, const String& ligne2 = "") {
    minitel.moveCursorXY(1, 23);
    ligneHorizontale();
    minitel.println(ligne1);
    if (ligne2.length() > 0) minitel.print(ligne2);
}

// Efface une zone de saisie et repositionne le curseur
void effacerChamp(int x, int y, int longueur) {
    minitel.moveCursorXY(x, y);
    for (int i = 0; i < longueur; i++) minitel.print(" ");
    minitel.moveCursorXY(x, y);
}

// Saisie d'une chaine avec echo (retourne la chaine finale)
// La touche CORRECTION efface le dernier caractere.
// La touche ENVOI ou SUITE termine la saisie.
// Retourne le code de la touche de fin (ENVOI, SUITE, SOMMAIRE...)
unsigned long saisirChaine(int x, int y, int maxLen, char* buffer,
                            bool masquer = false) {
    int pos = 0;
    buffer[0] = '\0';
    minitel.moveCursorXY(x, y);
    minitel.cursor();

    while (true) {
        unsigned long touche = minitel.getKeyCode();

        if (touche == CORRECTION) {
            if (pos > 0) {
                pos--;
                buffer[pos] = '\0';
                minitel.moveCursorXY(x + pos, y);
                minitel.print(" ");
                minitel.moveCursorXY(x + pos, y);
            }
        } else if (touche == ENVOI || touche == SUITE ||
                   touche == SOMMAIRE || touche == RETOUR) {
            minitel.noCursor();
            return touche;
        } else if (touche >= 0x20 && touche <= 0x7E && pos < maxLen) {
            // Caractere imprimable
            buffer[pos] = (char)touche;
            pos++;
            buffer[pos] = '\0';
            if (masquer) minitel.print("*");
            else         minitel.print(String((char)touche));
        }
    }
}

// ============================================================
// ECRAN : ACCUEIL
// ============================================================
void afficherAccueil() {
    minitel.newScreen();
    bandeauTitre("3615 RETROQUIZ");

    // Titre graphique (double hauteur)
    minitel.moveCursorXY(1, 3);
    minitel.attributs(DOUBLE_HAUTEUR);
    minitel.attributs(CARACTERE_CYAN);
    minitel.println(centrer("3615 RETROQUIZ"));
    minitel.attributs(CARACTERE_BLANC);

    // Sous-titre
    minitel.moveCursorXY(1, 6);
    minitel.println(centrer("Le quiz du Minitel !"));

    // Separateur mosaique
    minitel.moveCursorXY(1, 8);
    ligneHorizontale('=');

    // Menu
    minitel.moveCursorXY(1, 11);
    minitel.attributs(CARACTERE_VERT);
    minitel.println("  [ 1 ]  JOUER AU QUIZ");
    minitel.moveCursorXY(1, 13);
    minitel.println("  [ 2 ]  LEADERBOARD");
    minitel.attributs(CARACTERE_BLANC);

    piedPage("  Tapez votre choix puis [ENVOI]",
             "  (Touche 9 : acces admin)");

    ecranCourant = ACCUEIL;
}

void gererAccueil(unsigned long touche) {
    if (touche == '1') {
        demarrerPartie();
    } else if (touche == '2') {
        afficherLeaderboard();
    } else if (touche == '9') {
        afficherAdminMdp();
    }
}

// ============================================================
// ECRAN : JEU
// ============================================================
void demarrerPartie() {
    totalBanque = chargerQuestions(banqueQuestions);
    if (totalBanque < NB_QUESTIONS_PARTIE) {
        // Pas assez de questions
        minitel.newScreen();
        bandeauTitre("3615 RETROQUIZ");
        minitel.moveCursorXY(1, 10);
        minitel.attributs(CARACTERE_ROUGE);
        char msg[50];
        sprintf(msg, "  ERREUR : il faut au moins %d questions !", NB_QUESTIONS_PARTIE);
        minitel.println(msg);
        minitel.attributs(CARACTERE_BLANC);
        minitel.moveCursorXY(1, 12);
        minitel.println("  Allez en mode ADMIN pour en ajouter.");
        piedPage("  [SOMMAIRE] Retour a l'accueil");
        ecranCourant = LEADERBOARD; // On reutilise SOMMAIRE -> accueil
        return;
    }

    tirerQuestions(banqueQuestions, totalBanque, indicesPartie);
    questionEnCours = 0;
    scoreJoueur     = 0;
    afficherQuestion();
}

void afficherQuestion() {
    int idxQ = indicesPartie[questionEnCours];
    Question& q = banqueQuestions[idxQ];
    reponseSelectionnee = 0;

    minitel.newScreen();
    bandeauTitre("3615 RETROQUIZ", questionEnCours + 1);

    // Enonce
    minitel.moveCursorXY(1, 3);
    minitel.attributs(CARACTERE_CYAN);
    minitel.println(q.enonce);
    minitel.attributs(CARACTERE_BLANC);

    // Separateur
    minitel.moveCursorXY(1, 7);
    ligneHorizontale('-');

    // Choix A/B/C/D
    const char lettres[] = {'A', 'B', 'C', 'D'};
    for (int i = 0; i < 4; i++) {
        minitel.moveCursorXY(1, 9 + i * 2);
        char ligne[44];
        sprintf(ligne, "  %c/ ", lettres[i]);
        minitel.print(ligne);
        minitel.println(q.choix[i]);
    }

    // Zone de reponse
    minitel.moveCursorXY(1, 19);
    minitel.print("  Votre reponse (A, B, C ou D) : ");
    minitel.moveCursorXY(35, 19);
    minitel.print("[_]");

    piedPage("  [SUITE] Valider    [SOMMAIRE] Abandonner");

    ecranCourant = JEU;
}

void gererJeu(unsigned long touche) {
    if (touche == 'A' || touche == 'a' ||
        touche == 'B' || touche == 'b' ||
        touche == 'C' || touche == 'c' ||
        touche == 'D' || touche == 'd') {
        // Mise a jour de la reponse selectionnee
        reponseSelectionnee = toupper((char)touche);
        minitel.moveCursorXY(36, 19);
        minitel.attributs(CARACTERE_JAUNE);
        minitel.print(String(reponseSelectionnee));
        minitel.attributs(CARACTERE_BLANC);

    } else if (touche == SUITE) {
        if (reponseSelectionnee == 0) {
            // Aucune reponse selectionnee
            minitel.moveCursorXY(1, 21);
            minitel.attributs(CARACTERE_ROUGE);
            minitel.print("  Selectionnez une reponse (A/B/C/D)");
            minitel.attributs(CARACTERE_BLANC);
            return;
        }
        // Verifier la reponse
        int idxQ = indicesPartie[questionEnCours];
        if (reponseSelectionnee == banqueQuestions[idxQ].bonneReponse) {
            scoreJoueur++;
            minitel.moveCursorXY(1, 21);
            minitel.attributs(CARACTERE_VERT);
            minitel.print("  BONNE REPONSE !");
            minitel.attributs(CARACTERE_BLANC);
            minitel.bip();
        } else {
            minitel.moveCursorXY(1, 21);
            minitel.attributs(CARACTERE_ROUGE);
            char msg[42];
            sprintf(msg, "  FAUX ! Reponse : %c",
                    banqueQuestions[idxQ].bonneReponse);
            minitel.print(msg);
            minitel.attributs(CARACTERE_BLANC);
            minitel.bip();
        }
        delay(1200); // Laisser le joueur lire le feedback

        questionEnCours++;
        if (questionEnCours >= NB_QUESTIONS_PARTIE) {
            afficherResultats();
        } else {
            afficherQuestion();
        }

    } else if (touche == SOMMAIRE) {
        afficherConfirmationAbandon();
    }
}

// ============================================================
// ECRAN : CONFIRMATION ABANDON
// ============================================================
void afficherConfirmationAbandon() {
    minitel.moveCursorXY(1, 10);
    ligneHorizontale('=');
    minitel.moveCursorXY(1, 11);
    minitel.attributs(FOND_ROUGE);
    minitel.attributs(CARACTERE_BLANC);
    minitel.println(centrer("QUITTER LA PARTIE ?"));
    minitel.moveCursorXY(1, 12);
    minitel.println(centrer("[ENVOI] OUI     [RETOUR] NON"));
    minitel.moveCursorXY(1, 13);
    ligneHorizontale('=');
    minitel.attributs(FOND_NOIR);
    ecranCourant = CONFIRMATION_ABANDON;
}

void gererConfirmationAbandon(unsigned long touche) {
    if (touche == ENVOI) {
        afficherAccueil();
    } else if (touche == RETOUR) {
        // Reprendre la question en cours
        afficherQuestion();
    }
}

// ============================================================
// ECRAN : RESULTATS
// ============================================================
void afficherResultats() {
    minitel.newScreen();
    bandeauTitre("3615 RETROQUIZ");

    // Titre graphique
    minitel.moveCursorXY(1, 3);
    minitel.attributs(DOUBLE_HAUTEUR);
    minitel.attributs(CARACTERE_JAUNE);
    minitel.println(centrer("** BRAVO ! **"));
    minitel.attributs(CARACTERE_BLANC);

    // Score
    minitel.moveCursorXY(1, 7);
    minitel.println(centrer("VOUS AVEZ REUSSI"));
    char scoreMsg[20];
    sprintf(scoreMsg, "%d / %d", scoreJoueur, NB_QUESTIONS_PARTIE);
    minitel.moveCursorXY(1, 8);
    minitel.attributs(CARACTERE_CYAN);
    minitel.println(centrer(scoreMsg));
    minitel.attributs(CARACTERE_BLANC);
    minitel.moveCursorXY(1, 9);
    minitel.println(centrer("BONNES REPONSES"));

    // Zone saisie pseudo
    minitel.moveCursorXY(1, 12);
    ligneHorizontale('-');
    minitel.moveCursorXY(1, 13);
    minitel.println("  ENTREZ VOTRE PSEUDO POUR LE");
    minitel.moveCursorXY(1, 14);
    minitel.println("  CLASSEMENT (8 car. max) :");
    minitel.moveCursorXY(1, 16);
    minitel.print("  > [");
    // Position curseur : colonne 6, ligne 16
    minitel.moveCursorXY(35, 16);
    minitel.print("]");

    piedPage("  Saisissez votre pseudo puis [SUITE]");

    // Lancer la saisie directement
    memset(pseudoJoueur, 0, sizeof(pseudoJoueur));
    unsigned long fin = saisirChaine(6, 16, MAX_PSEUDO, pseudoJoueur);

    if (fin == SUITE || fin == ENVOI) {
        if (strlen(pseudoJoueur) == 0) strcpy(pseudoJoueur, "ANONYME");
        // Enregistrer le score
        Score leaderboard[MAX_SCORES];
        chargerLeaderboard(leaderboard);
        insererScore(leaderboard, pseudoJoueur, scoreJoueur);
        afficherLeaderboard();
    } else {
        afficherAccueil();
    }
}

// ============================================================
// ECRAN : LEADERBOARD
// ============================================================
void afficherLeaderboard() {
    Score leaderboard[MAX_SCORES];
    chargerLeaderboard(leaderboard);

    minitel.newScreen();
    bandeauTitre("3615 RETROQUIZ");

    // Titre graphique
    minitel.moveCursorXY(1, 2);
    minitel.attributs(DOUBLE_HAUTEUR);
    minitel.attributs(CARACTERE_JAUNE);
    minitel.println(centrer("** TOP 10 **"));
    minitel.attributs(CARACTERE_BLANC);

    // En-tete tableau
    minitel.moveCursorXY(1, 5);
    minitel.attributs(FOND_BLEU);
    minitel.print("  RANG  PSEUDO      SCORE     ");
    minitel.attributs(FOND_NOIR);

    // Scores
    for (int i = 0; i < MAX_SCORES; i++) {
        minitel.moveCursorXY(1, 7 + i);
        char ligne[41];

        if (i == 0) minitel.attributs(CARACTERE_JAUNE);      // 1er : or
        else if (i <= 2) minitel.attributs(CARACTERE_CYAN);  // Podium : cyan
        else minitel.attributs(CARACTERE_BLANC);

        sprintf(ligne, "   %2d.  %-8s    %2d / %d",
                i + 1,
                leaderboard[i].pseudo,
                leaderboard[i].points,
                NB_QUESTIONS_PARTIE);
        minitel.println(ligne);
    }
    minitel.attributs(CARACTERE_BLANC);

    piedPage("  [SOMMAIRE] Retour a l'accueil");
    ecranCourant = LEADERBOARD;
}

void gererLeaderboard(unsigned long touche) {
    if (touche == SOMMAIRE) {
        afficherAccueil();
    }
}

// ============================================================
// ECRAN : ADMIN - SAISIE MOT DE PASSE
// ============================================================
void afficherAdminMdp() {
    minitel.newScreen();
    bandeauTitre("3615 RETROQUIZ");

    minitel.moveCursorXY(1, 5);
    ligneHorizontale('=');
    minitel.moveCursorXY(1, 6);
    minitel.println(centrer("ACCES ADMINISTRATION"));
    minitel.moveCursorXY(1, 7);
    ligneHorizontale('=');

    minitel.moveCursorXY(1, 11);
    minitel.print("  CODE D'ACCES : [");
    // Position : colonne 19, ligne 11 (apres le crochet)
    minitel.moveCursorXY(37, 11);
    minitel.print("]");

    piedPage("  [ENVOI] Valider    [SOMMAIRE] Annuler");

    memset(mdpSaisi, 0, sizeof(mdpSaisi));
    // Saisie masquee (affiche des *)
    unsigned long fin = saisirChaine(19, 11, 6, mdpSaisi, true);

    if (fin == ENVOI || fin == SUITE) {
        if (strcmp(mdpSaisi, MDP_ADMIN) == 0) {
            afficherAdminMenu();
        } else {
            // Mauvais mot de passe
            minitel.moveCursorXY(1, 14);
            minitel.attributs(CARACTERE_ROUGE);
            minitel.println(centrer("CODE INCORRECT !"));
            minitel.attributs(CARACTERE_BLANC);
            delay(1500);
            afficherAccueil();
        }
    } else {
        afficherAccueil();
    }
}

// ============================================================
// ECRAN : ADMIN - MENU PRINCIPAL
// ============================================================
void afficherAdminMenu() {
    minitel.newScreen();
    bandeauTitre("3615 RETROQUIZ");

    minitel.moveCursorXY(1, 3);
    ligneHorizontale('=');
    minitel.moveCursorXY(1, 4);
    minitel.println(centrer("PANNEAU DE CONTROLE"));
    minitel.moveCursorXY(1, 5);
    ligneHorizontale('=');

    minitel.moveCursorXY(1, 8);
    minitel.attributs(CARACTERE_VERT);
    minitel.println("  [ 1 ]  REINITIALISER LE LEADERBOARD");
    minitel.moveCursorXY(1, 10);
    minitel.println("  [ 2 ]  AJOUTER UNE QUESTION");
    minitel.moveCursorXY(1, 12);
    minitel.println("  [ 3 ]  VOIR / MODIFIER LES QUESTIONS");
    minitel.moveCursorXY(1, 14);
    minitel.attributs(CARACTERE_ROUGE);
    minitel.println("  [ 4 ]  RETOUR A L'ACCUEIL");
    minitel.attributs(CARACTERE_BLANC);

    minitel.moveCursorXY(1, 17);
    minitel.print("  VOTRE CHOIX : [_]");

    piedPage("  Tapez 1, 2, 3 ou 4 puis [ENVOI]");
    ecranCourant = ADMIN_MENU;
}

void gererAdminMenu(unsigned long touche) {
    if (touche == '1') {
        afficherAdminConfirmReset();
    } else if (touche == '2') {
        afficherAdminAjoutEnonce();
    } else if (touche == '3') {
        afficherAdminListeQuestions();
    } else if (touche == '4') {
        afficherAccueil();
    }
}

// ============================================================
// ECRAN : ADMIN - CONFIRMATION RESET LEADERBOARD
// ============================================================
void afficherAdminConfirmReset() {
    minitel.newScreen();
    bandeauTitre("3615 RETROQUIZ");

    minitel.moveCursorXY(1, 8);
    minitel.attributs(FOND_ROUGE);
    minitel.attributs(CARACTERE_BLANC);
    minitel.println(centrer("ETES-VOUS SUR ?"));
    minitel.moveCursorXY(1, 9);
    minitel.println(centrer("Cette action est irreversible !"));
    minitel.attributs(FOND_NOIR);
    minitel.moveCursorXY(1, 12);
    minitel.println(centrer("[ENVOI] CONFIRMER   [RETOUR] ANNULER"));

    ecranCourant = ADMIN_CONFIRM_RESET;
}

void gererAdminConfirmReset(unsigned long touche) {
    if (touche == ENVOI) {
        reinitialiserLeaderboard();
        minitel.moveCursorXY(1, 15);
        minitel.attributs(CARACTERE_VERT);
        minitel.println(centrer("LEADERBOARD REINITIALISE !"));
        minitel.attributs(CARACTERE_BLANC);
        delay(1500);
        afficherAdminMenu();
    } else if (touche == RETOUR) {
        afficherAdminMenu();
    }
}

// ============================================================
// ECRAN : ADMIN - AJOUT QUESTION (Etape 1/3 : Enonce)
// ============================================================
void afficherAdminAjoutEnonce() {
    nouvelleQuestion.enonce   = "";
    nouvelleQuestion.bonneReponse = 'A';
    for (int i = 0; i < 4; i++) nouvelleQuestion.choix[i] = "";
    etapeAjout = 0;

    minitel.newScreen();
    bandeauTitre("3615 RETROQUIZ");

    minitel.moveCursorXY(1, 3);
    minitel.println(centrer("AJOUT D'UNE QUESTION (1/3)"));
    minitel.moveCursorXY(1, 4);
    ligneHorizontale('-');

    minitel.moveCursorXY(1, 6);
    minitel.println("  Saisissez l'enonce (2 lignes max) :");
    minitel.moveCursorXY(1, 8);
    minitel.print("  > ");
    // Zone de saisie : 36 caracteres, colonne 5, ligne 8
    for (int i = 0; i < 35; i++) minitel.print("_");

    piedPage("  [SUITE] Suivant    [SOMMAIRE] Annuler");
    ecranCourant = ADMIN_AJOUT_ENONCE;

    // Saisie immediate
    char buffer[80] = "";
    unsigned long fin = saisirChaine(5, 8, 35, buffer);
    if (fin == SOMMAIRE) { afficherAdminMenu(); return; }
    nouvelleQuestion.enonce = String(buffer);
    afficherAdminAjoutChoix();
}

// ============================================================
// ECRAN : ADMIN - AJOUT QUESTION (Etape 2/3 : Choix)
// ============================================================
void afficherAdminAjoutChoix() {
    minitel.newScreen();
    bandeauTitre("3615 RETROQUIZ");

    minitel.moveCursorXY(1, 3);
    minitel.println(centrer("AJOUT D'UNE QUESTION (2/3)"));
    minitel.moveCursorXY(1, 4);
    ligneHorizontale('-');

    minitel.moveCursorXY(1, 6);
    minitel.println("  Saisissez les 4 options :");

    const char lettres[] = {'A', 'B', 'C', 'D'};
    for (int i = 0; i < 4; i++) {
        minitel.moveCursorXY(1, 8 + i * 2);
        char label[6];
        sprintf(label, "  %c/ ", lettres[i]);
        minitel.print(label);
        for (int j = 0; j < 30; j++) minitel.print("_");
    }

    piedPage("  [SUITE] Suivant    [RETOUR] Etape prec.");
    ecranCourant = ADMIN_AJOUT_CHOIX;

    // Saisir les 4 options une par une
    for (int i = 0; i < 4; i++) {
        char buffer[32] = "";
        unsigned long fin = saisirChaine(6, 8 + i * 2, 30, buffer);
        if (fin == SOMMAIRE) { afficherAdminMenu(); return; }
        if (fin == RETOUR && i == 0) { afficherAdminAjoutEnonce(); return; }
        nouvelleQuestion.choix[i] = String(buffer);
    }
    afficherAdminAjoutReponse();
}

// ============================================================
// ECRAN : ADMIN - AJOUT QUESTION (Etape 3/3 : Bonne reponse)
// ============================================================
void afficherAdminAjoutReponse() {
    minitel.newScreen();
    bandeauTitre("3615 RETROQUIZ");

    minitel.moveCursorXY(1, 3);
    minitel.println(centrer("AJOUT D'UNE QUESTION (3/3)"));
    minitel.moveCursorXY(1, 4);
    ligneHorizontale('-');

    minitel.moveCursorXY(1, 7);
    minitel.println("  Quelle est la bonne reponse ?");
    minitel.moveCursorXY(1, 9);
    minitel.print("  Lettre (A, B, C ou D) : [_]");

    piedPage("  [ENVOI] SAUVEGARDER    [SOMMAIRE] Annuler");
    ecranCourant = ADMIN_AJOUT_REPONSE;

    char buffer[4] = "";
    unsigned long fin = saisirChaine(28, 9, 1, buffer);

    if (fin == SOMMAIRE) { afficherAdminMenu(); return; }

    char rep = toupper(buffer[0]);
    if (rep == 'A' || rep == 'B' || rep == 'C' || rep == 'D') {
        nouvelleQuestion.bonneReponse = rep;
    } else {
        nouvelleQuestion.bonneReponse = 'A';
    }

    // Sauvegarde
    if (sauvegarderNouvelleQuestion(nouvelleQuestion)) {
        minitel.moveCursorXY(1, 14);
        minitel.attributs(CARACTERE_VERT);
        minitel.println(centrer("QUESTION SAUVEGARDEE !"));
        minitel.attributs(CARACTERE_BLANC);
    } else {
        minitel.moveCursorXY(1, 14);
        minitel.attributs(CARACTERE_ROUGE);
        minitel.println(centrer("ERREUR DE SAUVEGARDE !"));
        minitel.attributs(CARACTERE_BLANC);
    }
    delay(1500);
    afficherAdminMenu();
}

// ============================================================
// ECRAN : ADMIN - LISTE DES QUESTIONS
// ============================================================
void afficherAdminListeQuestions() {
    Question banque[MAX_BANQUE_QUESTIONS];
    int total = chargerQuestions(banque);

    minitel.newScreen();
    bandeauTitre("3615 RETROQUIZ");

    minitel.moveCursorXY(1, 3);
    minitel.println(centrer("LISTE DES QUESTIONS EN BASE"));
    minitel.moveCursorXY(1, 4);
    ligneHorizontale('-');

    if (total == 0) {
        minitel.moveCursorXY(1, 8);
        minitel.println(centrer("Aucune question enregistree."));
        piedPage("  [SOMMAIRE] Retour au menu admin");
        ecranCourant = ADMIN_LISTE_QUESTIONS;
        return;
    }

    // Afficher les 14 premieres questions (limite ecran)
    int affichees = min(total, 14);
    for (int i = 0; i < affichees; i++) {
        minitel.moveCursorXY(1, 6 + i);
        char ligne[41];
        // Tronquer l'enonce a 34 chars
        String apercu = banque[i].enonce;
        if ((int)apercu.length() > 32) apercu = apercu.substring(0, 32) + "..";
        sprintf(ligne, " Q%02d: ", i + 1);
        minitel.print(ligne);
        minitel.println(apercu);
    }

    char totalMsg[30];
    sprintf(totalMsg, "  Total : %d question(s)", total);
    minitel.moveCursorXY(1, 21);
    minitel.println(totalMsg);

    piedPage("  [SOMMAIRE] Retour au menu admin");
    ecranCourant = ADMIN_LISTE_QUESTIONS;
}

void gererAdminListeQuestions(unsigned long touche) {
    if (touche == SOMMAIRE) {
        afficherAdminMenu();
    }
}

// ============================================================
// SETUP & LOOP
// ============================================================
void setup() {
    Serial2.begin(4800); // Vitesse du Minimit
    delay(500);

    minitel.pageMode();
    minitel.echo(false);   // Pas d'echo automatique (on gere nous-memes)
    minitel.noCursor();

    if (!initialiserStockage()) {
        // Erreur fatale LittleFS
        minitel.newScreen();
        minitel.print("ERREUR SYSTEME FICHIERS !");
        while (true) delay(1000);
    }

    afficherAccueil();
}

void loop() {
    unsigned long touche = minitel.getKeyCode();
    if (touche == 0) return; // Pas de touche

    switch (ecranCourant) {
        case ACCUEIL:               gererAccueil(touche);               break;
        case JEU:                   gererJeu(touche);                   break;
        case CONFIRMATION_ABANDON:  gererConfirmationAbandon(touche);   break;
        case LEADERBOARD:           gererLeaderboard(touche);           break;
        case ADMIN_MENU:            gererAdminMenu(touche);             break;
        case ADMIN_CONFIRM_RESET:   gererAdminConfirmReset(touche);     break;
        case ADMIN_LISTE_QUESTIONS: gererAdminListeQuestions(touche);   break;
        // Les ecrans de saisie (RESULTATS, ADMIN_MDP, ADMIN_AJOUT_*)
        // sont geres directement dans leur fonction d'affichage
        default: break;
    }
}
