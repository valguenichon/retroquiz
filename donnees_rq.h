#ifndef DONNEES_RQ_H
#define DONNEES_RQ_H

// ============================================================
//  DONNEES_RQ.H
//  Module de donnees pour 3615 RETROQUIZ
//  Adapte pour le Minimit : SPIFFS (et non LittleFS)
//  Prefixe "rq_" sur toutes les fonctions pour eviter
//  tout conflit de noms avec les autres services Minimit.
// ============================================================

#include <Arduino.h>
#include "SPIFFS.h"

// --- Configuration ---
const int RQ_MAX_SCORES          = 10;
const int RQ_MAX_BANQUE          = 100;
const int RQ_NB_QUESTIONS_PARTIE = 10;
const int RQ_MAX_PSEUDO          = 8;
const char* const RQ_MDP_ADMIN   = "1234"; // A changer avant de flasher !

// --- Chemins fichiers (prefixes rq_ pour eviter collisions) ---
const char* const RQ_FICHIER_QUESTIONS   = "/rq_questions.txt";
const char* const RQ_FICHIER_LEADERBOARD = "/rq_scores.dat";

// --- Structures ---
struct RQ_Score {
    char pseudo[RQ_MAX_PSEUDO + 1];
    int  points;
};

struct RQ_Question {
    String enonce;
    String choix[4];
    char   bonneReponse; // 'A', 'B', 'C' ou 'D'
};

// --- Declarations ---
void rq_initialiserFichiers();
void rq_chargerLeaderboard(RQ_Score table[]);
void rq_sauvegarderLeaderboard(RQ_Score table[]);
void rq_reinitialiserLeaderboard();
bool rq_insererScore(RQ_Score table[], const char* pseudo, int points);
int  rq_chargerQuestions(RQ_Question banque[]);
bool rq_sauvegarderNouvelleQuestion(const RQ_Question& q);
bool rq_modifierQuestion(int index, const RQ_Question& q);
void rq_tirerQuestions(int totalBanque, int indices[]);

#endif
