# Altigyro Ordinateur de Bord Enregistreur pour Fusée Amateur

Ce dépôt contient le code source de l'ordinateur de bord (boîte noire) pour fusée amateur, développé en **C** pour le **Raspberry Pi Pico**. Le système enregistre en continu les données télémétriques au format **JSON** pour une exploitation simplifiée et une robustesse maximale.

---

## 🛠️ Architecture Matérielle

L'ordinateur est conçu pour être embarqué dans la coiffe de la fusée et regroupe :

* **Calculateur :** Raspberry Pi Pico (RP2040).
* **Altimètre :** BMP280 (Pression & Température) - Bus I2C.
* **IMU 6-axes :** GY-521 / MPU6050 (Accéléromètre & Gyroscope) - Bus I2C.
* **Stockage :** Mémoire Flash interne du RP2040 (via FatFS / LittleFS).
* **Alimentation :** Batterie LiPo 1S.

---

## 🧠 Logique de l'Ordinateur (SDK C)

Le programme est optimisé pour la performance et la sécurité des données :

### 1. Enregistrement JSON en Flux Continu
L'ordinateur écrit les données directement sur la Flash sans interruption. Le choix du format **JSON** permet une structure de données auto-descriptive :
* **Écriture temps réel :** Chaque échantillon est immédiatement "flushé" (`f_sync` ou `fflush`) sur la Flash.
* **Résilience :** Même en cas de coupure électrique à l'atterrissage ou de crash, le fichier reste lisible et les données sont préservées.
* **Indépendance :** L'enregistrement ne s'arrête jamais, capturant les événements avant, pendant et après le vol.

### 2. Détection d'Événements ($t_0$)
L'algorithme analyse le flux de l'accéléromètre en tâche de fond. Dès qu'un seuil de **2 G** est franchi, le système marque l'index temporel **$t_0$** dans le fichier. Ce marqueur permet aux outils d'analyse de synchroniser la phase de propulsion (moteur D12-4) avec les données brutes.

### 3. Rapport d'Atterrissage Automatique
Dès que l'altimètre détecte une stabilité verticale prolongée (> 5s), l'ordinateur calcule et insère dynamiquement un objet JSON de résumé dans le flux, contenant l'altitude maximale ($H_{max}$), la vitesse de pointe ($V_{max}$), l'accélération de crête ($G_{max}$) et la vitesse de rotation maximale.

---

## 📊 Structure du fichier de données (`test.csv`)

Le script lit les données brutes depuis un fichier nommé **`test.csv`**. Ce fichier doit contenir **10 colonnes** séparées par des virgules.

| Index | Colonne | Unité | Description |
| :--- | :--- | :--- | :--- |
| 0 | **t_ms** | ms | Temps écoulé depuis le démarrage |
| 1 | **P_Pa** | Pa | Pression atmosphérique (BMP280) |
| 2 | **Temp_C** | °C | Température de l'air |
| 3 | **Alt_m** | m | Altitude barométrique (Référence axe Y) |
| 4 | **AccX** | m/s² | Accélération latérale (Capteur) |
| 5 | **AccY** | m/s² | Accélération latérale (Capteur) |
| 6 | **AccZ** | m/s² | Axe de poussée (Capteur) |
| 7 | **GyroX** | °/s | Vitesse angulaire (Tangage / Pitch) |
| 8 | **GyroY** | °/s | Vitesse angulaire (Lacet / Yaw) |
| 9 | **GyroZ** | °/s | Vitesse angulaire (Roulis / Roll) |
---

## 📊 Outils d'Analyse Post-Vol (Sur PC)

*Note : Ces scripts s'exécutent sur ordinateur après la récupération de la fusée. Ils ne sont pas chargés sur le Raspberry Pi Pico.*

Le dépôt inclut des outils de traitement de données qui exploitent le fichier `vol_data.txt` généré par la boîte noire :

1. **Générateur de Graphiques :** Un script qui trace automatiquement l'évolution de la **hauteur en fonction du temps** pour visualiser précisément les phases de poussée, de dérive, d'apogée et de descente sous parachute.
2. **Reconstructeur de Trajectoire 3D :** Un script de fusion de capteurs (Intégration de l'accéléromètre et calcul d'attitude via le gyroscope recalé par l'altimètre). 
   * Cet outil extrait le plus de points de mesure possibles pour modéliser la vraie trajectoire de la fusée dans l'espace.
   * Il génère un fichier texte (`trajectoire_3D.txt`) contenant une liste de coordonnées cartésiennes $(X, Y, Z)$. Ce fichier est directement prêt à être importé dans un logiciel de CAO ou de modélisation (comme **FreeCAD**) pour afficher le nuage de points ou tracer la courbe 3D exacte du vol.
