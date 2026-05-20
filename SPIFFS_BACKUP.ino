// ============================================================
//  SPIFFS_BACKUP.INO
//  Sketch temporaire pour sauvegarder tous les fichiers SPIFFS
//  du Minimit avant un upload.
//
//  UTILISATION :
//  1. Ouvrir ce fichier comme sketch SEUL (pas dans sketch_minimit/)
//  2. Configurer : carte ESP32 Wrover Kit, port COM du Minimit
//  3. Televerser
//  4. Ouvrir le Moniteur Serie a 115200 bauds
//  5. Les fichiers s'affichent — copier/coller chacun dans un
//     fichier local portant le meme nom
//  6. Placer ces fichiers dans sketch_minimit/data/
//  7. Ajouter rq_questions.txt dans data/
//  8. Faire le SPIFFS Data Upload
//  9. Reflasher le vrai sketch_minimit
// ============================================================

#include "SPIFFS.h"
#include "FS.h"

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println();
    Serial.println("============================================================");
    Serial.println("  MINIMIT SPIFFS BACKUP");
    Serial.println("============================================================");

    if (!SPIFFS.begin(false)) {  // false = ne pas formater si erreur
        Serial.println("ERREUR : impossible de monter SPIFFS !");
        Serial.println("Le SPIFFS est peut-etre vide ou corrompu.");
        return;
    }

    Serial.println();

    // Lister tous les fichiers
    File root = SPIFFS.open("/");
    if (!root || !root.isDirectory()) {
        Serial.println("ERREUR : impossible d'ouvrir la racine SPIFFS.");
        return;
    }

    int nbFichiers = 0;
    File entry = root.openNextFile();
    while (entry) {
        if (!entry.isDirectory()) {
            nbFichiers++;
        }
        entry = root.openNextFile();
    }

    Serial.print("  Nombre de fichiers trouves : ");
    Serial.println(nbFichiers);
    Serial.println();

    if (nbFichiers == 0) {
        Serial.println("  Le SPIFFS est vide. Rien a sauvegarder.");
        return;
    }

    // Relire depuis le debut et dumper chaque fichier
    root = SPIFFS.open("/");
    entry = root.openNextFile();

    while (entry) {
        if (!entry.isDirectory()) {
            String nom = entry.name();
            size_t taille = entry.size();

            Serial.println("------------------------------------------------------------");
            Serial.print("FICHIER : ");
            Serial.println(nom);
            Serial.print("TAILLE  : ");
            Serial.print(taille);
            Serial.println(" octets");
            Serial.println("--- DEBUT CONTENU ---");

            // Lire et afficher octet par octet
            // Pour les fichiers binaires, on affiche en hex
            // Pour les fichiers texte, on affiche directement

            bool estTexte = nom.endsWith(".txt") ||
                            nom.endsWith(".json") ||
                            nom.endsWith(".htm")  ||
                            nom.endsWith(".html") ||
                            nom.endsWith(".csv");

            if (estTexte) {
                // Affichage texte direct
                while (entry.available()) {
                    Serial.write(entry.read());
                }
            } else {
                // Affichage hexadecimal pour les fichiers binaires
                Serial.println("[FORMAT BINAIRE — affichage HEX]");
                size_t pos = 0;
                while (entry.available()) {
                    if (pos % 16 == 0) {
                        char addr[8];
                        sprintf(addr, "%04X: ", (unsigned int)pos);
                        Serial.print(addr);
                    }
                    byte b = entry.read();
                    if (b < 0x10) Serial.print("0");
                    Serial.print(b, HEX);
                    Serial.print(" ");
                    pos++;
                    if (pos % 16 == 0) Serial.println();
                }
                if (pos % 16 != 0) Serial.println();
                Serial.println();
                Serial.println("[RAPPEL : ce fichier est binaire.");
                Serial.println(" Il sera reecrit automatiquement par");
                Serial.println(" le sketch au premier demarrage.]");
            }

            Serial.println();
            Serial.println("--- FIN CONTENU ---");
            Serial.println();
        }
        entry = root.openNextFile();
    }

    Serial.println("============================================================");
    Serial.println("  BACKUP TERMINE");
    Serial.println();
    Serial.println("  FICHIERS TEXTE (.txt, .json...) :");
    Serial.println("  -> Copier le contenu affiche ci-dessus dans");
    Serial.println("     des fichiers locaux du meme nom.");
    Serial.println("  -> Les placer dans sketch_minimit/data/");
    Serial.println();
    Serial.println("  FICHIERS BINAIRES (.dat...) :");
    Serial.println("  -> Ne pas les copier manuellement.");
    Serial.println("  -> Ils seront recrees automatiquement au");
    Serial.println("     premier lancement du sketch_minimit.");
    Serial.println("     (ex: rq_scores.dat, leaderboard, config bin)");
    Serial.println();
    Serial.println("  IMPORTANT : le fichier conf6.txt contient vos");
    Serial.println("  identifiants WiFi — sauvegardez-le bien !");
    Serial.println("============================================================");
}

void loop() {
    // Rien — tout est fait dans setup()
}
