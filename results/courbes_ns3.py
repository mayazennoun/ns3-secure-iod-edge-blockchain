import matplotlib.pyplot as plt
import numpy as np

# ========== DONNÉES DES 3 SIMULATIONS ==========

drones = [5, 10, 20]

# Débit moyen (Mbps)
debit = [8.15, 6.10, 2.45]

# Latence Wi-Fi L_tx (ms)
ltx = [11.1, 28.0, 196.0]

# Latence totale L_total (ms)
ltotal = [118.0, 135.0, 303.0]

# Jitter (ms)
jitter = [1.45, 2.34, 5.56]

# Perte paquets (%)
pertes = [0.03, 0.16, 38.5]

# ========== COURBE 1 : Débit moyen ==========
plt.figure(figsize=(8, 5))
plt.plot(drones, debit, 'b-o', linewidth=2, markersize=8, label="Débit moyen")
plt.title("Débit moyen par drone selon le nombre de drones")
plt.xlabel("Nombre de drones")
plt.ylabel("Débit (Mbps)")
plt.xticks(drones)
plt.grid(True)
plt.legend()
plt.tight_layout()
plt.savefig("debit.png")
plt.show()
plt.close()

# ========== COURBE 2 : Latence totale ==========
plt.figure(figsize=(8, 5))
plt.plot(drones, ltotal, 'r-o', linewidth=2, markersize=8, label="L_total")
plt.plot(drones, ltx, 'g--o', linewidth=2, markersize=8, label="L_tx (Wi-Fi)")
plt.title("Latence totale et latence Wi-Fi selon le nombre de drones")
plt.xlabel("Nombre de drones")
plt.ylabel("Latence (ms)")
plt.xticks(drones)
plt.grid(True)
plt.legend()
plt.tight_layout()
plt.savefig("latence_ns3.png")
plt.show()
plt.close()

# ========== COURBE 3 : Jitter ==========
plt.figure(figsize=(8, 5))
plt.plot(drones, jitter, 'g-o', linewidth=2, markersize=8, label="Jitter moyen")
plt.title("Jitter moyen selon le nombre de drones")
plt.xlabel("Nombre de drones")
plt.ylabel("Jitter (ms)")
plt.xticks(drones)
plt.grid(True)
plt.legend()
plt.tight_layout()
plt.savefig("jitter.png")
plt.show()
plt.close()

# ========== COURBE 4 : Perte paquets ==========
plt.figure(figsize=(8, 5))
plt.plot(drones, pertes, 'm-o', linewidth=2, markersize=8, label="Perte paquets")
plt.title("Taux de perte de paquets selon le nombre de drones")
plt.xlabel("Nombre de drones")
plt.ylabel("Perte (%)")
plt.xticks(drones)
plt.grid(True)
plt.legend()
plt.tight_layout()
plt.savefig("pertes.png")
plt.show()
plt.close()

# ========== COURBE 5 : Résumé comparatif ==========
fig, axes = plt.subplots(2, 2, figsize=(12, 8))
fig.suptitle("Comparaison des performances IoD - NS-3", fontsize=14, fontweight='bold')

axes[0, 0].plot(drones, debit, 'b-o', linewidth=2, markersize=8)
axes[0, 0].set_title("Débit moyen (Mbps)")
axes[0, 0].set_xlabel("Nombre de drones")
axes[0, 0].set_ylabel("Mbps")
axes[0, 0].grid(True)
axes[0, 0].set_xticks(drones)

axes[0, 1].plot(drones, ltotal, 'r-o', linewidth=2, markersize=8)
axes[0, 1].set_title("Latence totale (ms)")
axes[0, 1].set_xlabel("Nombre de drones")
axes[0, 1].set_ylabel("ms")
axes[0, 1].grid(True)
axes[0, 1].set_xticks(drones)

axes[1, 0].plot(drones, jitter, 'g-o', linewidth=2, markersize=8)
axes[1, 0].set_title("Jitter moyen (ms)")
axes[1, 0].set_xlabel("Nombre de drones")
axes[1, 0].set_ylabel("ms")
axes[1, 0].grid(True)
axes[1, 0].set_xticks(drones)

axes[1, 1].plot(drones, pertes, 'm-o', linewidth=2, markersize=8)
axes[1, 1].set_title("Perte de paquets (%)")
axes[1, 1].set_xlabel("Nombre de drones")
axes[1, 1].set_ylabel("%")
axes[1, 1].grid(True)
axes[1, 1].set_xticks(drones)

plt.tight_layout()
plt.savefig("resume_comparatif.png")
plt.show()
plt.close()

print("Courbes sauvegardées :")
print("- debit.png")
print("- latence_ns3.png")
print("- jitter.png")
print("- pertes.png")
print("- resume_comparatif.png")
