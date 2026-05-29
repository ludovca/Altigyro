# 🚀 Altigyro : Ordinateur de Bord & Enregistreur pour Fusée Amateur

Ce dépôt contient le code source de l'ordinateur de bord (boîte noire) pour fusée amateur, développé en **C** pour le **Raspberry Pi Pico**, ainsi que les outils d'analyse de trajectoire 3D.

---

## 🛠️ Architecture Matérielle

L'ordinateur est conçu pour être embarqué dans la coiffe de la fusée et regroupe :

* **Calculateur :** Raspberry Pi Pico (RP2040).
* **Altimètre :** BMP280 (Pression & Température) via Bus I2C.
* **IMU 6-axes :** MPU6050 (Accéléromètre & Gyroscope) via Bus I2C.
* **Localisation :** Buzzer piézoélectrique pour signalisation sonore.

---

## 🧠 Logique de l'Ordinateur (SDK C)

### 2. Gestion intelligente du stockage (Buffer Circulaire)
Pour maximiser l'espace en Flash, l'ordinateur utilise un tampon glissant de **10 échantillons** :
* **Pré-vol :** Les données tournent en RAM. Si la variation entre le premier et le dernier échantillon est négligeable, la mémoire Flash n'est pas sollicitée.
* **Déclenchement ($t_0$) :** Dès qu'une différence significative (accélération > 2G) est détectée, le buffer est vidé vers la Flash et l'enregistrement haute fréquence commence.
* **Résultat :** Le fichier `test.csv` ne contient que le vol utile, optimisant les 2Mo de stockage.

### 2. Détection de fin de vol et récupération
Pour éviter un arrêt prématuré (notamment à l'apogée), le système confirme l'atterrissage via une double validation stricte :
* **Delta t de sécurité :** La détection de fin de vol est **bloquée** pendant les **1 minute 30** suivant le décollage. Cela garantit l'enregistrement complet, même en cas de vol plané prolongé.
* **Stabilité Altigraphique :** Après ce délai, l'altitude barométrique doit être stable (variation proche de zéro) pendant **10 secondes consécutives**.
* **Signal de récupération :** Une fois le vol validé terminé, l'enregistrement s'arrête et le buzzer émet des bips haute fréquence pour la localisation.

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
