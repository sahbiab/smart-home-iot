# ⚡ Alimentation Externe pour Servo Moteur avec ESP32

## 🎯 Pourquoi une Alimentation Externe ?

Les servo moteurs (surtout MG996R) consomment beaucoup de courant :
- **SG90** : ~500mA en charge
- **MG996R** : 1A - 2A en charge

❌ **L'ESP32 ne peut pas fournir assez de courant !**
✅ **Solution : Alimentation externe 5V**

---

## 🔌 Schéma de Branchement Complet

### Configuration avec Alimentation Externe 5V

```
┌─────────────────────────────────────────────────────────┐
│                                                         │
│  Alimentation Externe 5V (1-2A)                        │
│  (Adaptateur secteur ou batterie)                      │
│                                                         │
│      (+5V)         (GND)                               │
│        │             │                                  │
└────────┼─────────────┼──────────────────────────────────┘
         │             │
         │             │
         ├─────────────┼──────────────┐
         │             │              │
         │             │              │
    ┌────▼────┐   ┌────▼────┐   ┌────▼────────┐
    │   VCC   │   │   GND   │   │    GND      │
    │ Servo   │   │ Servo   │   │   ESP32     │
    │ Motor   │   │ Motor   │   │             │
    └─────────┘   └─────────┘   └─────────────┘
                                      │
                          Signal ─────┤ GPIO18
                                      │
```

### ⚠️ TRÈS IMPORTANT : GND Commun !
**Vous DEVEZ connecter tous les GND ensemble :**
- GND Alimentation externe
- GND Servo
- GND ESP32

---

## 📋 Connexions Détaillées

### Servo Moteur (SG90 ou MG996R)

| Câble Servo | Couleur | Connecter à |
|-------------|---------|-------------|
| **Signal** | 🟠 Orange/Jaune | **GPIO18** (ESP32) |
| **VCC (+)** | 🔴 Rouge | **+5V** (Alimentation externe) |
| **GND (-)** | 🟤 Marron/Noir | **GND Commun** (Alimentation + ESP32) |

---

## 🛠️ Options d'Alimentation Externe

### Option 1 : Adaptateur Secteur 5V ⭐ (Recommandé)
```
Adaptateur 5V 2A
    │
    ├──→ Servo VCC (Rouge)
    │
    └──→ GND → Connecter avec ESP32 GND
```

**Caractéristiques requises :**
- Tension : **5V** (ou 4.8V - 6V)
- Courant : **2A minimum** (pour MG996R)
- Connecteur : Jack DC 5.5mm ou fils dénudés

### Option 2 : Batterie 4.8V - 6V
```
Batterie 4x AA (6V)
ou
Batterie LiPo 2S (7.4V) + Régulateur 5V
    │
    ├──→ Servo VCC
    │
    └──→ GND Commun
```

### Option 3 : Power Bank USB
```
Power Bank 5V
    │
    └──→ Module Step-up/down USB
            │
            ├──→ Servo VCC
            └──→ GND Commun
```

---

## 🔧 Branchement Complet Système

```
                     ╔═══════════════════╗
                     ║  Alimentation 5V  ║
                     ║     (2A min)      ║
                     ╚═══════════════════╝
                        │           │
                       (+5V)       (GND)
                        │           │
        ┌───────────────┼───────────┼──────────────┐
        │               │           │              │
        │               │           │              │
    ┌───▼──────┐   ┌────▼───┐  ┌───▼────┐    ┌────▼───────┐
    │   VCC    │   │ Signal │  │  GND   │    │    GND     │
    │  Servo   │   │  Servo │  │ Servo  │    │   ESP32    │
    └──────────┘   └────┬───┘  └────────┘    └────┬───────┘
                        │                           │
                   (Orange/Jaune)              GPIO18 ◄───┘
                        │                           │
                        └───────────────────────────┘
```

---

## 📦 Liste du Matériel Nécessaire

### Pour l'Alimentation :
- [ ] **Adaptateur 5V 2A** (avec jack DC ou fils)
- [ ] **Breadboard** (pour connexions faciles)
- [ ] **Câbles jumper** (mâle-mâle et mâle-femelle)

### Composants :
- [ ] ESP32
- [ ] Servo moteur (SG90 ou MG996R)
- [ ] MQ-2 capteur de gaz
- [ ] LED + résistance 220Ω

---

## 🔌 Branchement Étape par Étape

### Étape 1 : Connexion GND Commun
```
Alimentation GND ──┬──→ ESP32 GND
                   └──→ Servo GND (Marron/Noir)
```

### Étape 2 : Alimenter le Servo
```
Alimentation +5V ───→ Servo VCC (Rouge)
```

### Étape 3 : Signal de Contrôle
```
ESP32 GPIO18 ───→ Servo Signal (Orange/Jaune)
```

### Étape 4 : Alimenter l'ESP32
```
USB ───→ ESP32 (ou utilisez VIN depuis alimentation 5V)
```

---

## ⚡ Schéma Complet avec Tous les Composants

```
╔═══════════════════════════════════════════════════════════╗
║           SYSTÈME COMPLET SMART HOME                      ║
╚═══════════════════════════════════════════════════════════╝

Alimentation 5V (2A)              USB (ESP32)
    │      │                           │
   +5V    GND                          │
    │      │                           │
    │      │                      ┌────▼────────────┐
    │      │                      │                 │
    │      └──────────────────────┤ GND             │
    │                             │                 │
    │              ┌──────────────┤ VIN (optionnel) │
    │              │              │                 │
    │              │         ┌────┤ GPIO34  (MQ-2)  │
    │              │         │    │                 │
    │              │         │    │ GPIO18  (Servo) ├────┐
    │              │         │    │                 │    │
    │              │         │    │ GPIO23  (LED)   ├──┐ │
    │              │         │    │                 │  │ │
    │              │         │    └─────────────────┘  │ │
    │              │         │                         │ │
    │              │    ┌────▼─────┐                  │ │
    │              │    │   MQ-2   │                  │ │
    │              │    │   VCC ◄──┼─── VIN ou 5V     │ │
    │              │    │   GND ◄──┼─── GND          │ │
    │              │    │    AO ───┤                  │ │
    │              │    └──────────┘                  │ │
    │              │                                  │ │
    ├──────────────┼──────────┐                      │ │
    │              │          │                      │ │
┌───▼────┐    ┌───▼────┐  ┌──▼────┐             ┌───▼─▼──┐
│ Servo  │    │ Servo  │  │ Servo │             │  LED   │
│  VCC   │    │  GND   │  │Signal │             │ +220Ω  │
│ (Rouge)│    │(Marron)│  │(Orange│             │  Res.  │
└────────┘    └────────┘  └───────┘             └────┬───┘
                                                     │
                                                    GND
```

---

## ✅ Points de Vérification

Avant de mettre sous tension :

- [ ] **GND commun** : Alimentation + ESP32 + Servo tous connectés
- [ ] **Servo VCC** : Connecté à +5V externe (PAS l'ESP32)
- [ ] **Servo Signal** : Connecté à GPIO18
- [ ] **Pas de court-circuit** : Vérifier avec multimètre
- [ ] **Alimentation 5V** : Capable de fournir 2A minimum

---

## 🧪 Test

### Code de Test Simple :
```cpp
#include <ESP32Servo.h>

Servo servo;

void setup() {
  servo.attach(18);
  servo.write(90);  // Position milieu
}

void loop() {
  // Test balayage
  servo.write(0);
  delay(1000);
  servo.write(90);
  delay(1000);
  servo.write(180);
  delay(1000);
}
```

### Si le servo ne bouge pas :
1. ✅ Vérifier GND commun
2. ✅ Vérifier alimentation 5V est ON
3. ✅ Vérifier connexion Signal sur GPIO18
4. ✅ Tester avec un multimètre : doit lire 5V entre VCC et GND du servo

---

## 💡 Conseils Professionnels

1. **Utilisez un breadboard** pour faciliter les connexions communes
2. **Ajoutez un condensateur** (100µF - 470µF) près du servo pour stabiliser
3. **Vérifiez la polarité** avant de brancher
4. **Commencez avec SG90** (moins de courant) pour tester
5. **Utilisez des câbles courts** pour minimiser les pertes

---

## 🔴 ATTENTION - Erreurs Courantes

❌ **NE PAS :**
- Alimenter le servo depuis le pin 3.3V de l'ESP32
- Oublier de connecter les GND ensemble
- Utiliser une alimentation < 1A pour MG996R
- Inverser VCC et GND du servo

✅ **FAIRE :**
- Toujours utiliser une alimentation externe 5V 2A
- Connecter TOUS les GND ensemble
- Vérifier les connexions avec un multimètre
- Tester d'abord sans charge mécanique

---

Votre système est maintenant prêt avec une alimentation stable ! 🚀
