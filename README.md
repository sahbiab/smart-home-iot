# 🏠 Smart Home IoT - Système de Maison Intelligente

## 📋 Description

Système de maison intelligente complet combinant reconnaissance faciale, streaming vidéo en temps réel et contrôle via application mobile. Le projet utilise un Raspberry Pi comme serveur embarqué et une application mobile Flutter pour l'interface utilisateur.

## ✨ Fonctionnalités

### Application Mobile (Flutter)
- 🔐 **Authentification sécurisée** avec Firebase (Google Sign-In)
- 📱 **Interface Material Design moderne** avec thème personnalisé
- 📹 **Streaming vidéo en temps réel** depuis le Raspberry Pi
- 👤 **Reconnaissance faciale** avec capture multi-angles
- ☁️ **Stockage cloud** des données biométriques (Firebase Storage)
- 👥 **Gestion des profils utilisateurs** avec Firestore

### Serveur Raspberry Pi (Python)
- 🎥 **Streaming vidéo MJPEG** en temps réel (OpenCV)
- 🤖 **API REST** pour la communication avec l'application mobile
- 💾 **Stockage local** des images de reconnaissance faciale
- 🔄 **Gestion automatique** de la caméra avec récupération d'erreurs
- 📊 **Endpoints de statut** pour monitoring

## 🛠️ Technologies Utilisées

### Frontend Mobile
- **Flutter/Dart** - Framework cross-platform
- **Firebase Auth** - Authentification utilisateurs
- **Cloud Firestore** - Base de données NoSQL
- **Firebase Storage** - Stockage cloud
- **flutter_mjpeg** - Affichage du stream vidéo
- **camera** - Accès à la caméra du téléphone
- **image_picker** - Sélection d'images

### Backend Raspberry Pi
- **Python 3** - Langage principal
- **Flask** - Framework web
- **OpenCV (cv2)** - Traitement vidéo
- **Flask-CORS** - Gestion des requêtes cross-origin

### Infrastructure
- **Raspberry Pi** - Serveur embarqué
- **Firebase** - Backend as a Service
- **REST API** - Communication client-serveur

## 📁 Structure du Projet

```
projet-arch_IOT/
├── smart_home_project/          # Application Flutter
│   ├── lib/
│   │   ├── auth/               # Pages d'authentification
│   │   ├── services/           # Services API
│   │   ├── profile/            # Page de profil
│   │   └── utils/              # Utilitaires et styles
│   ├── assets/                 # Images et ressources
│   └── pubspec.yaml            # Dépendances Flutter
│
├── camera_server_v2.py         # Serveur de streaming vidéo
├── raspberry_pi_server.py      # Serveur de reconnaissance faciale
├── requirements.txt            # Dépendances Python
└── RASPBERRY_PI_SETUP.md       # Guide d'installation Raspberry Pi
```

## 🚀 Installation et Démarrage

### Prérequis
- Flutter SDK (3.10.1+)
- Python 3.7+
- Raspberry Pi avec caméra
- Compte Firebase configuré

### Configuration Firebase
1. Créer un projet sur [Firebase Console](https://console.firebase.google.com/)
2. Activer Authentication (Google Sign-In)
3. Créer une base Firestore
4. Télécharger `google-services.json` et le placer dans `smart_home_project/android/app/`

### Application Mobile

```bash
cd smart_home_project
flutter pub get
flutter run
```

### Serveur Raspberry Pi

```bash
# Installer les dépendances
pip3 install -r requirements.txt

# Démarrer le serveur de streaming vidéo
python3 camera_server_v2.py

# Démarrer le serveur de reconnaissance faciale
python3 raspberry_pi_server.py
```

Voir [RASPBERRY_PI_SETUP.md](RASPBERRY_PI_SETUP.md) pour plus de détails.

## 📡 Architecture

```
┌─────────────────┐         ┌──────────────────┐         ┌─────────────┐
│  Application    │         │   Raspberry Pi   │         │   Firebase  │
│  Mobile Flutter │◄───────►│   Serveur Python │◄───────►│   Cloud     │
│                 │  REST   │                  │  Auth   │             │
│  - UI/UX        │  API    │  - Streaming     │  Data   │  - Auth     │
│  - Camera       │         │  - Face Recog.   │         │  - Firestore│
│  - Display      │         │  - Storage       │         │  - Storage  │
└─────────────────┘         └──────────────────┘         └─────────────┘
```

## 🔧 Configuration

### URLs du Raspberry Pi
Modifier l'adresse IP dans l'application mobile :
- Fichier : `smart_home_project/lib/services/face_recognition_api.dart`
- Remplacer `192.168.137.160` par l'IP de votre Raspberry Pi

### Ports utilisés
- **5000** : Serveur de reconnaissance faciale
- **8081** : Serveur de streaming vidéo

## 📸 Captures d'écran

*(À ajouter : captures d'écran de l'application)*

## 🎯 Fonctionnalités à venir

- [ ] Détection de mouvement avec capteurs ultrasoniques
- [ ] Notifications push en temps réel
- [ ] Historique des détections
- [ ] Support multi-caméras
- [ ] Dashboard de statistiques

## 👨‍💻 Auteur

**Sahbi Abbassi**
- Email: abbassisahbi0407@gmail.com
- GitHub: [@sahbiab](https://github.com/sahbiab)
- LinkedIn: [Abbassi Sahbi](https://www.linkedin.com/in/abbassi-sahbi-465526388/)

## 📄 Licence

Ce projet est sous licence MIT - voir le fichier LICENSE pour plus de détails.

## 🙏 Remerciements

- Flutter Team pour le framework
- Firebase pour l'infrastructure backend
- OpenCV pour le traitement vidéo
