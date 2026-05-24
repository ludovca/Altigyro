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
Dès que l'altimètre détecte une stabilité verticale prolongée (> 5s), l'ordinateur calcule et insère dynamiquement un objet JSON de résumé dans le flux, contenant :
* L'altitude maximale ($H_{max}$) et la vitesse de pointe ($V_{max}$).
* L'accélération de crête ($G_{max}$) et la vitesse de rotation maximale.

---

## 📄 Structure du Fichier de Log (`vol_data.json`)

Le format est conçu pour être lu directement par n'importe quel langage moderne (Python, JS, C++) :

```json
{
  "header": { "fusee": "Pico-Rocket", "moteur": "D12-4", "freq_hz": 50 },
  "logs": [
    { "t": 1250, "alt": 0.0, "acc": [0.01, 0.02, 0.98], "gyro": [0.1, 0.0, 0.0] },
    { "t": 1270, "alt": 0.2, "acc": [0.05, -0.01, 2.15], "gyro": [1.2, 0.5, 0.2] }
  ],
  "summary": { "apogee_m": 222.2, "v_max_ms": 67.6, "g_max": 12.4 }
}

---

## 📊 Outils d'Analyse Post-Vol (Sur PC)

*Note : Ces scripts s'exécutent sur ordinateur après la récupération de la fusée. Ils ne sont pas chargés sur le Raspberry Pi Pico.*

Le dépôt inclut des outils de traitement de données qui exploitent le fichier `vol_data.txt` généré par la boîte noire :

1. **Générateur de Graphiques :** Un script qui trace automatiquement l'évolution de la **hauteur en fonction du temps** pour visualiser précisément les phases de poussée, de dérive, d'apogée et de descente sous parachute.
2. **Reconstructeur de Trajectoire 3D :** Un script de fusion de capteurs (Intégration de l'accéléromètre et calcul d'attitude via le gyroscope recalé par l'altimètre). 
   * Cet outil extrait le plus de points de mesure possibles pour modéliser la vraie trajectoire de la fusée dans l'espace.
   * Il génère un fichier texte (`trajectoire_3D.txt`) contenant une liste de coordonnées cartésiennes $(X, Y, Z)$. Ce fichier est directement prêt à être importé dans un logiciel de CAO ou de modélisation (comme **FreeCAD**) pour afficher le nuage de points ou tracer la courbe 3D exacte du vol.
