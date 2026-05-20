#!/usr/bin/env python3
# =============================================================
#  SIMULATEUR 3615 RETROQUIZ
#  Reproduit la logique exacte de retroquiz.ino dans le terminal
#  Compatible Windows (cmd/PowerShell) et Linux/Mac
# =============================================================

import os
import sys
import random
import json
import time

# --- Configuration (identique a donnees.h) ---
MAX_SCORES           = 10
NB_QUESTIONS_PARTIE  = 10
MAX_PSEUDO           = 8
MDP_ADMIN            = "1234"
FICHIER_QUESTIONS    = "questions.txt"
FICHIER_LEADERBOARD  = "leaderboard.json"
LARGEUR              = 40

# =============================================================
# UTILITAIRES AFFICHAGE
# =============================================================

def effacer():
    os.system('cls' if os.name == 'nt' else 'clear')

def centrer(texte, largeur=LARGEUR):
    return texte.center(largeur)

def ligne_h(car='-', largeur=LARGEUR):
    print(car * largeur)

def bandeau(compteur=None):
    titre = "3615 RETROQUIZ"
    if compteur is not None:
        droite = f"[{compteur:02d}/10]"
        espace = LARGEUR - len(titre) - len(droite)
        print(titre + " " * espace + droite)
    else:
        print(titre.ljust(LARGEUR))
    print("=" * LARGEUR)

def pied_page(ligne1, ligne2=""):
    print()
    print("-" * LARGEUR)
    print(ligne1)
    if ligne2:
        print(ligne2)

def pause(msg="  Appuyez sur ENTREE pour continuer..."):
    input(msg)

def saisir(prompt, max_len=None, masquer=False):
    """Saisie avec longueur max optionnelle."""
    while True:
        if masquer:
            import getpass
            val = getpass.getpass(prompt)
        else:
            val = input(prompt)
        if max_len and len(val) > max_len:
            print(f"  (Maximum {max_len} caracteres)")
            continue
        return val

# =============================================================
# GESTION DES DONNEES
# =============================================================

def charger_questions():
    questions = []
    if not os.path.exists(FICHIER_QUESTIONS):
        return questions
    with open(FICHIER_QUESTIONS, "r", encoding="utf-8") as f:
        lignes = [l.rstrip('\n') for l in f.readlines()]

    i = 0
    while i + 5 < len(lignes):
        enonce = lignes[i].strip()
        if not enonce:
            i += 1
            continue
        choix = [lignes[i+1].strip(), lignes[i+2].strip(),
                 lignes[i+3].strip(), lignes[i+4].strip()]
        bonne = lignes[i+5].strip().upper()
        if bonne not in ('A', 'B', 'C', 'D'):
            bonne = 'A'
        questions.append({
            "enonce":   enonce,
            "choix":    choix,
            "reponse":  bonne
        })
        i += 6
    return questions

def sauvegarder_question(q):
    with open(FICHIER_QUESTIONS, "a", encoding="utf-8") as f:
        f.write(q["enonce"] + "\n")
        for c in q["choix"]:
            f.write(c + "\n")
        f.write(q["reponse"] + "\n")

def charger_leaderboard():
    if not os.path.exists(FICHIER_LEADERBOARD):
        return [{"pseudo": "---", "points": 0} for _ in range(MAX_SCORES)]
    with open(FICHIER_LEADERBOARD, "r", encoding="utf-8") as f:
        return json.load(f)

def sauvegarder_leaderboard(lb):
    with open(FICHIER_LEADERBOARD, "w", encoding="utf-8") as f:
        json.dump(lb, f, ensure_ascii=False, indent=2)

def reinitialiser_leaderboard():
    lb = [{"pseudo": "---", "points": 0} for _ in range(MAX_SCORES)]
    sauvegarder_leaderboard(lb)

def inserer_score(lb, pseudo, points):
    """Insere si eligible, retrie, sauvegarde. Retourne True si entre."""
    if points <= lb[-1]["points"] and lb[-1]["points"] != 0:
        return False
    lb[-1] = {"pseudo": pseudo[:MAX_PSEUDO], "points": points}
    lb.sort(key=lambda x: x["points"], reverse=True)
    sauvegarder_leaderboard(lb)
    return True

def tirer_questions(banque):
    if len(banque) < NB_QUESTIONS_PARTIE:
        return None
    return random.sample(range(len(banque)), NB_QUESTIONS_PARTIE)

# =============================================================
# ECRANS
# =============================================================

def ecran_accueil():
    while True:
        effacer()
        bandeau()
        print()
        print(centrer("** 3615 RETROQUIZ **"))
        print(centrer("Le quiz du Minitel !"))
        print()
        ligne_h('=')
        print()
        print("  [ 1 ]  JOUER AU QUIZ")
        print()
        print("  [ 2 ]  LEADERBOARD")
        print()
        pied_page(
            "  Tapez votre choix puis ENTREE",
            "  (Tapez 9 pour l'acces admin)"
        )
        choix = input("\n  > ").strip()

        if choix == '1':
            ecran_jeu()
        elif choix == '2':
            ecran_leaderboard()
        elif choix == '9':
            ecran_admin_mdp()


def ecran_jeu():
    banque = charger_questions()
    if len(banque) < NB_QUESTIONS_PARTIE:
        effacer()
        bandeau()
        print()
        print(f"  ERREUR : il faut au moins {NB_QUESTIONS_PARTIE} questions !")
        print(f"  Actuellement : {len(banque)} question(s) en base.")
        print()
        print("  Allez en mode ADMIN pour en ajouter.")
        pied_page("  [ENTREE] Retour a l'accueil")
        pause("")
        return

    indices = tirer_questions(banque)
    score   = 0

    for num, idx in enumerate(indices):
        q = banque[idx]
        choix_joueur = None

        while True:
            effacer()
            bandeau(num + 1)
            print()
            # Enonce (tronque a 40 colonnes visuellement)
            print(f"  {q['enonce']}")
            print()
            ligne_h('-')
            print()
            lettres = ['A', 'B', 'C', 'D']
            for i, c in enumerate(q["choix"]):
                sel = " <" if choix_joueur == lettres[i] else "  "
                print(f"  {lettres[i]}/ {c}{sel}")
            print()
            if choix_joueur:
                print(f"  Votre reponse : [ {choix_joueur} ]")
            else:
                print("  Votre reponse : [   ]")

            pied_page(
                "  [A/B/C/D] Selectionner   [V] Valider",
                "  [Q] Abandonner la partie"
            )
            saisie = input("\n  > ").strip().upper()

            if saisie in ('A', 'B', 'C', 'D'):
                choix_joueur = saisie

            elif saisie == 'V':
                if not choix_joueur:
                    print("\n  Selectionnez une reponse (A/B/C/D) !")
                    time.sleep(1)
                    continue
                # Verifier
                correct = (choix_joueur == q["reponse"])
                effacer()
                bandeau(num + 1)
                print()
                print(f"  {q['enonce']}")
                print()
                ligne_h('-')
                for i, c in enumerate(q["choix"]):
                    prefix = ">>> " if lettres[i] == q["reponse"] else "    "
                    print(f"  {lettres[i]}/ {c}  {prefix}")
                print()
                if correct:
                    score += 1
                    print("  *** BONNE REPONSE ! ***")
                else:
                    print(f"  --- FAUX ! La bonne reponse etait : {q['reponse']}")
                time.sleep(1.5)
                break  # Question suivante

            elif saisie == 'Q':
                # Confirmation abandon
                effacer()
                bandeau(num + 1)
                print()
                print(centrer("QUITTER LA PARTIE ?"))
                print()
                print(centrer("[O] OUI     [N] NON"))
                confirm = input("\n  > ").strip().upper()
                if confirm == 'O':
                    return  # Retour accueil
                # Sinon on reboucle sur la meme question

    # Fin de partie -> resultats
    ecran_resultats(score)


def ecran_resultats(score):
    effacer()
    bandeau()
    print()
    print(centrer("*** BRAVO ! ***"))
    print()
    print(centrer("VOUS AVEZ REUSSI"))
    print(centrer(f"{score} / {NB_QUESTIONS_PARTIE}"))
    print(centrer("BONNES REPONSES"))
    print()
    ligne_h('-')
    print()
    print("  ENTREZ VOTRE PSEUDO POUR LE CLASSEMENT")
    print(f"  ({MAX_PSEUDO} caracteres max)")
    print()
    pseudo = saisir("  > ", max_len=MAX_PSEUDO).strip()
    if not pseudo:
        pseudo = "ANONYME"

    lb = charger_leaderboard()
    entre = inserer_score(lb, pseudo, score)

    effacer()
    bandeau()
    print()
    if entre:
        print(centrer(f"Bravo {pseudo} !"))
        print(centrer("Vous entrez dans le TOP 10 !"))
    else:
        print(centrer("Score enregistre."))
    time.sleep(1.5)
    ecran_leaderboard()


def ecran_leaderboard():
    lb = charger_leaderboard()
    effacer()
    bandeau()
    print()
    print(centrer("** TOP 10 **"))
    print()
    ligne_h('=')
    print(f"  {'RANG':<6} {'PSEUDO':<10} {'SCORE'}")
    ligne_h('-')
    for i, s in enumerate(lb):
        medaille = ""
        if i == 0:   medaille = " [OR]"
        elif i == 1: medaille = " [AG]"
        elif i == 2: medaille = " [BR]"
        print(f"   {i+1:>2}.  {s['pseudo']:<10}  "
              f"{s['points']:>2} / {NB_QUESTIONS_PARTIE}{medaille}")
    ligne_h('=')
    pied_page("  [ENTREE] Retour a l'accueil")
    pause("")


def ecran_admin_mdp():
    effacer()
    bandeau()
    print()
    ligne_h('=')
    print(centrer("ACCES ADMINISTRATION"))
    ligne_h('=')
    print()
    try:
        import getpass
        mdp = getpass.getpass("  CODE D'ACCES : ")
    except Exception:
        mdp = input("  CODE D'ACCES : ")

    if mdp == MDP_ADMIN:
        ecran_admin_menu()
    else:
        print()
        print("  CODE INCORRECT !")
        time.sleep(1.5)


def ecran_admin_menu():
    while True:
        effacer()
        bandeau()
        print()
        ligne_h('=')
        print(centrer("PANNEAU DE CONTROLE"))
        ligne_h('=')
        print()
        print("  [ 1 ]  REINITIALISER LE LEADERBOARD")
        print()
        print("  [ 2 ]  AJOUTER UNE QUESTION")
        print()
        print("  [ 3 ]  VOIR / MODIFIER LES QUESTIONS")
        print()
        print("  [ 4 ]  RETOUR A L'ACCUEIL")
        print()
        pied_page("  Tapez 1, 2, 3 ou 4 puis ENTREE")
        choix = input("\n  > ").strip()

        if choix == '1':
            ecran_admin_confirm_reset()
        elif choix == '2':
            ecran_admin_ajout()
        elif choix == '3':
            ecran_admin_liste()
        elif choix == '4':
            return


def ecran_admin_confirm_reset():
    effacer()
    bandeau()
    print()
    print(centrer("ETES-VOUS SUR ?"))
    print(centrer("Cette action est irreversible !"))
    print()
    print(centrer("[O] CONFIRMER   [N] ANNULER"))
    choix = input("\n  > ").strip().upper()
    if choix == 'O':
        reinitialiser_leaderboard()
        print()
        print(centrer("LEADERBOARD REINITIALISE !"))
        time.sleep(1.5)


def ecran_admin_ajout():
    effacer()
    bandeau()
    print()
    print(centrer("AJOUT D'UNE QUESTION (1/3)"))
    ligne_h('-')
    print()
    print("  Saisissez l'enonce de la question :")
    enonce = saisir("  > ", max_len=72).strip()
    if not enonce:
        return

    effacer()
    bandeau()
    print()
    print(centrer("AJOUT D'UNE QUESTION (2/3)"))
    ligne_h('-')
    print()
    print("  Saisissez les 4 options :")
    print()
    choix = []
    for lettre in ['A', 'B', 'C', 'D']:
        c = saisir(f"  Option {lettre} : ", max_len=30).strip()
        choix.append(c if c else "---")

    effacer()
    bandeau()
    print()
    print(centrer("AJOUT D'UNE QUESTION (3/3)"))
    ligne_h('-')
    print()
    print("  Quelle est la bonne reponse ?")
    print()
    while True:
        rep = saisir("  Lettre (A, B, C ou D) : ").strip().upper()
        if rep in ('A', 'B', 'C', 'D'):
            break
        print("  Entrez A, B, C ou D uniquement.")

    q = {"enonce": enonce, "choix": choix, "reponse": rep}
    sauvegarder_question(q)
    print()
    print(centrer("QUESTION SAUVEGARDEE !"))
    time.sleep(1.5)


def modifier_question(index, q_modif):
    """Remplace la question a l'index donne et recrit le fichier."""
    banque = charger_questions()
    if index < 0 or index >= len(banque):
        return False
    banque[index] = q_modif
    with open(FICHIER_QUESTIONS, "w", encoding="utf-8") as f:
        for q in banque:
            f.write(q["enonce"] + "\n")
            for c in q["choix"]:
                f.write(c + "\n")
            f.write(q["reponse"] + "\n")
    return True


def ecran_admin_liste():
    page = 0
    par_page = 10

    while True:
        banque = charger_questions()
        effacer()
        bandeau()
        print()
        print(centrer("LISTE DES QUESTIONS EN BASE"))
        ligne_h('-')
        print()

        if not banque:
            print("  Aucune question enregistree.")
            pied_page("  [ENTREE] Retour au menu admin")
            pause("")
            return

        nb_pages = (len(banque) + par_page - 1) // par_page
        if page >= nb_pages:
            page = 0
        debut = page * par_page
        fin   = min(debut + par_page, len(banque))

        for i in range(debut, fin):
            apercu = banque[i]["enonce"][:30] + ".." \
                     if len(banque[i]["enonce"]) > 30 \
                     else banque[i]["enonce"]
            print(f"  Q{i+1:02d}: {apercu}")

        print()
        print(f"  Total : {len(banque)} question(s)   Page {page+1}/{nb_pages}")
        pied_page(
            "  Numero a editer, [S] Pg.suiv, [ENTREE] Menu"
        )
        choix = input("\n  > ").strip().upper()

        if choix == '':
            return
        elif choix == 'S':
            page += 1
            if page >= nb_pages:
                page = 0
        elif choix.isdigit():
            num = int(choix)
            if 1 <= num <= len(banque):
                ecran_admin_edition(num - 1, banque[num - 1])
            else:
                print("  Numero invalide !")
                time.sleep(1)


def ecran_admin_edition(index, q_orig):
    """Formulaire d'edition pre-rempli pour une question existante."""

    # --- Etape 1 : Enonce ---
    effacer()
    bandeau()
    print()
    print(centrer(f"EDITION Q{index+1:02d} (1/3)"))
    ligne_h('-')
    print()
    print("  Enonce actuel :")
    print(f"  {q_orig['enonce']}")
    print()
    print("  Nouvel enonce (ENTREE = garder) :")
    val = saisir("  > ", max_len=72).strip()
    enonce = val if val else q_orig["enonce"]

    # --- Etape 2 : Choix ---
    effacer()
    bandeau()
    print()
    print(centrer(f"EDITION Q{index+1:02d} (2/3)"))
    ligne_h('-')
    print()
    print("  Actuel -> Nouveau (ENTREE = garder)")
    print()
    choix = []
    for i, lettre in enumerate(['A', 'B', 'C', 'D']):
        print(f"  {lettre}/ actuel : {q_orig['choix'][i]}")
        val = saisir(f"  {lettre}/ nouveau : ", max_len=30).strip()
        choix.append(val if val else q_orig["choix"][i])
        print()

    # --- Etape 3 : Bonne reponse ---
    effacer()
    bandeau()
    print()
    print(centrer(f"EDITION Q{index+1:02d} (3/3)"))
    ligne_h('-')
    print()
    print(f"  Reponse actuelle : {q_orig['reponse']}")
    print()
    while True:
        val = saisir("  Nouvelle reponse (A/B/C/D, ENTREE = garder) : ").strip().upper()
        if val == '':
            reponse = q_orig["reponse"]
            break
        if val in ('A', 'B', 'C', 'D'):
            reponse = val
            break
        print("  Entrez A, B, C ou D uniquement.")

    q_modif = {"enonce": enonce, "choix": choix, "reponse": reponse}
    if modifier_question(index, q_modif):
        print()
        print(centrer("QUESTION MISE A JOUR !"))
    else:
        print()
        print(centrer("ERREUR DE SAUVEGARDE !"))
    time.sleep(1.5)


# =============================================================
# POINT D'ENTREE
# =============================================================

if __name__ == "__main__":
    print("  Chargement de 3615 RETROQUIZ...")
    # Verifications initiales
    if not os.path.exists(FICHIER_QUESTIONS):
        print(f"  ATTENTION : '{FICHIER_QUESTIONS}' introuvable.")
        print("  Placez ce fichier dans le meme dossier que simulateur.py")
        sys.exit(1)

    banque = charger_questions()
    print(f"  {len(banque)} question(s) chargee(s).")
    if not os.path.exists(FICHIER_LEADERBOARD):
        reinitialiser_leaderboard()
        print("  Leaderboard initialise.")
    time.sleep(1)
    ecran_accueil()
