# 🔥 FireBot — Voiture Robot IA de Détection d'Incendie Industriel

<div align="center">

![ISSAT Sousse](https://img.shields.io/badge/ISSAT-Sousse-1B6B47?style=for-the-badge)
![ESP32](https://img.shields.io/badge/ESP32-IoT-E7352C?style=for-the-badge&logo=espressif)
![PyTorch](https://img.shields.io/badge/PyTorch-Deep%20Learning-EE4C2C?style=for-the-badge&logo=pytorch)
![Python](https://img.shields.io/badge/Python-3.x-3776AB?style=for-the-badge&logo=python)
![Arduino](https://img.shields.io/badge/Arduino-C++-00979D?style=for-the-badge&logo=arduino)

**Projet académique réalisé à l'ISSAT Sousse — Encadré par Dr. AMMAR Anis**

</div>

---

## 📌 Description

**FireBot** est une plateforme robotique autonome conçue pour la **détection de flammes en temps réel** dans les environnements industriels (usines, entrepôts).

Le système combine **IoT (ESP32)**, **vision par ordinateur (ESP32-CAM)** et **Deep Learning (PyTorch/CNN)** pour détecter, localiser et alerter en cas d'incendie — là où les détecteurs fixes échouent.

---

## 👥 Équipe du Projet

| Nom | Rôle |
|-----|------|
| Mohamed Amine Messelmani | Embedded Systems & IA |
| Taha Hemdi | Hardware & Câblage |
| Hassen Chaouch | Firmware ESP32 |
| Akrimi Oussama | Modèle IA & Tests |

> **Encadrant :** Dr. AMMAR Anis — ISSAT Sousse

---

## 🎯 Objectifs

- Détecter rapidement une source de feu par caméra
- Localiser la flamme avec des bounding boxes
- Envoyer une alerte en temps réel
- Permettre une intervention rapide à distance

---

## 🏗️ Architecture du Système

```
[ESP32-CAM] ──Wi-Fi──► [Serveur Python / IA PyTorch] ──► [Détection Flamme]
                                                               │
[ESP32 DevKit] ◄──────────────────────────────────────── [Commande Robot]
     │
     ▼
[L298N] ──► [Moteurs DC]  +  [Servo SG90] ──► [Orientation Caméra]
```

**Flux de données :**
1. L'ESP32-CAM capture le flux vidéo en continu
2. Les images sont envoyées via Wi-Fi au serveur Python
3. Le modèle PyTorch analyse chaque frame
4. En cas de flamme détectée → alerte + bounding box affichée
5. L'opérateur pilote le robot via l'interface web

---

## 🛠️ Composants Matériels

| Composant | Rôle | Quantité |
|-----------|------|----------|
| ESP32 DevKit | Cerveau du robot (contrôle moteurs + serveur web) | 1 |
| ESP32-CAM | Vision + transmission flux vidéo | 1 |
| L298N | Driver moteurs DC | 1 |
| Moteur DC | Déplacement de la plateforme | 2 |
| Servomoteur SG90/MG90S | Orientation de la caméra (Pan) | 1 |
| Batterie Li-Ion 7.4V | Alimentation principale | 1 |

---

## 🔌 Câblage Détaillé

### Connexions ESP32 ↔ L298N

| GPIO ESP32 | Broche L298N | Moteur | Fonction |
|------------|--------------|--------|----------|
| GPIO 14 | IN1 | Moteur A | Sens avant |
| GPIO 27 | IN2 | Moteur A | Sens arrière |
| GPIO 25 | IN3 | Moteur B | Sens avant |
| GPIO 33 | IN4 | Moteur B | Sens arrière |
| GPIO 26 | ENA | Moteur A | Enable (HIGH fixe) |
| GPIO 32 | ENB | Moteur B | Enable (HIGH fixe) |
| GND | GND | — | Masse commune |

### Connexion Servomoteur

| Fil servo | Couleur | Connexion ESP32 |
|-----------|---------|-----------------|
| Signal PWM | Orange | GPIO 13 |
| Alimentation | Rouge | VIN (5V) |
| Masse | Marron | GND |

> ⚠️ **Note :** Si le servo vibre à l'arrêt, ajouter un condensateur 100µF entre VIN et GND pour stabiliser l'alimentation.

---

## 💻 Interface Web de Contrôle

L'interface web embarquée dans l'ESP32 permet :
- Pilotage directionnel du robot (avant / arrière / gauche / droite / stop)
- Contrôle de l'orientation de la caméra via un slider (0° → 180°)
- Affichage du statut en temps réel (état moteurs, angle servo)
- Chatbot intégré **"Amine Bot"** pour envoyer des commandes vocales textuelles

**Accès :** `http://<IP_ESP32>/` depuis n'importe quel appareil sur le même réseau Wi-Fi.

---

## 🧠 Intelligence Artificielle

### Pipeline IA

```
[Dataset Roboflow]
        │
        ▼
[Annotation & Augmentation]
        │
        ▼
[Entraînement sur Kaggle (GPU)]
   CNN PyTorch — architecture :
   Convolution → Pooling → Fully Connected
        │
        ▼
[Export modèle .pt]
        │
        ▼
[Déploiement sur Serveur Python local]
        │
        ▼
[Inférence temps réel sur flux ESP32-CAM]
```

### Architecture CNN utilisée

- **Convolution** : détection des caractéristiques (bords, formes, couleurs de flamme)
- **Pooling** : réduction dimensionnelle pour accélérer le calcul
- **Fully Connected** : décision finale → `fire` / `not fire` / `background`

### Dataset

- Source : **Roboflow** — dataset "Fire Detection from CCTV"
- Taille recommandée : **3000 images étiquetées** par classe
- Proportion images arrière-plan : 0–10%
- Entraînement réalisé sur **Kaggle** (GPU T4)

### Performances du Modèle

| Classe | Précision |
|--------|-----------|
| Fire | **0.96** |
| Not Fire | **0.93** |
| Background | — |

> Le modèle atteint **96% de précision** sur la détection de flammes (matrice de confusion normalisée sur 3000 images de test).

---

## 📁 Structure du Dépôt

```
FireBot-AI-Industrial-Detection/
│
├── Firmware/
│   ├── esp32_robot_control/      # Code Arduino ESP32 (moteurs + interface web)
│   └── esp32cam_stream/          # Code Arduino ESP32-CAM (flux MJPEG)
│
├── AI_Server/
│   ├── script.py                 # Serveur Python + inférence PyTorch temps réel
│   └── model_fire.pt             # Modèle PyTorch entraîné (à télécharger)
│
├── Assets/
│   ├── wiring_diagram.png        # Schéma de câblage complet
│   ├── web_interface.png         # Capture interface de contrôle
│   ├── confusion_matrix.png      # Matrice de confusion du modèle
│   ├── ai_pipeline.png           # Schéma pipeline IA
│   └── robot_cover.png           # Photo du robot FireBot
│
├── Docs/
│   └── Voiture_Robot_IA_Detection_Feu.pdf   # Présentation complète du projet
│
└── README.md
```

---

## 🚀 Installation & Démarrage

### 1. Firmware ESP32 (Arduino IDE)

```bash
# Bibliothèques nécessaires (Library Manager Arduino) :
# - ESP32 Board Package (par Espressif)
# - ESP32Servo
```

Ouvrir `Firmware/esp32_robot_control/esp32_robot_control.ino` et modifier :

```cpp
const char* ssid     = "TON_WIFI";
const char* password = "TON_MOT_DE_PASSE";
```

Flasher sur l'ESP32 DevKit.

### 2. Firmware ESP32-CAM

Ouvrir `Firmware/esp32cam_stream/esp32cam_stream.ino` et modifier les identifiants Wi-Fi.
Flasher sur l'ESP32-CAM (penser à mettre GPIO0 à GND pendant le flash).

### 3. Serveur IA Python

```bash
# Installer les dépendances
pip install torch torchvision opencv-python flask numpy

# Lancer le serveur
cd AI_Server/
python script.py
```

Le serveur écoute sur `http://0.0.0.0:5000` et traite le flux MJPEG de l'ESP32-CAM.

---

## 🔮 Améliorations Futures

| Amélioration | Technologie | Budget estimé |
|-------------|-------------|---------------|
| Caméra haute résolution | Raspberry Pi Camera v2 | ~800 DT |
| Modèle plus performant | YOLOv8 en HD temps réel | — |
| Cartographie de l'environnement | SLAM (ROS) | — |
| Alerte distante | Firebase + Telegram Bot | — |
| Navigation autonome | Capteurs ultrasoniques + PID | — |

---

## 📊 Résultats

- ✅ Détection de flammes en temps réel via flux vidéo
- ✅ Interface web fonctionnelle (contrôle moteurs + caméra)
- ✅ Modèle PyTorch avec **96% de précision**
- ✅ Architecture modulaire et extensible
- ✅ Démonstation en conditions réelles

---

## 📄 Licence

Ce projet est réalisé à des fins académiques à l'**ISSAT Sousse**.
Libre d'utilisation pour l'apprentissage et la recherche.

---

<div align="center">

**🔥 FireBot — ISSAT Sousse | EEA Department | 2025**

*Réalisé par : M.A. Messelmani · T. Hemdi · H. Chaouch · A. Oussama*

</div>
