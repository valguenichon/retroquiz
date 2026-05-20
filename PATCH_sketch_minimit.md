# PATCH — Intégration de 3615 RETROQUIZ dans le Minimit
# Deux fichiers existants à modifier manuellement.

==============================================================
## FICHIER 1 : sketch_minimit/guideservices.ino
==============================================================

### Modification 1a — Ajouter RETROQUIZ dans le tableau LocalService[]

Trouver :
```cpp
String LocalService[] = { "ANNUAIRE",
                          "ASTRO",
                          "COUPLESPARFAITS",
                          "ELIZA",
                          "FORTUNE",
                          "GALERIE",
                          "LEMONDE",
                          "METEO",
                          "NABAZTAG",
                          "PENDU",
                          "PONG",
                          "PPP",
                          "TAROT" };

int NB_LOCAL_SERVICES = 13;
```

Remplacer par :
```cpp
String LocalService[] = { "ANNUAIRE",
                          "ASTRO",
                          "COUPLESPARFAITS",
                          "ELIZA",
                          "FORTUNE",
                          "GALERIE",
                          "LEMONDE",
                          "METEO",
                          "NABAZTAG",
                          "PENDU",
                          "PONG",
                          "PPP",
                          "TAROT",
                          "RETROQUIZ" };   // <<< AJOUT

int NB_LOCAL_SERVICES = 14;               // <<< 13 -> 14
```

==============================================================
## FICHIER 2 : sketch_minimit/sketch_minimit.ino
==============================================================

### Modification 2a — Ajouter le #include en tête de fichier

Trouver (les includes existants) :
```cpp
#include <WiFi.h>
#include <HTTPClient.h>
#include "SPIFFS.h"
#include "FS.h"
#include <Arduino_JSON.h>
#include <Minitel1B_Hard.h>
#include <WebSocketsClient.h>
```

Ajouter juste après :
```cpp
#include "donnees_rq.h"   // <<< AJOUT RetroQuiz
```

---

### Modification 2b — Ajouter RETROQUIZ dans launchService()

Dans la fonction `launchService()`, trouver le dernier service avant les
services internes (CONFIG, CREDITS, GUIDE, OTA) :

```cpp
  if (minimit_service == "TAROT" || minimit_service == "13") {
    if (!isConnected) {
      return 1;
    }
    setupTarots();
    loopTarots();
    return 0;
  }
  if (minimit_service == "CONFIG") {
```

Insérer entre TAROT et CONFIG :
```cpp
  // --- 3615 RETROQUIZ (service local, pas de WiFi requis) ---
  if (minimit_service == "RETROQUIZ" || minimit_service == "14") {
    setupRetroquiz();
    loopRetroquiz();
    return 0;
  }
  // ----------------------------------------------------------
```

==============================================================
## FICHIER 3 : data/ — fichier questions à flasher via SPIFFS
==============================================================

Créer le dossier data/ dans sketch_minimit/ (s'il n'existe pas) et y
placer le fichier rq_questions.txt (fourni séparément).

Le fichier sera accessible dans SPIFFS sous le chemin /rq_questions.txt.

Le leaderboard /rq_scores.dat est créé automatiquement au 1er lancement.

==============================================================
## RESUME DES FICHIERS A PLACER DANS sketch_minimit/
==============================================================

  sketch_minimit/
  ├── sketch_minimit.ino   ← MODIFIER (2 endroits)
  ├── guideservices.ino    ← MODIFIER (2 endroits)
  ├── retroquiz.ino        ← NOUVEAU (copier tel quel)
  ├── donnees_rq.h         ← NOUVEAU (copier tel quel)
  ├── donnees_rq.cpp       ← NOUVEAU (copier tel quel)
  └── data/
      └── rq_questions.txt ← NOUVEAU (flasher via SPIFFS upload)
```
