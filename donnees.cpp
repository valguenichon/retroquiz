#include "donnees.h"

// ============================================================
// CHEMINS DES FICHIERS EN FLASH
// ============================================================
static const char* FICHIER_LEADERBOARD = "/leaderboard.dat";
static const char* FICHIER_QUESTIONS   = "/questions.txt";

// ============================================================
// INITIALISATION
// ============================================================

bool initialiserStockage() {
    // "true" = formater si le systeme est corrompu
    if (!LittleFS.begin(true)) {
        return false;
    }
    // Creer le leaderboard vide si absent
    if (!LittleFS.exists(FICHIER_LEADERBOARD)) {
        reinitialiserLeaderboard();
    }
    return true;
}

// ============================================================
// LEADERBOARD
// ============================================================

void chargerLeaderboard(Score table[]) {
    // Initialiser a vide par securite
    for (int i = 0; i < MAX_SCORES; i++) {
        strcpy(table[i].pseudo, "---");
        table[i].points = 0;
    }
    File f = LittleFS.open(FICHIER_LEADERBOARD, "r");
    if (!f) return;
    f.read((uint8_t*)table, sizeof(Score) * MAX_SCORES);
    f.close();
}

void sauvegarderLeaderboard(Score table[]) {
    File f = LittleFS.open(FICHIER_LEADERBOARD, "w");
    if (!f) return;
    f.write((const uint8_t*)table, sizeof(Score) * MAX_SCORES);
    f.close();
}

void reinitialiserLeaderboard() {
    Score tableVide[MAX_SCORES];
    for (int i = 0; i < MAX_SCORES; i++) {
        strcpy(tableVide[i].pseudo, "---");
        tableVide[i].points = 0;
    }
    sauvegarderLeaderboard(tableVide);
}

// Insere le score si eligible au Top 10, retrie et sauvegarde.
// Retourne true si le joueur est entre dans le classement.
bool insererScore(Score table[], const char* pseudo, int points) {
    // Verifier si le score est suffisant pour entrer
    if (points <= table[MAX_SCORES - 1].points &&
        table[MAX_SCORES - 1].points != 0) {
        return false;
    }
    // Remplacer la derniere place
    strncpy(table[MAX_SCORES - 1].pseudo, pseudo, MAX_PSEUDO);
    table[MAX_SCORES - 1].pseudo[MAX_PSEUDO] = '\0';
    table[MAX_SCORES - 1].points = points;

    // Tri par ordre decroissant (bubble sort simple, 10 elements max)
    for (int i = 0; i < MAX_SCORES - 1; i++) {
        for (int j = 0; j < MAX_SCORES - 1 - i; j++) {
            if (table[j].points < table[j + 1].points) {
                Score tmp    = table[j];
                table[j]     = table[j + 1];
                table[j + 1] = tmp;
            }
        }
    }
    sauvegarderLeaderboard(table);
    return true;
}

// ============================================================
// QUESTIONS
// ============================================================

// Lecture du fichier questions.txt ligne par ligne.
// Format : enonce / choixA / choixB / choixC / choixD / bonneReponse
// Retourne le nombre de questions chargees.
int chargerQuestions(Question banque[]) {
    if (!LittleFS.exists(FICHIER_QUESTIONS)) return 0;
    File f = LittleFS.open(FICHIER_QUESTIONS, "r");
    if (!f) return 0;

    int index = 0;
    while (f.available() && index < MAX_BANQUE_QUESTIONS) {
        String enonce = f.readStringUntil('\n');
        enonce.trim();
        if (enonce.length() == 0) continue; // ligne vide ignoree

        banque[index].enonce = enonce;
        for (int i = 0; i < 4; i++) {
            banque[index].choix[i] = f.readStringUntil('\n');
            banque[index].choix[i].trim();
        }
        String rep = f.readStringUntil('\n');
        rep.trim();
        banque[index].bonneReponse = (rep.length() > 0) ? rep.charAt(0) : 'A';

        index++;
    }
    f.close();
    return index;
}

// Ajoute une question a la fin du fichier (mode "append")
bool sauvegarderNouvelleQuestion(const Question& q) {
    File f = LittleFS.open(FICHIER_QUESTIONS, "a");
    if (!f) return false;
    f.println(q.enonce);
    for (int i = 0; i < 4; i++) f.println(q.choix[i]);
    f.println(q.bonneReponse);
    f.close();
    return true;
}

// Modifie la question a l'index donne (rechargement complet + reecriture)
bool modifierQuestion(int index, const Question& q) {
    Question banque[MAX_BANQUE_QUESTIONS];
    int total = chargerQuestions(banque);
    if (index < 0 || index >= total) return false;
    banque[index] = q;

    File f = LittleFS.open(FICHIER_QUESTIONS, "w");
    if (!f) return false;
    for (int i = 0; i < total; i++) {
        f.println(banque[i].enonce);
        for (int j = 0; j < 4; j++) f.println(banque[i].choix[j]);
        f.println(banque[i].bonneReponse);
    }
    f.close();
    return true;
}

// Supprime la question a l'index donne
bool supprimerQuestion(int index) {
    Question banque[MAX_BANQUE_QUESTIONS];
    int total = chargerQuestions(banque);
    if (index < 0 || index >= total) return false;

    File f = LittleFS.open(FICHIER_QUESTIONS, "w");
    if (!f) return false;
    for (int i = 0; i < total; i++) {
        if (i == index) continue;
        f.println(banque[i].enonce);
        for (int j = 0; j < 4; j++) f.println(banque[i].choix[j]);
        f.println(banque[i].bonneReponse);
    }
    f.close();
    return true;
}

// ============================================================
// TIRAGE ALEATOIRE SANS REPETITION
// ============================================================

void tirerQuestions(const Question banque[], int totalBanque,
                    int indicesPartie[NB_QUESTIONS_PARTIE]) {
    // Initialiser avec des indices invalides
    for (int i = 0; i < NB_QUESTIONS_PARTIE; i++) indicesPartie[i] = -1;

    if (totalBanque < NB_QUESTIONS_PARTIE) return; // Banque trop petite

    int nbTires = 0;
    int tentatives = 0;
    randomSeed(analogRead(0)); // Bruit analogique = meilleure graine

    while (nbTires < NB_QUESTIONS_PARTIE && tentatives < 1000) {
        int candidat = random(0, totalBanque);
        bool dejaChoisi = false;
        for (int i = 0; i < nbTires; i++) {
            if (indicesPartie[i] == candidat) { dejaChoisi = true; break; }
        }
        if (!dejaChoisi) indicesPartie[nbTires++] = candidat;
        tentatives++;
    }
}
