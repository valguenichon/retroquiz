#ifndef DONNEES_H
#define DONNEES_H

#include <Arduino.h>
#include <LittleFS.h>

// ============================================================
// CONFIGURATION GENERALE
// ============================================================
const int MAX_SCORES          = 10;   // Taille du top leaderboard
const int MAX_BANQUE_QUESTIONS = 100; // Capacite maximale de la banque
const int NB_QUESTIONS_PARTIE  = 10;  // Questions tirees par partie
const int MAX_PSEUDO           = 8;   // Longueur max du pseudo joueur
const char* const MDP_ADMIN    = "1234"; // Mot de passe admin (a changer !)

// ============================================================
// STRUCTURES DE DONNEES
// ============================================================

// Un score du Leaderboard
struct Score {
    char pseudo[MAX_PSEUDO + 1]; // +1 pour le '\0'
    int  points;
};

// Une question de la banque
struct Question {
    String enonce;
    String choix[4];
    char   bonneReponse; // 'A', 'B', 'C' ou 'D'
};

// ============================================================
// DECLARATIONS DES FONCTIONS
// ============================================================

// --- Initialisation ---
bool initialiserStockage();

// --- Leaderboard ---
void chargerLeaderboard(Score table[]);
void sauvegarderLeaderboard(Score table[]);
void reinitialiserLeaderboard();
bool insererScore(Score table[], const char* pseudo, int points);

// --- Questions ---
int  chargerQuestions(Question banque[]);
bool sauvegarderNouvelleQuestion(const Question& q);
bool modifierQuestion(int index, const Question& q);
bool supprimerQuestion(int index);

// --- Tirage aleatoire ---
void tirerQuestions(const Question banque[], int totalBanque,
                    int indicesPartie[NB_QUESTIONS_PARTIE]);

#endif // DONNEES_H
