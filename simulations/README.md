# Simulations NS-3 Instructions de compilation et d'execution

Ce dossier contient les trois scripts de simulation NS-3 correspondant aux trois scenarios evalues dans le projet. Chaque fichier est independant et peut etre compile et lance separement.

---

## Prerequis

- NS-3 v3.43 installe sur la machine
- WSL2 (Ubuntu 24.04) recommande pour les utilisateurs Windows
- Les modules NS-3 suivants doivent etre disponibles : wifi, mobility, internet, applications, flow-monitor, point-to-point, aodv

Pour verifier que NS-3 est bien installe :

```bash
cd ~/ns-allinone-3.43/ns-3.43
./ns3 --version
```

---

## Description des fichiers

| Fichier | Scenario | Largeur de bande | Debit par drone |
|---|---|---|---|
| `iod-simulation-5drones.cc` | N = 5 UAVs | 40 MHz | 8 Mbps |
| `iod-simulation-10drones.cc` | N = 10 UAVs | 80 MHz | 6 Mbps |
| `iod-simulation-20drones.cc` | N = 20 UAVs | 80 MHz | 6 Mbps |

Tous les scripts partagent les memes parametres de base :

- Standard sans-fil : IEEE 802.11ac, 5 GHz
- Modele de canal : Log-distance path loss + Nakagami fading
- Protocole de routage : AODV
- Mobilite : Random Waypoint, vitesse 5-15 m/s, pause 0-2 s
- Zone de mobilite : 100 x 100 m, Edge fixe au centre
- Transport : UDP, paquets de 1200 octets
- Duree : 300 secondes
- Delai blockchain modelise : 100 ms au demarrage de chaque session

---

## Compilation et execution

### Scenario 5 drones

```bash
cp iod-simulation-5drones.cc ~/ns-allinone-3.43/ns-3.43/scratch/iod-simulation.cc
cd ~/ns-allinone-3.43/ns-3.43
./ns3 build scratch/iod-simulation
./ns3 run scratch/iod-simulation
```

### Scenario 10 drones

```bash
cp iod-simulation-10drones.cc ~/ns-allinone-3.43/ns-3.43/scratch/iod-sim-10.cc
cd ~/ns-allinone-3.43/ns-3.43
./ns3 build scratch/iod-sim-10
./ns3 run scratch/iod-sim-10
```

### Scenario 20 drones

```bash
cp iod-simulation-20drones.cc ~/ns-allinone-3.43/ns-3.43/scratch/iod-sim-20.cc
cd ~/ns-allinone-3.43/ns-3.43
./ns3 build scratch/iod-sim-20
./ns3 run scratch/iod-sim-20
```

---

## Metriques collectees

Chaque simulation affiche pour chaque flux video (drone vers Edge) :

- **Debit** en Mbps
- **L_tx** : latence de transmission Wi-Fi mesuree par FlowMonitor (ms)
- **L_total** : latence bout-en-bout calculee selon le modele du papier (ms)
- **Jitter** moyen (ms)
- **Taux de perte de paquets** (%)
- **Delai blockchain** modelise (ms)

La formule de latence totale utilisee est :

```
L_total = L_cap + L_enc + L_tx + L_bc + L_tx2 + L_dec
```

avec L_cap = 5 ms, L_enc = 0.5 ms, L_bc = 100 ms, L_tx2 = 1 ms, L_dec = 0.5 ms.

Un fichier XML de resultats bruts FlowMonitor est egalement genere dans le repertoire courant a la fin de chaque simulation :

- `iod-results.xml` pour le scenario 5 drones
- `iod-results-10.xml` pour le scenario 10 drones
- `iod-results-20.xml` pour le scenario 20 drones

---

## Generer les courbes a partir des resultats

Une fois les trois simulations lancees et les resultats notes, se placer dans le dossier `results/` et executer :

```bash
python courbes_ns3.py
```

Les courbes comparatives sont sauvegardees dans `results/figures/`.

---
