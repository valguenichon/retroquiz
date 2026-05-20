// ============================================================
//  RETROQUIZ.INO
//  Service 3615 RETROQUIZ pour le Minimit
//  Patron : setupRetroquiz() + loopRetroquiz()
//  Pas de WiFi requis — fonctionne entierement hors ligne.
//  Variables globales Minimit utilisees :
//    touche, userInput, userInputLength, currentEcran
//  Fonctions Minimit utilisees :
//    wait_for_user_action(), champVide(), initMinitelService()
// ============================================================

#include "donnees_rq.h"

// ============================================================
// VARIABLES LOCALES AU SERVICE
// Prefixe rq_ pour eviter tout conflit avec les autres services
// ============================================================
static RQ_Question  rq_banque[RQ_MAX_BANQUE];
static int          rq_totalBanque     = 0;
static int          rq_indices[RQ_NB_QUESTIONS_PARTIE];
static int          rq_questionNum     = 0;   // 0..9
static int          rq_score           = 0;
static char         rq_reponse         = 0;   // 'A'..'D' ou 0
static char         rq_pseudo[RQ_MAX_PSEUDO + 1] = "";
static char         rq_mdpSaisi[8]     = "";
static int          rq_pageAdmin       = 0;
static RQ_Question  rq_editQ;                 // question en cours d'edition

// ============================================================
// UTILITAIRES AFFICHAGE
// ============================================================

// Positionne et efface une zone de saisie de longueur donnee
static void rq_champSaisie(int x, int y, int len) {
    minitel.newXY(x, y);
    for (int i = 0; i < len; i++) minitel.print(".");
    minitel.newXY(x, y);
    minitel.cursor();
}

// Affiche une chaine centree sur 40 colonnes a la position y
static void rq_centrer(const String& txt, int y) {
    int pad = (40 - (int)txt.length()) / 2;
    if (pad < 0) pad = 0;
    minitel.newXY(1 + pad, y);
    minitel.print(txt);
}

// Ligne horizontale de 40 caracteres
static void rq_ligneH(int y, char c = '-') {
    minitel.newXY(1, y);
    for (int i = 0; i < 40; i++) minitel.print(String(c));
}

// Bandeau haut commun a tous les ecrans du service
static void rq_bandeau(int numQ = 0) {
    minitel.newXY(1, 1);
    minitel.attributs(FOND_BLEU);
    minitel.attributs(CARACTERE_JAUNE);
    if (numQ > 0) {
        char buf[41];
        sprintf(buf, "3615 RETROQUIZ              [%02d/%02d]",
                numQ, RQ_NB_QUESTIONS_PARTIE);
        minitel.print(buf);
    } else {
        minitel.print("3615 RETROQUIZ                        ");
    }
    minitel.attributs(FOND_NOIR);
    minitel.attributs(CARACTERE_BLANC);
}

// Pied de page : ligne 23 = separateur, ligne 24 = aide touches
static void rq_pied(const String& aide) {
    rq_ligneH(23);
    minitel.newXY(1, 24);
    minitel.attributs(CARACTERE_CYAN);
    minitel.print(aide);
    minitel.attributs(CARACTERE_BLANC);
}

// Saisie bloquante d'une chaine (max maxLen caracteres).
// Gere CORRECTION et se termine sur ENVOI, SUITE ou SOMMAIRE.
// Retourne le code de la touche de fin.
// NB : n'utilise PAS wait_for_user_action() pour avoir l'echo masquable.
static unsigned long rq_saisir(int x, int y, int maxLen,
                                char* buf, bool masquer = false) {
    int pos = 0;
    buf[0] = '\0';
    minitel.newXY(x, y);
    minitel.cursor();
    minitel.echo(false);

    while (true) {
        unsigned long t = minitel.getKeyCode();
        if (t == CORRECTION) {
            if (pos > 0) {
                pos--;
                buf[pos] = '\0';
                minitel.moveCursorLeft(1);
                minitel.print(".");
                minitel.moveCursorLeft(1);
            }
        } else if (t == ENVOI || t == SUITE || t == SOMMAIRE || t == CONNEXION_FIN) {
            minitel.noCursor();
            minitel.echo(false);
            return t;
        } else if (t >= 0x20 && t <= 0x7E && pos < maxLen) {
            buf[pos++] = (char)t;
            buf[pos]   = '\0';
            minitel.print(masquer ? "*" : String((char)t));
        }
    }
}

// ============================================================
// ECRAN ACCUEIL DU SERVICE
// ============================================================
static void rq_afficheAccueil() {
    minitel.newScreen();
    rq_bandeau();
    currentEcran = "RQ_ACCUEIL";

    // Titre double hauteur
    minitel.newXY(1, 3);
    minitel.attributs(DOUBLE_HAUTEUR);
    minitel.attributs(CARACTERE_CYAN);
    rq_centrer("** 3615 RETROQUIZ **", 3);
    minitel.attributs(CARACTERE_BLANC);

    minitel.newXY(1, 6);
    rq_centrer("Le quiz du Minitel !", 6);
    rq_ligneH(8, '=');

    minitel.newXY(1, 11);
    minitel.attributs(CARACTERE_VERT);
    minitel.print("  [ 1 ]  JOUER AU QUIZ");
    minitel.newXY(1, 13);
    minitel.print("  [ 2 ]  LEADERBOARD");
    minitel.attributs(CARACTERE_BLANC);

    rq_pied("  1/2=Menu  9=Admin  CNX/FIN=Quitter");
    champVide(14, 17, 20);
}

// ============================================================
// ECRAN JEU
// ============================================================
static void rq_afficheQuestion() {
    RQ_Question& q = rq_banque[rq_indices[rq_questionNum]];
    rq_reponse = 0;

    minitel.newScreen();
    rq_bandeau(rq_questionNum + 1);
    currentEcran = "RQ_JEU";

    // Enonce
    minitel.newXY(1, 3);
    minitel.attributs(CARACTERE_CYAN);
    minitel.print("  " + q.enonce);
    minitel.attributs(CARACTERE_BLANC);

    rq_ligneH(7);

    // Choix A/B/C/D
    const char lettres[] = {'A','B','C','D'};
    for (int i = 0; i < 4; i++) {
        minitel.newXY(1, 9 + i * 2);
        char label[5];
        sprintf(label, "  %c/ ", lettres[i]);
        minitel.print(label);
        minitel.print(q.choix[i]);
    }

    // Zone reponse
    minitel.newXY(1, 19);
    minitel.print("  Votre reponse (A/B/C/D) : [");
    minitel.newXY(31, 19);
    minitel.print("]");

    rq_pied("  A/B/C/D=Choisir  SUITE=Valider  SOM=Quit");
    champVide(30, 19, 1);
}

static void rq_demarrerPartie() {
    rq_totalBanque = rq_chargerQuestions(rq_banque);
    if (rq_totalBanque < RQ_NB_QUESTIONS_PARTIE) {
        minitel.newScreen();
        rq_bandeau();
        minitel.newXY(1, 10);
        minitel.attributs(CARACTERE_ROUGE);
        char msg[41];
        sprintf(msg, "  Besoin de %d questions min (%d dispo).",
                RQ_NB_QUESTIONS_PARTIE, rq_totalBanque);
        minitel.print(msg);
        minitel.attributs(CARACTERE_BLANC);
        minitel.newXY(1, 12);
        minitel.print("  Ajoutez des questions via l'admin (9).");
        rq_pied("  CNX/FIN ou SOMMAIRE = retour");
        currentEcran = "RQ_ERREUR";
        return;
    }
    rq_tirerQuestions(rq_totalBanque, rq_indices);
    rq_questionNum = 0;
    rq_score       = 0;
    rq_afficheQuestion();
}

static void rq_validerReponse() {
    if (rq_reponse == 0) {
        // Aucune reponse selectionnee : clignoter la zone
        minitel.newXY(1, 21);
        minitel.attributs(CARACTERE_ROUGE);
        minitel.print("  Selectionnez A, B, C ou D !");
        minitel.attributs(CARACTERE_BLANC);
        return;
    }
    RQ_Question& q = rq_banque[rq_indices[rq_questionNum]];
    bool correct   = (rq_reponse == q.bonneReponse);

    // Feedback
    minitel.newXY(1, 21);
    if (correct) {
        rq_score++;
        minitel.attributs(CARACTERE_VERT);
        minitel.print("  BONNE REPONSE !                      ");
    } else {
        minitel.attributs(CARACTERE_ROUGE);
        char msg[41];
        sprintf(msg, "  FAUX ! Bonne reponse : %c             ", q.bonneReponse);
        minitel.print(msg);
    }
    minitel.attributs(CARACTERE_BLANC);
    minitel.bip();
    delay(1200);

    rq_questionNum++;
    if (rq_questionNum >= RQ_NB_QUESTIONS_PARTIE) {
        currentEcran = "RQ_RESULTATS";
    } else {
        rq_afficheQuestion();
    }
}

// ============================================================
// ECRAN RESULTATS + SAISIE PSEUDO
// ============================================================
static void rq_afficheResultats() {
    minitel.newScreen();
    rq_bandeau();
    currentEcran = "RQ_RESULTATS";

    minitel.newXY(1, 3);
    minitel.attributs(DOUBLE_HAUTEUR);
    minitel.attributs(CARACTERE_JAUNE);
    rq_centrer("** BRAVO ! **", 3);
    minitel.attributs(CARACTERE_BLANC);

    minitel.newXY(1, 7);
    rq_centrer("VOUS AVEZ REUSSI", 7);
    char sc[20];
    sprintf(sc, "%d / %d BONNES REPONSES", rq_score, RQ_NB_QUESTIONS_PARTIE);
    minitel.attributs(CARACTERE_CYAN);
    rq_centrer(String(sc), 8);
    minitel.attributs(CARACTERE_BLANC);

    rq_ligneH(11);
    minitel.newXY(1, 12);
    minitel.print("  Votre pseudo pour le classement :");
    minitel.newXY(1, 13);
    minitel.print("  (8 car. max, SUITE pour valider)");
    minitel.newXY(1, 15);
    minitel.print("  > [");
    minitel.newXY(35, 15);
    minitel.print("]");
    rq_pied("  Saisissez votre pseudo puis SUITE");

    // Saisie directe (bloquante)
    memset(rq_pseudo, 0, sizeof(rq_pseudo));
    unsigned long fin = rq_saisir(5, 15, RQ_MAX_PSEUDO, rq_pseudo);

    if (fin == CONNEXION_FIN) { currentEcran = "RQ_FIN"; return; }
    if (strlen(rq_pseudo) == 0) strcpy(rq_pseudo, "ANONYME");

    RQ_Score lb[RQ_MAX_SCORES];
    rq_chargerLeaderboard(lb);
    bool entre = rq_insererScore(lb, rq_pseudo, rq_score);

    // Feedback insertion
    minitel.newXY(1, 18);
    if (entre) {
        minitel.attributs(CARACTERE_VERT);
        char msg[41];
        sprintf(msg, "  %s entre dans le TOP 10 !", rq_pseudo);
        minitel.print(msg);
        minitel.attributs(CARACTERE_BLANC);
    }
    delay(1500);
    currentEcran = "RQ_LEADERBOARD";
}

// ============================================================
// ECRAN LEADERBOARD
// ============================================================
static void rq_afficheLeaderboard() {
    RQ_Score lb[RQ_MAX_SCORES];
    rq_chargerLeaderboard(lb);

    minitel.newScreen();
    rq_bandeau();
    currentEcran = "RQ_LEADERBOARD";

    minitel.newXY(1, 2);
    minitel.attributs(DOUBLE_HAUTEUR);
    minitel.attributs(CARACTERE_JAUNE);
    rq_centrer("** TOP 10 **", 2);
    minitel.attributs(CARACTERE_BLANC);

    // En-tete tableau
    minitel.newXY(1, 5);
    minitel.attributs(FOND_BLEU);
    minitel.print("   RANG  PSEUDO      SCORE          ");
    minitel.attributs(FOND_NOIR);

    for (int i = 0; i < RQ_MAX_SCORES; i++) {
        minitel.newXY(1, 7 + i);
        if (i == 0)       minitel.attributs(CARACTERE_JAUNE);
        else if (i <= 2)  minitel.attributs(CARACTERE_CYAN);
        else              minitel.attributs(CARACTERE_BLANC);

        char ligne[41];
        sprintf(ligne, "   %2d.  %-8s    %2d / %d",
                i + 1,
                lb[i].pseudo,
                lb[i].points,
                RQ_NB_QUESTIONS_PARTIE);
        minitel.print(ligne);
    }
    minitel.attributs(CARACTERE_BLANC);
    rq_pied("  SOMMAIRE ou CNX/FIN = retour accueil");
}

// ============================================================
// ECRAN ADMIN : MOT DE PASSE
// ============================================================
static void rq_afficheAdminMdp() {
    minitel.newScreen();
    rq_bandeau();
    currentEcran = "RQ_ADMIN_MDP";

    rq_ligneH(5, '=');
    rq_centrer("ACCES ADMINISTRATION", 6);
    rq_ligneH(7, '=');

    minitel.newXY(1, 11);
    minitel.print("  Code d'acces : [");
    minitel.newXY(37, 11);
    minitel.print("]");

    rq_pied("  SUITE=Valider  SOMMAIRE=Annuler");

    memset(rq_mdpSaisi, 0, sizeof(rq_mdpSaisi));
    unsigned long fin = rq_saisir(19, 11, 6, rq_mdpSaisi, true);

    if ((fin == SUITE || fin == ENVOI) && strcmp(rq_mdpSaisi, RQ_MDP_ADMIN) == 0) {
        currentEcran = "RQ_ADMIN_MENU";
    } else {
        if (fin != CONNEXION_FIN && fin != SOMMAIRE) {
            minitel.newXY(1, 14);
            minitel.attributs(CARACTERE_ROUGE);
            rq_centrer("CODE INCORRECT !", 14);
            minitel.attributs(CARACTERE_BLANC);
            delay(1500);
        }
        currentEcran = "RQ_ACCUEIL";
    }
}

// ============================================================
// ECRAN ADMIN : MENU
// ============================================================
static void rq_afficheAdminMenu() {
    minitel.newScreen();
    rq_bandeau();
    currentEcran = "RQ_ADMIN_MENU";

    rq_ligneH(3, '=');
    rq_centrer("PANNEAU DE CONTROLE", 4);
    rq_ligneH(5, '=');

    minitel.newXY(1, 8);
    minitel.attributs(CARACTERE_VERT);
    minitel.print("  [ 1 ]  REINITIALISER LE LEADERBOARD");
    minitel.newXY(1, 10);
    minitel.print("  [ 2 ]  AJOUTER UNE QUESTION");
    minitel.newXY(1, 12);
    minitel.print("  [ 3 ]  VOIR / MODIFIER LES QUESTIONS");
    minitel.newXY(1, 14);
    minitel.attributs(CARACTERE_ROUGE);
    minitel.print("  [ 4 ]  RETOUR A L'ACCUEIL");
    minitel.attributs(CARACTERE_BLANC);

    rq_pied("  Tapez 1, 2, 3 ou 4");
    champVide(14, 17, 20);
}

// ============================================================
// ECRAN ADMIN : CONFIRMATION RESET LEADERBOARD
// ============================================================
static void rq_afficheAdminConfirmReset() {
    minitel.newScreen();
    rq_bandeau();
    currentEcran = "RQ_ADMIN_RESET";

    minitel.newXY(1, 8);
    minitel.attributs(FOND_ROUGE);
    minitel.attributs(CARACTERE_BLANC);
    rq_centrer("ETES-VOUS SUR ?", 8);
    minitel.newXY(1, 9);
    rq_centrer("Cette action est irreversible !", 9);
    minitel.attributs(FOND_NOIR);
    minitel.newXY(1, 12);
    rq_centrer("[ENVOI/SUITE] OUI    [RETOUR/SOM] NON", 12);

    rq_pied("  ENVOI=Confirmer  SOMMAIRE=Annuler");
    // La gestion des touches se fait dans la loop principale
}

// ============================================================
// ECRAN ADMIN : AJOUT QUESTION (formulaire en 3 etapes)
// ============================================================
static void rq_afficheAdminAjout() {
    // Etape 1 : enonce
    minitel.newScreen();
    rq_bandeau();
    rq_centrer("AJOUT D'UNE QUESTION (1/3)", 3);
    rq_ligneH(4);
    minitel.newXY(1, 6);
    minitel.print("  Enonce de la question :");
    minitel.newXY(1, 8);
    minitel.print("  > ");
    for (int i = 0; i < 35; i++) minitel.print(".");
    rq_pied("  SUITE=Suivant  SOMMAIRE=Annuler");

    char bufEnonce[80] = "";
    unsigned long fin = rq_saisir(5, 8, 35, bufEnonce);
    if (fin == SOMMAIRE || fin == CONNEXION_FIN || strlen(bufEnonce) == 0) {
        currentEcran = "RQ_ADMIN_MENU";
        return;
    }
    rq_editQ.enonce = String(bufEnonce);

    // Etape 2 : 4 choix
    minitel.newScreen();
    rq_bandeau();
    rq_centrer("AJOUT D'UNE QUESTION (2/3)", 3);
    rq_ligneH(4);
    minitel.newXY(1, 5);
    minitel.print("  Les 4 options (SUITE apres chaque) :");

    const char lettres[] = {'A','B','C','D'};
    for (int i = 0; i < 4; i++) {
        int lig = 7 + i * 2;
        minitel.newXY(1, lig);
        char label[5];
        sprintf(label, "  %c/ ", lettres[i]);
        minitel.print(label);
        for (int j = 0; j < 30; j++) minitel.print(".");

        char bufChoix[32] = "";
        unsigned long fc = rq_saisir(5, lig, 30, bufChoix);
        if (fc == SOMMAIRE || fc == CONNEXION_FIN) {
            currentEcran = "RQ_ADMIN_MENU";
            return;
        }
        rq_editQ.choix[i] = (strlen(bufChoix) > 0) ? String(bufChoix) : "---";
    }

    // Etape 3 : bonne reponse
    minitel.newScreen();
    rq_bandeau();
    rq_centrer("AJOUT D'UNE QUESTION (3/3)", 3);
    rq_ligneH(4);
    minitel.newXY(1, 7);
    minitel.print("  Quelle est la bonne reponse ?");
    minitel.newXY(1, 9);
    minitel.print("  Lettre A, B, C ou D : [.]");
    rq_pied("  SUITE=Sauvegarder  SOMMAIRE=Annuler");

    char bufRep[4] = "";
    unsigned long fr = rq_saisir(25, 9, 1, bufRep);
    if (fr == SOMMAIRE || fr == CONNEXION_FIN) {
        currentEcran = "RQ_ADMIN_MENU";
        return;
    }
    char rep = toupper(bufRep[0]);
    rq_editQ.bonneReponse = (rep=='A'||rep=='B'||rep=='C'||rep=='D') ? rep : 'A';

    // Sauvegarde
    if (rq_sauvegarderNouvelleQuestion(rq_editQ)) {
        minitel.newXY(1, 13);
        minitel.attributs(CARACTERE_VERT);
        rq_centrer("QUESTION SAUVEGARDEE !", 13);
        minitel.attributs(CARACTERE_BLANC);
    } else {
        minitel.newXY(1, 13);
        minitel.attributs(CARACTERE_ROUGE);
        rq_centrer("ERREUR DE SAUVEGARDE !", 13);
        minitel.attributs(CARACTERE_BLANC);
    }
    delay(1500);
    currentEcran = "RQ_ADMIN_MENU";
}

// ============================================================
// ECRAN ADMIN : LISTE ET EDITION DES QUESTIONS
// ============================================================
static void rq_afficheAdminListe() {
    RQ_Question banque[RQ_MAX_BANQUE];
    int total = rq_chargerQuestions(banque);

    minitel.newScreen();
    rq_bandeau();
    currentEcran = "RQ_ADMIN_LISTE";

    rq_centrer("LISTE DES QUESTIONS", 3);
    rq_ligneH(4);

    if (total == 0) {
        minitel.newXY(1, 8);
        rq_centrer("Aucune question enregistree.", 8);
        rq_pied("  SOMMAIRE = retour menu admin");
        return;
    }

    int parPage = 10;
    int nbPages = (total + parPage - 1) / parPage;
    if (rq_pageAdmin >= nbPages) rq_pageAdmin = 0;
    int debut = rq_pageAdmin * parPage;
    int fin   = min(debut + parPage, total);

    for (int i = debut; i < fin; i++) {
        minitel.newXY(1, 6 + (i - debut));
        char ligne[41];
        String apercu = banque[i].enonce;
        if ((int)apercu.length() > 30) apercu = apercu.substring(0, 30) + "..";
        sprintf(ligne, " Q%02d: ", i + 1);
        minitel.print(ligne);
        minitel.print(apercu);
    }

    char info[41];
    sprintf(info, "  Total: %d question(s)   Page %d/%d",
            total, rq_pageAdmin + 1, nbPages);
    minitel.newXY(1, 20);
    minitel.print(info);

    minitel.newXY(1, 21);
    minitel.print("  Numero a editer : [");
    minitel.newXY(37, 21);
    minitel.print("]");

    rq_pied("  Num=Editer  SUITE=Pg.suiv  SOM=Menu");
    champVide(22, 21, 2);
}

// Formulaire d'edition pre-rempli
static void rq_afficheAdminEdition(int index) {
    RQ_Question banque[RQ_MAX_BANQUE];
    int total = rq_chargerQuestions(banque);
    if (index < 0 || index >= total) {
        currentEcran = "RQ_ADMIN_LISTE";
        return;
    }
    RQ_Question q = banque[index];

    // Etape 1 : enonce
    minitel.newScreen();
    rq_bandeau();
    char titre[41];
    sprintf(titre, "EDITION Q%02d (1/3)", index + 1);
    rq_centrer(String(titre), 3);
    rq_ligneH(4);
    minitel.newXY(1, 6);
    minitel.print("  Actuel : ");
    minitel.attributs(CARACTERE_CYAN);
    minitel.print(q.enonce);
    minitel.attributs(CARACTERE_BLANC);
    minitel.newXY(1, 9);
    minitel.print("  Nouveau (SUITE=garder) :");
    minitel.newXY(1, 11);
    minitel.print("  > ");
    for (int i = 0; i < 35; i++) minitel.print(".");
    rq_pied("  SUITE=Suivant  SOMMAIRE=Annuler");

    char bufEnonce[80] = "";
    unsigned long fin = rq_saisir(5, 11, 35, bufEnonce);
    if (fin == SOMMAIRE || fin == CONNEXION_FIN) { currentEcran = "RQ_ADMIN_LISTE"; return; }
    if (strlen(bufEnonce) > 0) q.enonce = String(bufEnonce);

    // Etape 2 : choix
    minitel.newScreen();
    rq_bandeau();
    sprintf(titre, "EDITION Q%02d (2/3)", index + 1);
    rq_centrer(String(titre), 3);
    rq_ligneH(4);
    minitel.newXY(1, 5);
    minitel.print("  Actuel -> Nouveau (SUITE=garder) :");

    const char lettres[] = {'A','B','C','D'};
    for (int i = 0; i < 4; i++) {
        int lig = 7 + i * 3;
        minitel.newXY(1, lig);
        minitel.attributs(CARACTERE_CYAN);
        char act[38];
        sprintf(act, "  %c/ %s", lettres[i], q.choix[i].c_str());
        minitel.print(act);
        minitel.attributs(CARACTERE_BLANC);
        minitel.newXY(1, lig + 1);
        minitel.print("  > ");
        for (int j = 0; j < 30; j++) minitel.print(".");

        char bufChoix[32] = "";
        unsigned long fc = rq_saisir(5, lig + 1, 30, bufChoix);
        if (fc == SOMMAIRE || fc == CONNEXION_FIN) { currentEcran = "RQ_ADMIN_LISTE"; return; }
        if (strlen(bufChoix) > 0) q.choix[i] = String(bufChoix);
    }

    // Etape 3 : bonne reponse
    minitel.newScreen();
    rq_bandeau();
    sprintf(titre, "EDITION Q%02d (3/3)", index + 1);
    rq_centrer(String(titre), 3);
    rq_ligneH(4);
    minitel.newXY(1, 7);
    char actRep[20];
    sprintf(actRep, "  Reponse actuelle : %c", q.bonneReponse);
    minitel.print(actRep);
    minitel.newXY(1, 9);
    minitel.print("  Nouvelle (A/B/C/D, SUITE=garder) :");
    minitel.newXY(1, 11);
    minitel.print("  > [.]");
    rq_pied("  SUITE=Sauvegarder  SOMMAIRE=Annuler");

    char bufRep[4] = "";
    unsigned long fr = rq_saisir(5, 11, 1, bufRep);
    if (fr == SOMMAIRE || fr == CONNEXION_FIN) { currentEcran = "RQ_ADMIN_LISTE"; return; }
    char rep = toupper(bufRep[0]);
    if (rep=='A'||rep=='B'||rep=='C'||rep=='D') q.bonneReponse = rep;

    // Sauvegarde
    if (rq_modifierQuestion(index, q)) {
        minitel.newXY(1, 15);
        minitel.attributs(CARACTERE_VERT);
        rq_centrer("QUESTION MISE A JOUR !", 15);
        minitel.attributs(CARACTERE_BLANC);
    } else {
        minitel.newXY(1, 15);
        minitel.attributs(CARACTERE_ROUGE);
        rq_centrer("ERREUR DE SAUVEGARDE !", 15);
        minitel.attributs(CARACTERE_BLANC);
    }
    delay(1500);
    rq_pageAdmin = 0;
    currentEcran = "RQ_ADMIN_LISTE";
}

// ============================================================
// SETUP DU SERVICE  (appele par launchService)
// ============================================================
void setupRetroquiz() {
    Serial.println("setup RetroQuiz");
    initMinitelService();  // pageMode + newScreen + echo(true) du Minimit
    rq_initialiserFichiers();
    rq_afficheAccueil();
}

// ============================================================
// LOOP DU SERVICE  (appele par launchService)
// Retourne quand l'utilisateur appuie sur CNX/FIN depuis l'accueil.
// ============================================================
void loopRetroquiz() {
    Serial.println("loop RetroQuiz");

    while (true) {

        // --- Ecrans qui se gerent via wait_for_user_action ---
        // (ecrans sans saisie de chaine bloquante)
        if (currentEcran == "RQ_ACCUEIL" ||
            currentEcran == "RQ_JEU"     ||
            currentEcran == "RQ_LEADERBOARD" ||
            currentEcran == "RQ_ADMIN_MENU"  ||
            currentEcran == "RQ_ADMIN_RESET" ||
            currentEcran == "RQ_ADMIN_LISTE" ||
            currentEcran == "RQ_ERREUR") {
            wait_for_user_action();
        }

        // --- Transitions provoquees par les ecrans bloquants ---
        // (les fonctions rq_affiche* ont deja change currentEcran)

        if (currentEcran == "RQ_RESULTATS") {
            rq_afficheResultats();
            if (currentEcran == "RQ_FIN") return;
            rq_afficheLeaderboard();
            currentEcran = "RQ_LEADERBOARD";
            continue;
        }

        if (currentEcran == "RQ_ADMIN_MDP") {
            rq_afficheAdminMdp();
            // currentEcran mis a jour dans la fonction
            if (currentEcran == "RQ_ADMIN_MENU") rq_afficheAdminMenu();
            else rq_afficheAccueil();
            continue;
        }

        if (currentEcran == "RQ_ADMIN_AJOUT") {
            rq_afficheAdminAjout();
            rq_afficheAdminMenu();
            continue;
        }

        // --- Machine a etats sur wait_for_user_action ---

        if (currentEcran == "RQ_ACCUEIL") {
            switch (touche) {
                case CONNEXION_FIN: return;
                case '1': rq_demarrerPartie(); break;
                case '2': rq_afficheLeaderboard(); currentEcran = "RQ_LEADERBOARD"; break;
                case '9': currentEcran = "RQ_ADMIN_MDP"; break;
                default: champVide(14, 17, 20); break;
            }
        }

        else if (currentEcran == "RQ_JEU") {
            switch (touche) {
                case CONNEXION_FIN: return;
                case SOMMAIRE: rq_afficheAccueil(); break;
                case 'A': case 'a':
                    rq_reponse = 'A';
                    minitel.newXY(30, 19);
                    minitel.attributs(CARACTERE_JAUNE);
                    minitel.print("A");
                    minitel.attributs(CARACTERE_BLANC);
                    break;
                case 'B': case 'b':
                    rq_reponse = 'B';
                    minitel.newXY(30, 19);
                    minitel.attributs(CARACTERE_JAUNE);
                    minitel.print("B");
                    minitel.attributs(CARACTERE_BLANC);
                    break;
                case 'C': case 'c':
                    rq_reponse = 'C';
                    minitel.newXY(30, 19);
                    minitel.attributs(CARACTERE_JAUNE);
                    minitel.print("C");
                    minitel.attributs(CARACTERE_BLANC);
                    break;
                case 'D': case 'd':
                    rq_reponse = 'D';
                    minitel.newXY(30, 19);
                    minitel.attributs(CARACTERE_JAUNE);
                    minitel.print("D");
                    minitel.attributs(CARACTERE_BLANC);
                    break;
                case SUITE: rq_validerReponse(); break;
                default: break;
            }
        }

        else if (currentEcran == "RQ_LEADERBOARD" || currentEcran == "RQ_ERREUR") {
            switch (touche) {
                case CONNEXION_FIN: return;
                case SOMMAIRE: rq_afficheAccueil(); break;
                default: break;
            }
        }

        else if (currentEcran == "RQ_ADMIN_MENU") {
            switch (touche) {
                case CONNEXION_FIN: return;
                case '1': rq_afficheAdminConfirmReset(); break;
                case '2': currentEcran = "RQ_ADMIN_AJOUT"; break;
                case '3':
                    rq_pageAdmin = 0;
                    rq_afficheAdminListe();
                    break;
                case '4': rq_afficheAccueil(); break;
                case SOMMAIRE: rq_afficheAccueil(); break;
                default: champVide(14, 17, 20); break;
            }
        }

        else if (currentEcran == "RQ_ADMIN_RESET") {
            switch (touche) {
                case CONNEXION_FIN: return;
                case ENVOI:
                case SUITE:
                    rq_reinitialiserLeaderboard();
                    minitel.newXY(1, 15);
                    minitel.attributs(CARACTERE_VERT);
                    rq_centrer("LEADERBOARD REINITIALISE !", 15);
                    minitel.attributs(CARACTERE_BLANC);
                    delay(1500);
                    rq_afficheAdminMenu();
                    break;
                case RETOUR:
                case SOMMAIRE:
                    rq_afficheAdminMenu();
                    break;
                default: break;
            }
        }

        else if (currentEcran == "RQ_ADMIN_LISTE") {
            switch (touche) {
                case CONNEXION_FIN: return;
                case SOMMAIRE: rq_afficheAdminMenu(); break;
                case SUITE:
                    rq_pageAdmin++;
                    rq_afficheAdminListe();
                    break;
                default:
                    // Chiffres 1-9 : debut de saisie du numero
                    if (touche >= '1' && touche <= '9') {
                        // Afficher le 1er chiffre deja tape
                        minitel.newXY(22, 21);
                        minitel.attributs(CARACTERE_JAUNE);
                        minitel.print(String((char)touche));
                        minitel.attributs(CARACTERE_BLANC);

                        // Attendre un 2eme chiffre optionnel
                        char buf[4] = {(char)touche, 0, 0, 0};
                        unsigned long t2 = minitel.getKeyCode();
                        if (t2 >= '0' && t2 <= '9') {
                            buf[1] = (char)t2;
                            buf[2] = '\0';
                            minitel.print(String((char)t2));
                        }
                        int num = atoi(buf);

                        RQ_Question banque[RQ_MAX_BANQUE];
                        int total = rq_chargerQuestions(banque);
                        if (num >= 1 && num <= total) {
                            rq_afficheAdminEdition(num - 1);
                            rq_afficheAdminListe();
                        } else {
                            minitel.newXY(1, 22);
                            minitel.attributs(CARACTERE_ROUGE);
                            minitel.print("  Numero invalide !");
                            minitel.attributs(CARACTERE_BLANC);
                            delay(1000);
                            rq_afficheAdminListe();
                        }
                    }
                    break;
            }
        }

        else if (currentEcran == "RQ_FIN") {
            return;
        }

    } // fin while(true)
}
