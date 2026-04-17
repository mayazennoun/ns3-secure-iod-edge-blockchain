# Blockchain-Enhanced Secure Communication Framework for Real-Time Internet of Drones (IoD) Video Transmission

**Projet de Fin d'Etudes (PFE)**
Departement Informatique
Annee universitaire 2025/2026

---

## Presentation

Ce projet propose et evalue un cadre de securite pour la transmission video en temps reel dans les reseaux Internet of Drones (IoD). Les drones sont de plus en plus utilises dans des applications critiques — surveillance, gestion de catastrophes, inspection d'infrastructures — et leurs flux video circulent sur des canaux sans fil ouverts avec peu ou pas de protection. Ce travail repond a ce manque.

Le cadre propose repose sur trois composants. Un reseau blockchain prive gere les identites et le controle d'acces, garantissant que seuls les drones enregistres peuvent etablir une session. Un chiffrement leger du flux video par ChaCha20-Poly1305 protege les donnees sans introduire une surcharge computationnelle prohibitive. Un noeud de calcul en bordure de reseau (Edge) s'intercale entre les drones et la station de controle au sol, decharge la logique d'authentification et joue le role de relais.

Le systeme a ete evalue par simulation reseau sous NS-3 v3.43 selon trois scenarios : 5 drones avec un Edge unique, 10 drones avec un Edge unique, et 20 drones avec une architecture multi-Edge (2 noeuds Edge independants). Les resultats demontrent des performances acceptables et stables sur les trois scenarios.

---

## Architecture du systeme

L'architecture est organisee en quatre couches.

La **couche drone** est responsable de la capture video et du chiffrement. Chaque UAV genere un flux UDP/RTP a 6-8 Mbps et applique le chiffrement ChaCha20-Poly1305 avec une cle de session obtenue lors du handshake d'authentification.

La **couche Edge computing** heberge la logique d'authentification et de gestion des sessions. Quand un drone demande l'acces, le noeud Edge interroge le smart contract blockchain pour verifier l'identite enregistree du drone, genere une cle de session fraiche, et relaie le flux video chiffre vers la station au sol. L'Edge gere egalement le renouvellement de la cle toutes les 60 secondes.

La **couche reseau blockchain** est une chaine privee en Proof-of-Authority avec 4 a 10 validateurs. Elle stocke les identites et cles publiques des drones, traite les decisions d'autorisation via un smart contract, et maintient un journal d'audit immuable. Les donnees video ne passent jamais par la blockchain.

La **station de controle au sol (GCS)** recoit le flux video chiffre, le dechiffre avec la cle de session courante et l'affiche en temps reel.

---

## Protocole d'authentification

Session establishment follows a three-message handshake analyzed using BAN Logic.

```
M1:  Drone  ->  Edge  :  { ID_D, N_D } signe cle privee drone
M2:  Edge   ->  Drone :  { ID_E, N_E, N_D, K_DE, token } signe cle privee Edge
M3:  Drone  ->  Edge  :  { N_E, Ack } signe cle privee drone
```

L'analyse BAN Logic confirme que les objectifs d'authentification mutuelle et de fraicheur de cle sont atteints sous les hypotheses standard.

---

## Configuration de simulation

| Parametre | Valeur |
|---|---|
| Simulateur | NS-3 v3.43 |
| Standard sans-fil | IEEE 802.11ac, 5 GHz |
| Modele de canal | Log-distance path loss + Nakagami fading |
| Largeur de bande | 40 MHz (N=5), 80 MHz (N=10, N=20) |
| Puissance d'emission | 20 dBm (N=5), 23 dBm (N=10, N=20) |
| Protocole de routage | AODV |
| Mobilite UAV | Random Waypoint, vitesse 5-15 m/s, pause 0-2 s |
| Zone de mobilite | 100 x 100 m par noeud Edge |
| Debit video par UAV | 8 Mbps (N=5), 6 Mbps (N=10, N=20) |
| Transport | UDP |
| Taille des paquets | 1200 octets |
| Duree de simulation | 300 secondes |
| Delai blockchain | 100 ms injecte au demarrage de session |
| Architecture N=20 | Multi-Edge : 2 noeuds Edge independants, canaux physiques separes |

---

## Resultats

### Tableau recapitulatif

| Scenario | Debit moyen | L_tx moyen | L_total moyen | Jitter moyen | Perte moyenne |
|---|---|---|---|---|---|
| N=5 (1 Edge) | 8.148 Mbps | 11.09 ms | 118.09 ms | 1.456 ms | 0.031 % |
| N=10 (1 Edge) | 6.098 Mbps | 28.06 ms | 135.06 ms | 2.341 ms | 0.156 % |
| N=20 (2 Edges) | 6.095 Mbps | 27.45 ms | 134.45 ms | 2.334 ms | 0.180 % |

La latence bout-en-bout L_total est calculee comme suit :

```
L_total = L_cap + L_enc + L_tx + L_bc + L_tx2 + L_dec
```

avec L_cap=5ms, L_enc=0.5ms, L_bc=100ms (handshake uniquement), L_tx2=1ms, L_dec=0.5ms.

### Interpretation

Le scenario N=5 delivre des performances quasi optimales avec un debit de 8.15 Mbps et des pertes inferieures a 0.04%.

Le scenario N=10 montre une degradation moderee due au partage du canal, avec un debit de 6.10 Mbps et des pertes inferieures a 0.2%, ce qui reste acceptable pour du streaming temps reel.

Le scenario N=20 avec architecture multi-Edge (2 noeuds Edge independants couvrant chacun 10 drones sur des canaux physiques separes) restaure des performances comparables au scenario N=10, avec un debit de 6.09 Mbps et des pertes de 0.18%. Ce resultat valide l'architecture multi-Edge comme solution de scalabilite pour les deployements IoD a grande echelle.

---

## Structure du depot

```
ns3-secure-iod-edge-blockchain/
│
├── README.md
├── LICENSE
├── .gitignore
│
├── simulations/
│   ├── iod-simulation-5drones.cc      # N=5  | 1 Edge | 40 MHz | 8 Mbps
│   ├── iod-simulation-10drones.cc     # N=10 | 1 Edge | 80 MHz | 6 Mbps
│   ├── iod-simulation-20drones.cc     # N=20 | 2 Edges | 80 MHz | 6 Mbps
│   └── README.md
│
├── results/
│   ├── raw/
│   │   ├── results-5drones.txt
│   │   ├── results-10drones.txt
│   │   └── results-20drones.txt
│   ├── figures/
│   │   ├── debit.png
│   │   ├── latence_ns3.png
│   │   ├── jitter.png
│   │   ├── pertes.png
│   │   └── resume_comparatif.png
│   └── courbes_ns3.py
│
├── docs/
│   ├── architecture.md
│   ├── simulation-setup.md
│   └── results-analysis.md
│  
└── attack-scenarios/
│   ├── iod-attack-dos.cc
│   └── iod-attack-eavesdrop.cc
│   ├── iod-attack-replay.cc
│   └── iod-attack-spoofing.cc
|   └── README.md   
|
   
```

---
## Comment lancer les simulations

```bash
cp simulations/iod-simulation-5drones.cc ~/ns-allinone-3.43/ns-3.43/scratch/iod-simulation.cc
cd ~/ns-allinone-3.43/ns-3.43
./ns3 build scratch/iod-simulation
./ns3 run scratch/iod-simulation
```

Repeter pour les scenarios 10 et 20 drones. Instructions detaillees dans `simulations/README.md`.

```bash
cd results/
python courbes_ns3.py
```

---

## Dependances

- NS-3 v3.43 (modules : wifi, mobility, internet, applications, flow-monitor, point-to-point, aodv)
- Python 3.x, matplotlib, pandas

```bash
pip install matplotlib pandas
```

---

## Reference

> M. Nafa, M. Zennoun, "Blockchain-Enhanced Secure Communication Framework for Real-Time Internet of Drones (IoD) Video Transmission", Laboratoire LRS, Universite Badji Mokhtar Annaba, 2026.

---

## Licence

Ce projet est publie sous la licence MIT.
