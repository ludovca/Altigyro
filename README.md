# Altigyro Ordinateur de Bord Enregistreur pour Fusée Amateur

Ce dépôt contient le code de l'ordinateur de bord (boîte noire) développé en **C** (via le SDK Raspberry Pi Pico). Son rôle est d'enregistrer en continu et sans interruption les paramètres dynamiques du vol, puis d'y injecter un rapport d'analyse automatique dès que l'atterrissage est détecté, tout en poursuivant l'acquisition des données au sol.

---

## 🛠️ Architecture Matérielle

L'ordinateur de bord est conçu pour être embarqué de manière autonome et s'articule autour des composants suivants :

*   **Microcontrôleur :** Raspberry Pi Pico (RP2040).
*   **Altimètre :** Capteur de pression et température **BMP280** (Liaison I2C).
*   **Centrale à inertie (IMU) :** Accéléromètre & Gyroscope 3 axes **GY-521 / MPU6050** (Liaison I2C).
*   **Alimentation :** Batterie LiPo 1S légère.

---

## 🧠 Logique du Code & Sécurisation des Données

Développé en **C**, le code tire parti des performances du RP2040 pour maximiser la vitesse d'acquisition tout en garantissant la persistance des données face aux risques du vol :

### 1. Enregistrement Continu et Infini (Principe de la Boîte Noire)
*   Dès son démarrage au sol, l'ordinateur initialise le système de fichiers (sur la mémoire Flash) et ouvre le fichier de log (`vol_data.txt` / `.csv`).
*   Il y écrit immédiatement chaque ligne de données lue sur les capteurs à haute fréquence ($\approx 50\text{ à }100\text{ Hz}$).
*   **Zéro coupure :** L'enregistrement continue tout le long du vol **et même après l'atterrissage** (pas de fermeture du flux). Pour sécuriser les données en temps réel, la mémoire Flash est synchronisée de manière logicielle à intervalles réguliers (via `f_sync()` ou `fflush()`), protégeant les logs contre les chocs ou les défaillances électriques.

### 2. Détection du Décollage ($t_0$)
*   L'ordinateur surveille l'accéléromètre en arrière-plan. Lorsqu'un pic vertical supérieur à $2\text{ G}$ se produit, le script enregistre l'index temporel exact de cet événement.
*   Ce **Top Décollage ($t_0$)** sert de point de référence pour isoler la phase active du vol par rapport au bruit de fond et à l'attente sur la rampe.

### 3. Insertion du Rapport à l'Atterrissage
*   L'ordinateur surveille en parallèle la stabilisation de l'altitude barométrique. Lorsqu'un calme plat est détecté pendant plus de 5 secondes, la phase de vol est considérée comme terminée.
*   **Génération du rapport à la volée :** Sans jamais interrompre ni fermer le flux d'enregistrement continu, le programme calcule les statistiques clés du vol ($H_{max}$, $V_{max}$, $G_{max}$, $A_{moyenne}$...) et vient insérer cette section de **conclusion automatique** dans un nouveau fichier `vol_conclusion_auro.txt` / `.csv`. L'acquisition des données brutes continue même après la conclusion pour être sur de ne raté aucune mesure.

---

## 📊 Outils d'Analyse Post-Vol (Sur PC)

*Note : Ces scripts s'exécutent sur ordinateur après la récupération de la fusée. Ils ne sont pas chargés sur le Raspberry Pi Pico.*

Le dépôt inclut des outils de traitement de données qui exploitent le fichier `vol_data.txt` généré par la boîte noire :

1. **Générateur de Graphiques :** Un script qui trace automatiquement l'évolution de la **hauteur en fonction du temps** pour visualiser précisément les phases de poussée, de dérive, d'apogée et de descente sous parachute.
2. **Reconstructeur de Trajectoire 3D :** Un script de fusion de capteurs (Intégration de l'accéléromètre et calcul d'attitude via le gyroscope recalé par l'altimètre). 
   * Cet outil extrait le plus de points de mesure possibles pour modéliser la vraie trajectoire de la fusée dans l'espace.
   * Il génère un fichier texte (`trajectoire_3D.txt`) contenant une liste de coordonnées cartésiennes $(X, Y, Z)$. Ce fichier est directement prêt à être importé dans un logiciel de CAO ou de modélisation (comme **FreeCAD**) pour afficher le nuage de points ou tracer la courbe 3D exacte du vol.
