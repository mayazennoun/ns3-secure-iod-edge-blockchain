# Analyse des resultats

Ce document presente une analyse detaillee des resultats obtenus lors des trois simulations NS-3 et leur interpretation dans le contexte du cadre de securite propose.

---

## Rappel des resultats bruts

Les resultats complets flow par flow sont disponibles dans `results/raw/`. Le tableau ci-dessous resume les moyennes calculees sur l'ensemble des drones pour chaque scenario.

| Scenario | Debit moyen | L_tx moyen | L_total moyen | Jitter moyen | Perte moyenne |
|---|---|---|---|---|---|
| N = 5  | 8.148 Mbps | 11.09 ms | 118.09 ms | 1.456 ms | 0.031 % |
| N = 10 | 6.098 Mbps | 28.06 ms | 135.06 ms | 2.341 ms | 0.156 % |
| N = 20 | 1.452 Mbps | 262.07 ms | 369.07 ms | 4.574 ms | 76.47 % |

---

## Analyse par scenario

### N = 5 drones

Le scenario a 5 drones represente les conditions optimales du système. Avec une demande agregee de 40 Mbps (5 x 8 Mbps) sur un canal de 40 MHz, la charge reste bien en dessous de la capacite theorique du canal 802.11ac. Les resultats confirment cette observation : le debit moyen atteint 8.148 Mbps par drone, soit une utilisation quasi complete du debit cible, et le taux de perte est inferieur a 0.04 %.

La latence Wi-Fi L_tx est de 11.09 ms en moyenne, ce qui inclut les delais de transmission physique, de propagation et de file d'attente dans les interfaces reseau. La latence totale L_total de 118 ms est dominee par le delai blockchain de 100 ms, qui ne s'applique qu'au handshake initial et aux renouvellements de cle toutes les 60 secondes. Sur le chemin video continu, la latence effective est donc de 18 ms, ce qui est largement en dessous du seuil de 150 ms generalement admis pour le streaming video en temps reel.

Le jitter de 1.456 ms indique un flux tres stable, ce qui est coherent avec la faible charge du canal.

Ce scenario valide que la surcharge de securite introduite par le système (blockchain + ChaCha20) est negligeable sur les performances du flux video.

### N = 10 drones

Le scenario a 10 drones montre une degradation moderee mais qui reste dans des limites acceptables pour une application de streaming video. La demande agregee passe a 60 Mbps (10 x 6 Mbps) sur un canal elargi a 80 MHz.

Le debit moyen chute a 6.098 Mbps, ce qui correspond au debit cible fixe pour ce scenario. La latence Wi-Fi augmente a 28.06 ms, refletant une contention plus elevee sur le canal partage entre 10 noeuds. La latence totale de 135 ms reste acceptable. Le taux de perte de 0.156 % est negligeable pour une application video.

Le jitter de 2.341 ms reste faible et n'affecterait pas la qualite du flux video percu par l'operateur.

Ce scenario confirme que l'architecture proposee avec un noeud Edge unique supporte efficacement jusqu'a 10 drones simultanees dans des conditions de mobilite realistes.

### N = 20 drones

LOADING
---

## Conclusion

LOADING
