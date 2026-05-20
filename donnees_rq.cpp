// ============================================================
//  DONNEES_RQ.CPP
//  Gestion des donnees persistantes pour 3615 RETROQUIZ
//  Stockage : SPIFFS (coherent avec le reste du Minimit)
// ============================================================

#include "donnees_rq.h"

// ============================================================
// INITIALISATION
// Cree les fichiers absents au premier demarrage.
// SPIFFS.begin() est deja appele dans le setup() principal
// du sketch_minimit — on n'y touche pas.
// ============================================================
void rq_initialiserFichiers() {
    if (!SPIFFS.exists(RQ_FICHIER_LEADERBOARD)) {
        rq_reinitialiserLeaderboard();
    }
    // questions.txt est fourni dans le data/ flash — pas besoin de le creer
}

// ============================================================
// LEADERBOARD
// ============================================================
void rq_chargerLeaderboard(RQ_Score table[]) {
    for (int i = 0; i < RQ_MAX_SCORES; i++) {
        strcpy(table[i].pseudo, "---");
        table[i].points = 0;
    }
    File f = SPIFFS.open(RQ_FICHIER_LEADERBOARD, "r");
    if (!f) return;
    f.read((uint8_t*)table, sizeof(RQ_Score) * RQ_MAX_SCORES);
    f.close();
}

void rq_sauvegarderLeaderboard(RQ_Score table[]) {
    File f = SPIFFS.open(RQ_FICHIER_LEADERBOARD, "w");
    if (!f) return;
    f.write((const uint8_t*)table, sizeof(RQ_Score) * RQ_MAX_SCORES);
    f.close();
}

void rq_reinitialiserLeaderboard() {
    RQ_Score vide[RQ_MAX_SCORES];
    for (int i = 0; i < RQ_MAX_SCORES; i++) {
        strcpy(vide[i].pseudo, "---");
        vide[i].points = 0;
    }
    rq_sauvegarderLeaderboard(vide);
}

// Insere le score si eligible, retrie et sauvegarde.
// Retourne true si le joueur entre dans le classement.
bool rq_insererScore(RQ_Score table[], const char* pseudo, int points) {
    if (points <= table[RQ_MAX_SCORES - 1].points &&
        table[RQ_MAX_SCORES - 1].points != 0) {
        return false;
    }
    strncpy(table[RQ_MAX_SCORES - 1].pseudo, pseudo, RQ_MAX_PSEUDO);
    table[RQ_MAX_SCORES - 1].pseudo[RQ_MAX_PSEUDO] = '\0';
    table[RQ_MAX_SCORES - 1].points = points;

    // Tri decroissant (10 elements max, bubble sort suffisant)
    for (int i = 0; i < RQ_MAX_SCORES - 1; i++) {
        for (int j = 0; j < RQ_MAX_SCORES - 1 - i; j++) {
            if (table[j].points < table[j + 1].points) {
                RQ_Score tmp   = table[j];
                table[j]       = table[j + 1];
                table[j + 1]   = tmp;
            }
        }
    }
    rq_sauvegarderLeaderboard(table);
    return true;
}

// ============================================================
// QUESTIONS
// Format du fichier rq_questions.txt :
//   ligne 1 : enonce
//   ligne 2 : choix A
//   ligne 3 : choix B
//   ligne 4 : choix C
//   ligne 5 : choix D
//   ligne 6 : bonne reponse (A, B, C ou D)
//   (bloc de 6 lignes repete pour chaque question)
// ============================================================
int rq_chargerQuestions(RQ_Question banque[]) {
    if (!SPIFFS.exists(RQ_FICHIER_QUESTIONS)) return 0;
    File f = SPIFFS.open(RQ_FICHIER_QUESTIONS, "r");
    if (!f) return 0;

    int index = 0;
    while (f.available() && index < RQ_MAX_BANQUE) {
        String enonce = f.readStringUntil('\n');
        enonce.trim();
        if (enonce.length() == 0) continue;

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

bool rq_sauvegarderNouvelleQuestion(const RQ_Question& q) {
    File f = SPIFFS.open(RQ_FICHIER_QUESTIONS, "a");
    if (!f) return false;
    f.println(q.enonce);
    for (int i = 0; i < 4; i++) f.println(q.choix[i]);
    f.println(q.bonneReponse);
    f.close();
    return true;
}

bool rq_modifierQuestion(int index, const RQ_Question& q) {
    RQ_Question banque[RQ_MAX_BANQUE];
    int total = rq_chargerQuestions(banque);
    if (index < 0 || index >= total) return false;
    banque[index] = q;

    File f = SPIFFS.open(RQ_FICHIER_QUESTIONS, "w");
    if (!f) return false;
    for (int i = 0; i < total; i++) {
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
void rq_tirerQuestions(int totalBanque, int indices[]) {
    for (int i = 0; i < RQ_NB_QUESTIONS_PARTIE; i++) indices[i] = -1;
    if (totalBanque < RQ_NB_QUESTIONS_PARTIE) return;

    randomSeed(analogRead(0));
    int nb = 0, tentatives = 0;
    while (nb < RQ_NB_QUESTIONS_PARTIE && tentatives < 1000) {
        int c = random(0, totalBanque);
        bool deja = false;
        for (int i = 0; i < nb; i++) if (indices[i] == c) { deja = true; break; }
        if (!deja) indices[nb++] = c;
        tentatives++;
    }
}
