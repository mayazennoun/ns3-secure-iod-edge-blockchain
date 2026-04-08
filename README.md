# Blockchain-Enhanced Secure Communication Framework for Real-Time Internet of Drones (IoD) Video Transmission

**Projet de Fin d'Etudes (PFE)**  
Universite Badji Mokhtar Annaba — Departement Informatique  
Annee universitaire 2025/2026

---

## Presentation

Ce projet propose et evalue un cadre de securite pour la transmission video en temps reel dans les reseaux Internet of Drones (IoD). Le constat de depart est simple : les drones sont de plus en plus utilises dans des applications critiques comme surveillance, gestion de catastrophes, inspection d'infrastructures, et leurs flux video circulent sur des canaux sans fil ouverts avec peu ou pas de protection. Ce travail repond a ce manque.

Le cadre propose repose sur trois composants qui jouent chacun un role distinct. Un reseau blockchain prive gere les identites et le controle d'acces, garantissant que seuls les drones enregistres peuvent etablir une session. Un chiffrement leger du flux video par ChaCha20-Poly1305 protege les donnees sans introduire une surcharge computationnelle prohibitive sur des dispositifs aux ressources limitees. Un noeud de calcul en bordure de reseau (Edge) s'intercale entre les drones et la station de controle au sol, decharge la logique d'authentification et joue le role de relais, ce qui maintient la latence a un niveau acceptable.

Le systeme a ete evalue par simulation reseau sous NS-3 v3.43 selon trois scenarios de scalabilite : 5, 10 et 20 UAVs simultanees. Les resultats montrent des performances acceptables jusqu'a 10 drones et identifient une limite de saturation claire au-dela de ce seuil avec une architecture a noeud Edge unique.

---

## Architecture du systeme

L'architecture est organisee en quatre couches.

La **couche drone** est responsable de la capture video et du chiffrement. Chaque UAV genere un flux UDP/RTP a 6-8 Mbps et applique le chiffrement ChaCha20-Poly1305 avec une cle de session obtenue lors du handshake d'authentification.

La **couche Edge computing** heberge la logique d'authentification et de gestion des sessions. Quand un drone demande l'acces, le noeud Edge interroge le smart contract blockchain pour verifier l'identite enregistree du drone, genere une cle de session fraiche liee a un nonce et une date d'expiration, et relaie le flux video chiffre vers la station au sol. L'Edge gere egalement le renouvellement de la cle de session toutes les 60 secondes.

La **couche reseau blockchain** est une chaine privee en Proof-of-Authority avec 4 a 10 validateurs. Elle stocke les identites et cles publiques des drones, traite les decisions d'autorisation via un smart contract, et maintient un journal d'audit immuable de tous les evenements de session. Les donnees video ne passent jamais par la blockchain — seuls les evenements du chemin de controle sont enregistres sur la chaine.

La **station de controle au sol (GCS)** recoit le flux video chiffre, le dechiffre avec la cle de session courante et l'affiche en temps reel.

---

## Protocole d'authentification

L'etablissement de session suit un handshake en trois messages base sur des signatures asymetriques et un echange de nonces, analyse formellement par BAN Logic.

```
M1:  Drone  ->  Edge  :  { ID_D, N_D } signe avec la cle privee du drone
M2:  Edge   ->  Drone :  { ID_E, N_E, N_D, K_DE, token } signe avec la cle privee de l'Edge
M3:  Drone  ->  Edge  :  { N_E, Ack } signe avec la cle privee du drone
```

Dans M1, le drone presente son identite et un nonce frais. L'Edge verifie l'identite dans le registre blockchain, puis repond dans M2 avec son propre nonce, le nonce du drone repris pour preuve de fraicheur, une cle de session fraichement generee, et un token signe liant les parametres de session. Le drone confirme la reception de N_E dans M3, fournissant une preuve de vivacite a l'Edge. L'analyse BAN Logic confirme que les objectifs d'authentification mutuelle et de fraicheur de cle sont atteints sous les hypotheses standard.

---

## Configuration de simulation

Toutes les simulations ont ete conduites sous NS-3 v3.43 tournant sous WSL2 sur Windows 11.

| Parametre | Valeur |
|---|---|
| Simulateur | NS-3 v3.43 |
| Standard sans-fil | IEEE 802.11ac, 5 GHz |
| Modele de canal | Log-distance path loss + Nakagami fading |
| Largeur de bande | 40 MHz (N=5), 80 MHz (N=10, N=20) |
| Puissance d'emission | 20 dBm (N=5), 23 dBm (N=10, N=20) |
| Protocole de routage | AODV |
| Mobilite UAV | Random Waypoint, vitesse 5-15 m/s, pause 0-2 s |
| Zone de mobilite | 100 x 100 m, Edge fixe au centre (50, 50) |
| Debit video par UAV | 8 Mbps (N=5), 6 Mbps (N=10, N=20) |
| Transport | UDP |
| Taille des paquets | 1200 octets |
| Duree de simulation | 300 secondes |
| Delai blockchain | 100 ms injecte au demarrage de session (PoA modelise) |
| Metriques | FlowMonitor : debit, latence, jitter, perte de paquets |

---

## Resultats

### Tableau recapitulatif

| Scenario | Debit moyen | L_tx moyen | L_total moyen | Jitter moyen | Perte moyenne |
|---|---|---|---|---|---|
| N = 5  | 8.148 Mbps | 11.09 ms | 118.09 ms | 1.456 ms | 0.031 % |
| N = 10 | 6.098 Mbps | 28.06 ms | 135.06 ms | 2.341 ms | 0.156 % |
| N = 20 | 1.452 Mbps | 262.07 ms | 369.07 ms | 4.574 ms | 76.47 % |

La latence bout-en-bout L_total est calculee comme suit :

```
L_total = L_cap + L_enc + L_tx + L_bc + L_tx2 + L_dec
```

ou L_cap = 5 ms (capture), L_enc = 0.5 ms (chiffrement), L_tx est mesure par FlowMonitor, L_bc = 100 ms (handshake blockchain, chemin de controle uniquement), L_tx2 = 1 ms (lien filaire Edge-GCS), et L_dec = 0.5 ms (dechiffrement).

### Interpretation

Le scenario a 5 drones delivre des performances quasi optimales, avec un debit atteignant 8.15 Mbps par UAV et une perte de paquets inferieure a 0.04 %. Cela confirme que la surcharge de securite introduite par le handshake blockchain et le chiffrement ChaCha20 est negligeable sur le chemin video.

Le scenario a 10 drones montre une degradation moderee mais acceptable. Le debit chute a environ 6.1 Mbps en raison du partage du canal, la latence augmente legerement a 135 ms, et la perte de paquets reste bien en dessous de 0.2 %. Le systeme reste viable pour du streaming en temps reel dans ces conditions.

Le scenario a 20 drones (LOADING)

---

## Structure du depot

```
ns3-iod-simulation/
│
├── README.md
├── LICENSE
├── .gitignore
│
├── simulations/
│   ├── iod-simulation-5drones.cc      # N=5  | 40 MHz | 8 Mbps
│   ├── iod-simulation-10drones.cc     # N=10 | 80 MHz | 6 Mbps
│   ├── iod-simulation-20drones.cc     # N=20 | 80 MHz | 6 Mbps
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
└── 
```

---

## Comment lancer les simulations

Ces instructions supposent que NS-3 v3.43 est deja installe. Les simulations ont ete developpees et testees sous WSL2 (Ubuntu 24.04). Des instructions detaillees sont egalement disponibles dans `simulations/README.md`.

**Etape 1 — Copier le fichier de simulation dans le dossier scratch**

```bash
cp simulations/iod-simulation-5drones.cc ~/ns-allinone-3.43/ns-3.43/scratch/iod-simulation.cc
```

**Etape 2 — Compiler**

```bash
cd ~/ns-allinone-3.43/ns-3.43
./ns3 build scratch/iod-simulation
```

**Etape 3 — Lancer**

```bash
./ns3 run scratch/iod-simulation
```

Repeter les etapes 1 a 3 pour les scenarios a 10 et 20 drones en copiant les fichiers correspondants.

**Etape 4 — Generer les courbes comparatives**

```bash
cd results/
python courbes_ns3.py
```

Necessite Python 3 avec matplotlib et pandas :

```bash
pip install matplotlib pandas
```

---

## Dependances

**Simulation NS-3**

- NS-3 v3.43
- Modules utilises : wifi, mobility, internet, applications, flow-monitor, point-to-point, aodv

**Visualisation Python**

- Python 3.x
- matplotlib
- pandas

---

## Reference

> M. Nafa, M. Zennoun, "Blockchain-Enhanced Secure Communication Framework for Real-Time Internet of Drones (IoD) Video Transmission", Laboratoire LRS, Universite Badji Mokhtar Annaba, 2025.

---

## Licence

Ce projet est publie sous la licence MIT. Voir le fichier LICENSE pour les details.
