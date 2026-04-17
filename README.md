# Framework de Communication Sécurisé par Blockchain pour la Transmission Vidéo en Temps Réel dans l'Internet des Drones (IoD)

**Projet de Fin d'Études – Département Informatique**  
*Année universitaire 2025/2026*

---

## I. Introduction

Les drones sont de plus en plus déployés dans des applications critiques : inspection d'infrastructures, gestion de catastrophes, surveillance de sites sensibles. Pourtant, dans l'Internet des Drones (IoD), les flux vidéo empruntent des canaux sans fil ouverts, souvent sans protection native. Cette vulnérabilité expose les transmissions à des attaques par écoute, usurpation d'identité ou rejeu.

Ce travail propose et évalue une solution de sécurité légère combinant **gestion d'identités par blockchain** et **chiffrement authentifié** (ChaCha20-Poly1305). L'architecture introduit un **nœud de calcul en bordure (Edge)** qui déporte la logique d'authentification hors de la station sol, réduisant ainsi la latence tout en maintenant un niveau de sécurité élevé.

Le système est évalué par simulation sur NS‑3 v3.43 selon trois scénarios :
- 5 drones avec un nœud Edge unique,
- 10 drones avec un nœud Edge unique,
- 20 drones avec deux nœuds Edge indépendants (architecture multi‑Edge).

Les résultats montrent que le cadre proposé maintient des performances compatibles avec le temps réel (latence < 140 ms, pertes < 0,2 %) dans toutes les configurations. L'architecture multi‑Edge restaure efficacement le débit par drone en environnement dense.

---

## II. Architecture du système

L'architecture s'organise en quatre couches fonctionnelles.

**Couche drone** – Chaque UAV capture la vidéo, génère un flux RTP/UDP à 6–8 Mbps et applique un chiffrement ChaCha20-Poly1305 paquet par paquet, à l'aide d'une clé de session obtenue lors de l'authentification mutuelle.

**Couche Edge computing** – Le nœud Edge héberge la logique d'authentification et de gestion des sessions. À la demande d'un drone, il interroge la blockchain via un contrat intelligent pour vérifier l'identité enregistrée, génère une clé de session fraîche, et relaie le flux chiffré vers la station sol. La clé est renouvelée toutes les 60 secondes.

**Couche blockchain** – Chaîne privée en Proof‑of‑Authority (4 à 10 validateurs). Elle stocke les identités et clés publiques des drones, exécute les décisions d'autorisation via un contrat intelligent, et tient un journal d'audit immuable. Les données vidéo ne transitent jamais par la blockchain.

**Station de contrôle au sol (GCS)** – Reçoit le flux chiffré, le déchiffre avec la clé de session courante et l'affiche en temps réel.

---

## III. Protocole d'authentification

L'établissement de session suit un handshake à trois messages, analysé formellement par la logique BAN.

| Message | Émetteur → Récepteur | Contenu |
|---------|---------------------|---------|
| M1 | Drone → Edge | `{ID_D, N_D}` signé avec clé privée du drone |
| M2 | Edge → Drone | `{ID_E, N_E, N_D, K_DE, token}` signé avec clé privée de l'Edge |
| M3 | Drone → Edge | `{N_E, Ack}` signé avec clé privée du drone |

L'analyse BAN confirme que les objectifs suivants sont atteints sous les hypothèses standard :
- Authentification mutuelle (le drone et l'Edge s'identifient réciproquement),
- Fraîcheur de la clé de session (garantie par les nonces),
- Confidentialité de la clé `K_DE`.

---

## IV. Configuration de simulation

| Paramètre | Valeur |
|-----------|--------|
| Simulateur | NS‑3 v3.43 |
| Standard Wi‑Fi | IEEE 802.11ac, 5 GHz |
| Modèle de canal | Log‑distance path loss + Nakagami fading |
| Bande passante | 40 MHz (N=5), 80 MHz (N=10, N=20) |
| Puissance d'émission | 20 dBm (N=5), 23 dBm (N=10, N=20) |
| Routage | AODV |
| Mobilité UAV | Random Waypoint (5–15 m/s, pause 0–2 s) |
| Zone de mobilité | 100 × 100 m par nœud Edge |
| Débit vidéo par UAV | 8 Mbps (N=5), 6 Mbps (N=10, N=20) |
| Transport | UDP |
| Taille de paquet | 1200 octets |
| Durée de simulation | 300 secondes |
| Délai blockchain | 100 ms (injecté au démarrage de session) |
| Architecture N=20 | Multi‑Edge : 2 nœuds Edge, canaux physiques séparés |

La latence totale est calculée comme suit :

L_total = L_cap + L_enc + L_tx + L_bc + L_tx2 + L_dec


Avec `L_cap = 5 ms` (capture), `L_enc = 0,5 ms`, `L_bc = 100 ms` (uniquement lors du handshake), `L_tx2 = 1 ms`, `L_dec = 0,5 ms`.

---

## V. Résultats

### Tableau récapitulatif

| Scénario | Débit moyen | L_tx moyen | L_total moyen | Jitter moyen | Perte moyenne |
|----------|-------------|------------|---------------|--------------|----------------|
| N=5 (1 Edge) | 8,148 Mbps | 11,09 ms | 118,09 ms | 1,456 ms | 0,031 % |
| N=10 (1 Edge) | 6,098 Mbps | 28,06 ms | 135,06 ms | 2,341 ms | 0,156 % |
| N=20 (2 Edges) | 6,095 Mbps | 27,45 ms | 134,45 ms | 2,334 ms | 0,180 % |

### Interprétation

**Scénario N=5** – Performances quasi optimales. Le canal n'est pas saturé, le débit atteint 8,15 Mbps et les pertes restent inférieures à 0,04 %. La latence totale (118 ms) est compatible avec des applications temps réel exigeantes.

**Scénario N=10** – Dégradation modérée due au partage du canal unique. Le débit descend à 6,10 Mbps, la latence augmente à 135 ms, les pertes atteignent 0,16 %. Ces valeurs restent acceptables pour de la vidéosurveillance ou de l'inspection téléopérée.

**Scénario N=20 (multi‑Edge)** – L'architecture distribuée (2 nœuds Edge, canaux physiques séparés) restaure des performances très proches du scénario N=10 : débit 6,09 Mbps, pertes 0,18 %, latence 134 ms. Ce résultat valide l'approche multi‑Edge comme solution de passage à l'échelle pour les déploiements IoD denses.

---

## VI. Structure du dépôt

ns3-secure-iod-edge-blockchain/
│
├── README.md
├── LICENSE
├── .gitignore
│
├── paper/
│ └── Blockchain-Enhanced Secure Communication Framework for Real-Time Internet of Drones (IoD) Video Transmission.pdf
│
├── simulations/
│ ├── iod-simulation-5drones.cc
│ ├── iod-simulation-10drones.cc
│ ├── iod-simulation-20drones.cc
│ └── README.md
│
├── results/
│ ├── raw/
│ │ ├── results-5drones.txt
│ │ ├── results-10drones.txt
│ │ └── results-20drones.txt
│ │
│ ├── figures/
│ │ ├── debit.png
│ │ ├── latence_ns3.png
│ │ ├── jitter.png
│ │ ├── pertes.png
│ │ └── resume_comparatif.png
│ │
│ └── courbes_ns3.py
│
├── docs/
│ ├── architecture.md
│ ├── simulation-setup.md
│ └── results-analysis.md
│
└── attack-scenarios/
├── iod-attack-dos.cc
├── iod-attack-eavesdrop.cc
├── iod-attack-replay.cc
├── iod-attack-spoofing.cc
└── README.md


---

## VII. Exécution des simulations

```bash
# Exemple pour 5 drones
cp simulations/iod-simulation-5drones.cc ~/ns-allinone-3.43/ns-3.43/scratch/iod-simulation.cc
cd ~/ns-allinone-3.43/ns-3.43
./ns3 build scratch/iod-simulation
./ns3 run scratch/iod-simulation

# Génération des courbes
cd results/
python courbes_ns3.py

Des instructions détaillées sont fournies dans simulations/README.md
---

## VIII. Dépendances

NS‑3 v3.43 (modules : wifi, mobility, internet, applications, flow-monitor, point-to-point, aodv)

Python 3.x, matplotlib, pandas
```bash

pip install matplotlib pandas

---

## IX. Référence
M. Nafa, M. Zennoun, "Blockchain-Enhanced Secure Communication Framework for Real-Time Internet of Drones (IoD) Video Transmission", Laboratoire LRS, Université Badji Mokhtar Annaba, 2026.

---

## X. Licence
Ce projet est diffusé sous licence MIT. Libre utilisation, modification et redistribution.
