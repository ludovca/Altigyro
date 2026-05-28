# 🚀 Altigyro : Ordinateur de Bord & Enregistreur pour Fusée Amateur

Ce dépôt contient le code source de l'ordinateur de bord (boîte noire) pour fusée amateur, développé en **C** pour le **Raspberry Pi Pico**, ainsi que les outils d'analyse de trajectoire 3D.

---

## 🛠️ Architecture Matérielle

L'ordinateur est conçu pour être embarqué dans la coiffe de la fusée et regroupe :

* **Calculateur :** Raspberry Pi Pico (RP2040).
* **Altimètre :** BMP280 (Pression & Température) via Bus I2C.
* **IMU 6-axes :** MPU6050 (Accéléromètre & Gyroscope) via Bus I2C.
* **Localisation :** Buzzer piézoélectrique pour signalisation sonore.
* **Stockage :** Mémoire Flash interne (LittleFS).
* **Rampe :** Tube aluminium Alberts (10x1mm, 2m) pour un guidage rigide.

---

## 🧠 Logique de l'Ordinateur (SDK C)

### 1. Enregistrement Dynamique
L'algorithme surveille l'accéléromètre pour optimiser l'espace mémoire :
* **Veille (Pré-vol) :** Enregistrement basse fréquence (50 ms) pour économiser la Flash.
* **Vol (Seuil > 2G) :** Passage automatique en mode haute fréquence (**10 ms**) dès la détection du décollage.

### 2. Post-Atterrissage : Récupération et Diagnostic
Une fois la descente terminée et l'immobilité détectée :
* **Balise Sonore :** La fusée émet des **bips intermittents** à haute fréquence pour faciliter sa localisation sur le terrain.

---

## 📊 Structure du fichier de données (`test.csv`)

Le pipeline d'analyse utilise un fichier **`test.csv`** structuré comme suit :

| Index | Colonne | Unité | Description |
| :--- | :--- | :--- | :--- |
| 0 | **t_ms** | ms | Temps depuis le démarrage |
| 1 | **P_Pa** | Pa | Pression atmosphérique |
| 2 | **Temp_C** | °C | Température de l'air |
| 3 | **Alt_m** | m | Altitude barométrique (Référence Y) |
| 4 | **AccX** | m/s² | Accélération latérale |
| 5 | **AccY** | m/s² | Accélération latérale |
| 6 | **AccZ** | m/s² | Axe de poussée moteur |
| 7 | **GyroX** | °/s | Rotation Tangage (Pitch) |
| 8 | **GyroY** | °/s | Rotation Lacet (Yaw) |
| 9 | **GyroZ** | °/s | Rotation Roulis (Roll) |

---

## 📈 Outils d'Analyse Post-Vol (Python)

*Scripts exécutés sur PC après récupération de la fusée.*

1. **Générateur de Graphiques :** Visualisation de la télémétrie brute (Altitude, Vitesse, Accélération).
2. **Reconstructeur 3D :** * Fusion de capteurs via **matrice de rotation**.
   * Compensation de la dérive par recalage barométrique.
   * Modélisation du déploiement du parachute et de la dérive au vent.
   * Export au format **`trajectoire_3D.txt`** (X Y Z) pour import dans **FreeCAD** ou **Blender**.
