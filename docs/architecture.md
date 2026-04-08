# Architecture du système

Ce document décrit en détail l'architecture du cadre de sécurité proposé pour la transmission video en temps reel dans les reseaux Internet of Drones (IoD).

---

## Vue d'ensemble

Le système est organisé en quatre couches hiérarchiques qui interagissent selon un flux précis. Les données video ne transitent jamais par la blockchain — cette separation est fondamentale pour maintenir une latence acceptable tout en garantissant la securite.

```
[ Drone ]  -->  Wi-Fi 802.11ac  -->  [ Edge ]  -->  filaire 1 Gbps  -->  [ GCS ]
                                        |
                                        | HTTP/WebSocket
                                        v
                                  [ Blockchain PoA ]
```

---

## Couche 1 — Drone (UAV)

Le drone est le point de depart du flux. Il assure deux fonctions principales.

**Capture et encodage video**
Le drone capture un flux video en continu via une camera embarquee. Le flux est encode au format RTP et transmis en UDP a un debit de 6 a 8 Mbps, ce qui correspond a une resolution 720p a 30 images par seconde.

**Chiffrement du flux**
Avant toute transmission, chaque trame video est chiffree avec l'algorithme ChaCha20-Poly1305 (AEAD). La cle de chiffrement utilisee est la cle de session K_DE obtenue lors du handshake d'authentification avec le noeud Edge. Le nonce change a chaque trame pour garantir l'unicite. Les donnees associees AD_i incluent l'identifiant du drone, l'identifiant de l'Edge, le token de session et un horodatage.

Le drone ne communique jamais directement avec la blockchain. Il ne connait que l'adresse du noeud Edge.

---

## Couche 2 — Edge Computing (noeud MEC)

Le noeud Edge est le composant central du système. Il joue trois roles simultanes.

**Authentification et controle d'acces**
Lorsqu'un drone demande l'acces, l'Edge interroge le smart contract deploye sur la blockchain pour verifier que l'identifiant du drone est bien enregistre et autorise. Cette verification prend entre 50 et 150 ms selon la charge du reseau blockchain (modelise a 100 ms dans les simulations).

**Gestion des cles de session**
Apres verification, l'Edge genere une cle de session fraiche K_DE de 256 bits, liee a un nonce et une date d'expiration. Cette cle est transmise au drone via le message M2 du handshake et partagee avec la GCS. La cle est renouvelee toutes les 60 secondes pour limiter la fenetre d'exposition en cas de compromission.

**Relais et filtrage**
L'Edge recoit les trames video chiffrees et les relaie vers la GCS sans les dechiffrer. Il surveille egalement le trafic entrant pour filtrer les attaques par inondation UDP (DoS).

---

## Couche 3 — Reseau Blockchain (PoA)

La blockchain est un reseau Ethereum prive en mode Proof-of-Authority avec 4 a 10 noeuds validateurs. Elle assure trois fonctions exclusivement liees au chemin de controle.

**Registre d'identites**
Avant tout vol, l'administrateur enregistre chaque drone dans le smart contract DroneRegistry en associant son identifiant a sa cle publique. Cet enregistrement est permanent et infalsifiable.

**Decisions d'autorisation**
Lors de chaque demande d'acces, l'Edge appelle la fonction isAuthorized du smart contract qui retourne vrai ou faux en fonction du registre. La decision est enregistree dans le journal.

**Journal d'audit immuable**
Chaque evenement de session (connexion, renouvellement de cle, deconnexion) est enregistre sous forme de transaction sur la chaine. Ce journal ne peut pas etre modifie apres enregistrement, ce qui garantit la tracabilite complete des acces.

---

## Couche 4 — Station de controle au sol (GCS)

La GCS est le destinataire final du flux video. Elle recoit les trames chiffrees relayees par l'Edge, les dechiffre avec la cle de session K_DE courante, et affiche le flux en temps reel a l'operateur. Elle recoit egalement les notifications de renouvellement de cle envoyees par l'Edge toutes les 60 secondes.

---

## Protocole d'authentification

Le handshake entre le drone et l'Edge suit trois messages formellement analyses par BAN Logic.

```
M1:  Drone  ->  Edge  :  { ID_D, N_D } signe cle privee drone
M2:  Edge   ->  Drone :  { ID_E, N_E, N_D, K_DE, token } signe cle privee Edge
M3:  Drone  ->  Edge  :  { N_E, Ack } signe cle privee drone
```

M1 presente l'identite du drone et un nonce frais. L'Edge verifie l'identite sur la blockchain puis repond dans M2 avec son propre nonce, le nonce du drone pour preuve de fraicheur, la cle de session, et un token signe liant les parametres de session. Le drone renvoie N_E dans M3 pour prouver qu'il a bien recu et dechiffre M2.

Le token inclus dans M2 est un conteneur signe contenant (ID_D, ID_E, nonce, expiry). Il permet au drone de se presenter a la GCS sans necessiter un nouvel appel a la blockchain.

---

## Separation chemin de controle / chemin de donnees

Cette separation est le choix architectural le plus important du système.

Le chemin de controle (authentification, autorisation, renouvellement de cle) passe par la blockchain et supporte une latence de 50 a 150 ms car ces evenements sont rares (une fois au demarrage puis toutes les 60 secondes).

Le chemin de donnees (flux video) bypasse completement la blockchain et transite directement Drone -> Edge -> GCS sous protection ChaCha20. Chaque paquet video n'est pas soumis a une verification blockchain ce qui maintient la latence du flux video en dessous de 20 ms sur le lien sans fil.

---

## Modele de latence

La latence bout-en-bout d'une trame video est modelisee comme suit :

```
L_total = L_cap + L_enc + L_tx + L_bc + L_tx2 + L_dec
```

| Composante | Description | Valeur |
|---|---|---|
| L_cap | Capture et encodage camera | 5 ms |
| L_enc | Chiffrement ChaCha20 | 0.5 ms |
| L_tx | Transmission Wi-Fi drone -> Edge | mesure par FlowMonitor |
| L_bc | Verification blockchain (handshake uniquement) | 50-150 ms |
| L_tx2 | Transmission filaire Edge -> GCS | 1 ms |
| L_dec | Dechiffrement GCS | 0.5 ms |

L_bc n'impacte que le handshake initial et les renouvellements de cle, pas chaque paquet video. C'est pourquoi la latence moyenne du flux video reste acceptable malgre l'integration blockchain.
