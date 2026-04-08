# Configuration de simulation

Ce document decrit en detail les parametres utilises dans les trois scenarios de simulation NS-3, les choix de configuration et les justifications associees.

---

## Environnement de simulation

Les simulations ont ete realisees avec NS-3 version 3.43 sous WSL2 (Ubuntu 24.04) sur Windows 11. NS-3 a ete choisi pour sa capacite a modeliser fidelement les couches physique et MAC du standard IEEE 802.11ac, ainsi que pour la disponibilite des modules de mobilite, de routage ad hoc et de surveillance de flux (FlowMonitor).

---

## Topologie reseau

La topologie est fixe pour les trois scenarios :

```
N drones (mobilite)  -->  Wi-Fi 802.11ac  -->  Edge (fixe)  -->  filaire 1 Gbps  -->  GCS (fixe)
```

Le noeud Edge joue le role de point d'acces Wi-Fi (AP). Les drones sont des stations (STA). L'Edge est positionne au centre de la zone de mobilite aux coordonnees (50, 50, 0). La GCS est connectee a l'Edge par un lien point-a-point filaire de 1 Gbps avec 1 ms de delai.

---

## Parametres communs aux trois scenarios

| Parametre | Valeur | Justification |
|---|---|---|
| Standard sans-fil | IEEE 802.11ac, 5 GHz | Specifie dans le document de reference |
| Modele de canal | Log-distance path loss + Nakagami fading | Modelisation realiste des canaux aeriens |
| Protocole de routage | AODV | Adapte aux reseaux ad hoc mobiles |
| Mobilite UAV | Random Waypoint | Standard pour les simulations IoD |
| Vitesse UAV | 5 a 15 m/s uniforme | Document de reference |
| Pause UAV | 0 a 2 s uniforme | Document de reference |
| Zone de mobilite | 100 x 100 m | Choisie pour maintenir une couverture Wi-Fi coherente |
| Transport | UDP | Adapte au streaming video temps reel |
| Taille des paquets | 1200 octets | Document de reference |
| Duree de simulation | 300 secondes | Document de reference |
| Delai blockchain | 100 ms injecte au demarrage | Modelisation PoA (50-150 ms selon document) |
| Lien Edge-GCS | Point-to-point 1 Gbps, 1 ms | Infrastructure filaire standard |
| Exposant path loss | 2.5 | Environnement mixte interieur/exterieur |
| Perte de reference | 46.67 dB a 1 m | Calibration 5 GHz |
| Parametres Nakagami | m0=m1=m2=1.0 | Canal de Rayleigh (cas conservateur) |

---

## Parametres specifiques par scenario

### Scenario N=5 (iod-simulation-5drones.cc)

| Parametre | Valeur |
|---|---|
| Nombre de drones | 5 |
| Largeur de bande | 40 MHz |
| Puissance d'emission | 20 dBm |
| Debit video par drone | 8 Mbps |
| Adresse IP Edge (Wi-Fi) | 10.1.1.6 |
| Plage IP drones | 10.1.1.1 a 10.1.1.5 |

Ce scenario represente un deploiement leger ou le canal dispose d'une capacite suffisante pour absorber la charge totale (5 x 8 Mbps = 40 Mbps sur 40 MHz).

### Scenario N=10 (iod-simulation-10drones.cc)

| Parametre | Valeur |
|---|---|
| Nombre de drones | 10 |
| Largeur de bande | 80 MHz |
| Puissance d'emission | 23 dBm |
| Debit video par drone | 6 Mbps |
| Adresse IP Edge (Wi-Fi) | 10.1.1.11 |
| Plage IP drones | 10.1.1.1 a 10.1.1.10 |

La largeur de bande est portee a 80 MHz conformement aux valeurs du document de reference (20/40/80 MHz). La puissance est augmentee a 23 dBm pour compenser les pertes supplementaires dues au partage du canal entre davantage de noeuds.

### Scenario N=20 (iod-simulation-20drones.cc)

| Parametre | Valeur |
|---|---|
| Nombre de drones | 20 |
| Largeur de bande | 80 MHz |
| Puissance d'emission | 23 dBm |
| Debit video par drone | 6 Mbps |
| Adresse IP Edge (Wi-Fi) | 10.1.1.21 |
| Plage IP drones | 10.1.1.1 a 10.1.1.20 |

---
## Modele de trafic video

Le trafic video est modelise par des flux UDP constants (CBR) generes par le composant OnOffHelper de NS-3. Chaque drone envoie des paquets de 1200 octets a un rythme calcule pour atteindre le debit cible. Les flux demarrent avec un decalage de 100 ms entre chaque drone pour simuler le delai du handshake blockchain initial.

```
Temps de demarrage du drone i = 1.0 + i * 0.1 + 0.1 (delai blockchain)
```

---

## Collecte des metriques

Les metriques sont collectees par le module FlowMonitor de NS-3, qui surveille chaque flux IP individuellement. Seuls les flux video (debit > 1 Mbps, destination = adresse Wi-Fi de l'Edge) sont affiches dans les resultats finaux. Les flux de controle AODV sont filtres.

Pour chaque flux video, les valeurs suivantes sont enregistrees et affichees :

- Debit reel recu (Mbps) = octets recus x 8 / duree simulation / 1e6
- L_tx mesure = somme des delais / nombre de paquets recus (ms)
- L_total calcule = L_cap + L_enc + L_tx + L_bc + L_tx2 + L_dec (ms)
- Jitter moyen = somme des variations de delai / (paquets recus - 1) (ms)
- Taux de perte = paquets perdus / (paquets perdus + paquets recus) x 100 (%)

Un fichier XML de resultats bruts FlowMonitor est egalement genere en fin de simulation.
