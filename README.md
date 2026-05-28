# 🚀 Altigyro : Ordinateur de Bord & Enregistreur pour Fusée Amateur

Ce dépôt contient le code source de l'ordinateur de bord (boîte noire) pour fusée amateur, développé en **C** pour le **Raspberry Pi Pico**, ainsi que les outils d'analyse de trajectoire 3D. Le système enregistre les données en flux continu pour une robustesse maximale en cas de crash.

---

## 🛠️ Architecture Matérielle

L'ordinateur est conçu pour être embarqué dans la coiffe de la fusée et regroupe :

* **Calculateur :** Raspberry Pi Pico (RP2040).
* **Altimètre :** BMP280 (Pression & Température) via Bus I2C.
* **IMU 6-axes :** MPU6050 (Accéléromètre & Gyroscope) via Bus I2C.
* **Stockage :** Mémoire Flash interne du RP2040 (FatFS / LittleFS).
* **Alimentation :** Batterie LiPo 1S.
* **Rampe de lancement :** Tube aluminium Alberts (10x1mm, 2m) pour un guidage rigide et précis.

---

## 🧠 Logique de l'Ordinateur (SDK C)

### 1. Enregistrement Résilient
Les données sont écrites en flux continu sur la Flash. En cas de coupure électrique à l'atterrissage, les données sont préservées grâce à des appels fréquents à `f_sync`.

### 2. Détection dynamique du décollage ($t_0$)
L'algorithme surveille l'accéléromètre en temps réel :
* **Veille :** Enregistrement toutes les 50 ms.
* **Vol (Seuil > 2G) :** Passage automatique à un échantillonnage haute fréquence toutes les **10 ms**.

---

## 📊 Structure du fichier de données (`test.csv`)

Pour l'analyse post-vol, les données brutes doivent être converties ou extraites au format **`test.csv`** avec les 10 colonnes suivantes :

| Index | Colonne | Unité | Description |
| :--- | :--- | :--- | :--- |
| 0 | **t_ms** | ms | Temps depuis le démarrage |
| 1 | **P_Pa** | Pa | Pression atmosphérique |
| 2 | **Temp_C** | °C | Température de l'air |
| 3 | **Alt_m** | m | Altitude barométrique (Référence axe Y) |
| 4 | **AccX** | m/s² | Accélération latérale (Capteur) |
| 5 | **AccY** | m/s² | Accélération latérale (Capteur) |
| 6 | **AccZ** | m/s² | Axe de poussée (Capteur) |
| 7 | **GyroX** | °/s | Rotation Tangage (Pitch) |
| 8 | **GyroY** | °/s | Rotation Lacet (Yaw) |
| 9 | **GyroZ** | °/s | Rotation Roulis (Roll) |

---

## 📈 Outils d'Analyse Post-Vol (Python)

*Ces scripts s'exécutent sur PC après récupération de la fusée.*

1. **Générateur de Graphiques :** Trace l'évolution de la hauteur/temps (phases de poussée, apogée, descente).
2. **Reconstructeur 3D :** * Utilise une **matrice de rotation** pour fusionner les données Gyro/Accéo.
   * Recale l'axe vertical sur le baromètre pour annuler la dérive.
   * Gère le déploiement du parachute (redressement de l'attitude) et la dérive due au vent.
   * Génère un fichier **`trajectoire_3D.txt`** (nuage de points X Y Z) importable dans **FreeCAD** ou **Blender**.

---

## 🚀 Utilisation rapide

1. Récupérez le fichier de données de la Flash du Pico.
2. Nommez-le `test.csv` et placez-le dans le dossier des scripts Python.
3. Lancez l'analyse :
   ```bash
   python calcul_trajectoire.py
