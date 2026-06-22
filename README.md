# 🚀 Altigyro : Ordinateur de Bord & Enregistreur pour Fusée Amateur

Ce dépôt contient le code source de l'ordinateur de bord (boîte noire) pour fusée amateur, développé en **C** pour le **Raspberry Pi Pico**, ainsi que les outils d'analyse de trajectoire 3D.

---

## Matériel

| Composant | Rôle | Interface |
|-----------|------|-----------|
| Raspberry Pi Pico 1 | Microcontrôleur principal | — |
| BMP280 | Pression / Température → Altitude | I2C (0x76) |
| MPU6050 | Accéléromètre + Gyroscope 6 axes | I2C (0x68) |
| Module carte SD | Stockage des données de vol | SPI |
| Buzzer actif | Signaux sonores (démarrage, apogée) | GPIO |

---

## Câblage

### I2C — BMP280 + MPU6050 (bus partagé)

| Capteur | Pin capteur | Pin Pico | N° pin |
|---------|-------------|----------|--------|


### SPI — Module carte SD

| Module SD | Pin Pico | N° pin |
|-----------|----------|--------|


### GPIO

| Composant | Pin Pico | N° pin |
|-----------|----------|--------|


---

## Configuration (`altimetre.ino`)

```cpp
#define FREQUENCE_HZ      100       // Fréquence boucle principale (MPU6050)
#define BMP_DIVIDER       2         // BMP280 à 100 Hz (1 lecture / 2 boucles)
#define BUFFER_RECORDS    40        // Flush SD toutes les 40 trames → 5x/sec
#define PRESSION_MER      1013.25f  // hPa — à ajuster selon météo locale
#define APOGEE_DELTA      0.15f     // hPa de remontée pour confirmer l'apogée
```

> **Régler `PRESSION_MER`** avant chaque session avec la pression locale ramenée au niveau de la mer (disponible sur meteoblue.com ou météo locale).

---

## Signaux buzzer

| Signal | Signification |
|--------|---------------|
| 2 bips courts au démarrage | Tous les capteurs OK, prêt au vol |
| 5 bips longs | Erreur capteur ou carte SD — vérifier câblage |
| 1 bip court au décollage | Décollage détecté (chute de pression > 1 hPa) |
| 3 bips longs à l'apogée | Apogée détectée |

---

## Format des données

### Fichier binaire `.bin`

Chaque vol génère un fichier `VOL_001.bin`, `VOL_002.bin`, etc. (incrémentation automatique, aucun écrasement).

**En-tête (8 octets) :**
```
46 55 53 45  →  "FUSE" (magic number)
01           →  version 1
24           →  36 octets par record
00 00        →  padding
```

**Structure d'un record (36 octets) :**

| Champ | Type | Octets | Unité |
|-------|------|--------|-------|
| timestamp | uint32 | 4 | ms |
| ax | float | 4 | g |
| ay | float | 4 | g |
| az | float | 4 | g |
| gx | float | 4 | °/s |
| gy | float | 4 | °/s |
| gz | float | 4 | °/s |
| pression | float | 4 | hPa (NaN si non relu) |
| temperature | float | 4 | °C (NaN si non relu) |

**Fréquences d'enregistrement :**
- MPU6050 : 200 Hz → toutes les lignes
- BMP280 : 100 Hz → 1 ligne sur 2 (les autres ont `NaN`)

---

## Suite logicielle post-vol

Tous les scripts sont dans le dossier `tools/`.  
Installer les dépendances une seule fois :

```bash
pip install numpy pandas matplotlib scipy pyqt5 pyopengl
```

---

### `convert.py` — Conversion binaire → CSV

Convertit le fichier `.bin` brut en `.csv` exploitable.

```bash
python tools/convert.py VOL_001.bin
# → génère VOL_001.csv
```

**Colonnes du CSV généré :**
```
timestamp_ms, ax_g, ay_g, az_g, gx_dps, gy_dps, gz_dps, pression_hPa, temp_C
```

Les cellules pression/temp sont vides sur les trames sans lecture BMP280.

---

### `altitude.py` — Altitude en fonction du temps

Trace le graphique altitude (m) vs temps (ms) à partir de la pression barométrique.

```bash
python tools/altitude.py VOL_001.csv
```

**Fonctionnalités :**
- Interpolation des valeurs BMP280 manquantes (100 Hz → 200 Hz)
- Calcul altitude via formule barométrique : `h = 44330 × (1 − (P/P0)^(1/5.255))`
- Détection automatique du décollage, de l'apogée et de l'atterrissage
- Affichage de la vitesse verticale (dérivée de l'altitude)
- Export PNG du graphique

---

### `trajectory3d.py` — Trajectoire 3D

Reconstitue et trace la trajectoire spatiale de la fusée par intégration double de l'accélération.

```bash
python tools/trajectory3d.py VOL_001.csv
```

**Fonctionnalités :**
- Soustraction de la gravité (calibration au sol sur les 200 premières trames)
- Intégration accélération → vitesse → position
- Tracé 3D interactif (rotation, zoom)
- Repère : X = Est, Y = Nord, Z = Altitude
- Marqueurs décollage / apogée / atterrissage

> **Note :** L'intégration double accumule de la dérive. Sans filtre de Kalman (prévu quand le MPU6050 sera calibré), la trajectoire 3D est indicative. L'altitude barométrique reste la référence fiable.

---

### `orientation_viewer.py` — Visualiseur d'inclinaison (boîte noire)

Rejoue l'orientation de la fusée frame par frame, comme une vidéo, à partir des données gyroscope.

```bash
python tools/orientation_viewer.py VOL_001.csv
```

**Fonctionnalités :**
- Rendu 3D OpenGL d'un modèle fusée
- Intégration du gyroscope (gx, gy, gz) pour reconstruire l'orientation
- Contrôles :
  - `Espace` — lecture / pause
  - `←` `→` — avancer / reculer trame par trame
  - `+` `-` — vitesse de lecture (0.25× à 4×)  
  - `R` — revenir au début
- Barre de progression avec timestamp en ms
- Indicateur d'inclinaison en degrés (roulis, tangage, lacet)

> Utile pour analyser la stabilité de vol et détecter des rotations non souhaitées.

---

## Structure du projet

```
fusee-altimetre/
│
├── altimetre.ino               ← Code Arduino (Pico)
│
├── tools/
│   ├── convert.py              ← Binaire → CSV
│   ├── altitude.py             ← Graphique altitude/temps
│   ├── trajectory3d.py         ← Trajectoire 3D
│   └── orientation_viewer.py   ← Visualiseur inclinaison
│
├── data/
│   ├── VOL_001.bin             ← Données brutes (SD card)
│   ├── VOL_001.csv             ← Données converties
│   └── ...
│
└── README.md
```

---

## Workflow complet après un vol

```
1. Retirer la carte SD de la boîte noire
2. Copier VOL_XXX.bin dans data/
3. python tools/convert.py data/VOL_XXX.bin
4. python tools/altitude.py data/VOL_XXX.csv
5. python tools/trajectory3d.py data/VOL_XXX.csv
6. python tools/orientation_viewer.py data/VOL_XXX.csv
```

---

## Dépendances Arduino IDE

- Board : **Raspberry Pi Pico** via le board manager *Earle Philhower* (`https://github.com/earlephilhower/arduino-pico`)
- Librairie : **SD** (incluse dans Arduino IDE)
- Librairie : **Wire** (incluse dans Arduino IDE)

---

## Limites connues

- Le BMP280 est limité à ~100 Hz en pratique (temps de conversion ~8 ms)
- L'intégration gyroscope dérive dans le temps — l'orientation absolue n'est fiable que sur des vols courts (< 30 sec)
- Sans filtre de Kalman, la trajectoire 3D est approximative
- La détection d'apogée par pression peut être retardée de ~20 ms (délai lecture BMP)
