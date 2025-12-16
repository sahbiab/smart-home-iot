# 🔑 Guide de Configuration Firebase pour ESP32

Ce guide vous montre où trouver les valeurs de configuration Firebase nécessaires pour votre code ESP32.

---

## 1️⃣ FIREBASE_HOST (URL Firebase Realtime Database)

### Où le trouver :
1. Allez sur [Firebase Console](https://console.firebase.google.com/)
2. Sélectionnez votre projet **smart-home-iot**
3. Dans le menu de gauche, cliquez sur **"Realtime Database"** (🗄️ icône de base de données)
4. En haut de la page, vous verrez l'URL de votre base de données

### Format :
```
https://VOTRE-PROJECT-ID-default-rtdb.firebaseio.com/
```

### Ce qu'il faut copier dans le code :
```cpp
#define FIREBASE_HOST "VOTRE-PROJECT-ID-default-rtdb.firebaseio.com"
```
**⚠️ IMPORTANT : N'incluez PAS `https://` et PAS le `/` à la fin**

### Exemple :
Si votre URL est : `https://smart-home-12345-default-rtdb.firebaseio.com/`
Vous devez mettre : `smart-home-12345-default-rtdb.firebaseio.com`

---

## 2️⃣ FIREBASE_AUTH (Database Secret / Legacy Token)

### Où le trouver :

#### Option A : Via les Paramètres du Projet
1. Dans Firebase Console, cliquez sur l'⚙️ **icône engrenage** → **Project settings**
2. Allez dans l'onglet **"Service accounts"**
3. En bas, cliquez sur **"Database secrets"**
4. Vous verrez une clé secrète (une longue chaîne de caractères)
5. Cliquez sur **"Show"** puis copiez-la

#### Option B : Via Realtime Database
1. Allez dans **Realtime Database** dans le menu gauche
2. Cliquez sur l'onglet **"Rules"** (Règles)
3. En haut à droite, cliquez sur les trois points (⋮)
4. Sélectionnez **"Manage Database Secrets"**
5. Copiez la clé secrète affichée

### Format :
Une longue chaîne alphanumérique, exemple : `AbCdEfGh123456789XyZaBcDeFgHiJkLmNoPqRsTuVwXyZ`

### Ce qu'il faut copier dans le code :
```cpp
#define FIREBASE_AUTH "AbCdEfGh123456789XyZaBcDeFgHiJkLmNoPqRsTuVwXyZ"
```

### ⚠️ Note Importante :
Si vous ne trouvez pas les "Database Secrets", c'est normal pour les nouveaux projets Firebase. Vous devez créer une clé API :

1. Allez dans **Project Settings** → **Service accounts**
2. Cliquez sur **"Generate new private key"**
3. **OU** utilisez plutôt la **Web API Key** :
   - Allez dans **Project Settings** → **General**
   - Sous "Your apps", trouvez **"Web API Key"**
   - Copiez cette clé

```cpp
#define FIREBASE_AUTH "AIzaSyAaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQ"  // Web API Key
```

---

## 3️⃣ USER_ID (ID d'utilisateur Firebase Authentication)

### Où le trouver :

#### Méthode 1 : Via Firebase Console
1. Dans Firebase Console, allez dans **Authentication** (🔐 icône de cadenas)
2. Cliquez sur l'onglet **"Users"**
3. Vous verrez la liste de tous les utilisateurs enregistrés
4. Dans la colonne **"User UID"**, copiez l'ID de l'utilisateur souhaité

### Format :
Une chaîne alphanumérique longue, exemple : `AbCdEfGh1234567890XyZaBcDe`

### Ce qu'il faut copier dans le code :
```cpp
#define USER_ID "AbCdEfGh1234567890XyZaBcDe"
```

#### Méthode 2 : Via votre Application Flutter
Vous pouvez également obtenir l'User ID depuis votre application Flutter :

1. Ouvrez votre application
2. Connectez-vous avec votre compte
3. Dans le code Flutter, ajoutez temporairement :
   ```dart
   print("User ID: ${FirebaseAuth.instance.currentUser?.uid}");
   ```
4. L'ID s'affichera dans la console de débogage

#### Méthode 3 : Via le Moniteur Série (Debug)
Si vous n'êtes pas sûr de l'UID, vous pouvez temporairement le lire depuis Firebase :
1. Dans l'Arduino IDE, ouvrez le **Moniteur Série** (115200 baud)
2. Ajoutez ce code temporaire dans `setup()` :
   ```cpp
   // Lister tous les utilisateurs (pour debug uniquement)
   if (Firebase.getJSON(firebaseData, "/users")) {
     Serial.println("Users data:");
     Serial.println(firebaseData.jsonString());
   }
   ```
3. Redémarrez l'ESP32 et regardez le Moniteur Série
4. Vous verrez les UIDs disponibles

---

## 📋 Récapitulatif - Exemple Complet

Voici un exemple de configuration complète :

```cpp
// WiFi credentials
#define WIFI_SSID "MonWiFi"
#define WIFI_PASSWORD "MotDePasse123"

// Firebase credentials
#define FIREBASE_HOST "smart-home-12345-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH "AIzaSyAaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQ"

// User ID from Firebase Authentication
#define USER_ID "xYz123AbC456DeF789GhI012JkL"
```

---

## 🔒 Sécurité

**⚠️ IMPORTANT :**
- Ne partagez JAMAIS votre `FIREBASE_AUTH` publiquement
- Ne commitez PAS ce fichier .ino avec vos vrais identifiants sur GitHub
- Créez un fichier `secrets.h` séparé pour vos identifiants si vous voulez partager le code

Exemple de fichier `secrets.h` :
```cpp
#ifndef SECRETS_H
#define SECRETS_H

#define WIFI_SSID "VotreWiFi"
#define WIFI_PASSWORD "VotreMotDePasse"
#define FIREBASE_HOST "votre-projet.firebaseio.com"
#define FIREBASE_AUTH "VotreCléSecrète"
#define USER_ID "VotreUserID"

#endif
```

Puis dans votre fichier .ino principal :
```cpp
#include "secrets.h"
```

---

## 🆘 Problèmes Courants

### Problème : "Authentication failed"
- Vérifiez que `FIREBASE_AUTH` est correct
- Assurez-vous que les règles de sécurité Firebase permettent l'accès

### Problème : "Host not found"
- Vérifiez que `FIREBASE_HOST` ne contient pas `https://` ni `/`
- Vérifiez votre connexion WiFi

### Problème : "Permission denied"
- Vérifiez vos règles Firebase Realtime Database
- Assurez-vous que l'authentification est activée

### Règles Firebase Recommandées (pour développement) :
```json
{
  "rules": {
    "users": {
      "$uid": {
        ".read": "auth != null",
        ".write": "auth != null"
      }
    }
  }
}
```

---

## ✅ Vérification

Pour vérifier que tout fonctionne :
1. Téléversez le code sur votre ESP32
2. Ouvrez le Moniteur Série (115200 baud)
3. Vous devriez voir :
   ```
   ✓ WiFi Connecté!
   ✓ Firebase Connecté!
   ✓ Données Firebase initialisées
   ✓ Système prêt!
   ```

Si vous voyez ces messages, votre configuration est correcte ! 🎉
