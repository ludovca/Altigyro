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

## 📄 Structure du Fichier de Log (`vol_data.json`)

Le format généré par le code en C respecte scrupuleusement la structure suivante :

```json
{
  "infos_vol": {
    "statut": "Termine",
    "frequence_hz": 50,
    "t0_decollage_ms": 25400
    "tf_atterrissage_ms": 35000
  },
  "resume_analyse": {
    "altitude_max_m": 222.2,
    "vitesse_max_ms": 67.6,
    "acceleration_max_verticale": 12.4,
    "acceleration_max": 17.4,
    "temps_montée_ms" : 6000
    "temps_descente_ms" : 30000
  },
  "donnies_brutes": [
    [25380, 0.0, 0.98, 0.01, 0.02, 0.1, 0.0, 0.2],
    [25400, 0.1, 2.15, 0.05, -0.12, 1.5, 0.5, 12.0],
    [25420, 0.4, 4.80, 0.12, -0.45, 5.0, 2.1, 45.0]
  ]
}
```
---

## 📊 Outils d'Analyse Post-Vol (Sur PC)

*Note : Ces scripts s'exécutent sur ordinateur après la récupération de la fusée. Ils ne sont pas chargés sur le Raspberry Pi Pico.*

Le dépôt inclut des outils de traitement de données qui exploitent le fichier `vol_data.txt` généré par la boîte noire :

1. **Générateur de Graphiques :** Un script qui trace automatiquement l'évolution de la **hauteur en fonction du temps** pour visualiser précisément les phases de poussée, de dérive, d'apogée et de descente sous parachute.
2. **Reconstructeur de Trajectoire 3D :** Un script de fusion de capteurs (Intégration de l'accéléromètre et calcul d'attitude via le gyroscope recalé par l'altimètre). 
   * Cet outil extrait le plus de points de mesure possibles pour modéliser la vraie trajectoire de la fusée dans l'espace.
   * Il génère un fichier texte (`trajectoire_3D.txt`) contenant une liste de coordonnées cartésiennes $(X, Y, Z)$. Ce fichier est directement prêt à être importé dans un logiciel de CAO ou de modélisation (comme **FreeCAD**) pour afficher le nuage de points ou tracer la courbe 3D exacte du vol.
