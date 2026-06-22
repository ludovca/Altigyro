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
- MPU6050 : 100 Hz
- BMP280 : 100 Hz

---

## Suite logicielle post-vol

Installer les dépendances une seule fois :

```bash
pip install numpy pandas matplotlib
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
time, accel_x, accel_y, accel_z, accel_R_x, accel_R_y, accel_R_z, pressure, temp_C
```
---

### `\traj` — Altitude en fonction du temps & Trajectoire 3D

Trace le graphique altitude (m) vs temps (s) à partir de la pression barométrique ou du gyroscope.

**Fonctionnalités :**
- Calcul altitude via formule barométrique : `h = 44330 × (1 − (P/P0)^(1/5.255))`
- Détection automatique du décollage, de l'apogée et de l'atterrissage
- Marqueurs décollage / apogée / atterrissage

---


Reconstitue et trace la trajectoire spatiale de la fusée par intégration double de l'accélération.

**Fonctionnalités :**
- Soustraction de la gravité (calibration au sol sur les 100 premières trames)
- Intégration accélération → vitesse → position
- Tracé 3D interactif (rotation, zoom)
- Marqueurs décollage / apogée / atterrissage

> **Note :** L'intégration double accumule de la dérive. Sans filtre de Kalman (prévu quand le MPU6050 sera calibré), la trajectoire 3D est indicative. L'altitude barométrique reste la référence fiable.

---

### `\orientation_viewer` — Visualiseur d'inclinaison (boîte noire)

Rejoue l'orientation de la fusée frame par frame, comme une vidéo, à partir des données gyroscope.

**Fonctionnalités :**
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
