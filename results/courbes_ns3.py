import matplotlib.pyplot as plt
import numpy as np

# ========== DONNEES DES 3 SIMULATIONS ==========

drones = [5, 10, 20]

# Debit moyen (Mbps)
debit = [8.15, 6.10, 6.09]

# Latence Wi-Fi L_tx (ms)
ltx = [11.09, 28.06, 27.0]

# Latence totale L_total (ms)
ltotal = [118.09, 135.06, 134.0]

# Jitter (ms)
jitter = [1.456, 2.341, 2.33]

# Perte paquets (%)
pertes = [0.031, 0.156, 0.18]

# Labels pour les graphiques
labels = ["N=5\n(1 Edge)", "N=10\n(1 Edge)", "N=20\n(2 Edges)"]
couleurs = ["#2ecc71", "#3498db", "#9b59b6"]

# ========== COURBE 1 : Debit moyen ==========
plt.figure(figsize=(8, 5))
bars = plt.bar(labels, debit, color=couleurs, width=0.5)
plt.title("Debit moyen par drone selon le nombre de drones")
plt.xlabel("Scenario")
plt.ylabel("Debit (Mbps)")
plt.ylim(0, 10)
for bar, val in zip(bars, debit):
    plt.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 0.1,
             f"{val} Mbps", ha="center", fontweight="bold")
plt.grid(axis="y", linestyle="--", alpha=0.5)
plt.tight_layout()
plt.savefig("debit.png")
plt.show()
plt.close()

# ========== COURBE 2 : Latence totale ==========
plt.figure(figsize=(8, 5))
x = np.arange(len(labels))
width = 0.35
plt.bar(x - width/2, ltotal, width, label="L_total", color=couleurs, alpha=0.9)
plt.bar(x + width/2, ltx, width, label="L_tx (Wi-Fi)", color=["#27ae60", "#2980b9", "#8e44ad"], alpha=0.6)
plt.title("Latence totale et latence Wi-Fi selon le nombre de drones")
plt.xlabel("Scenario")
plt.ylabel("Latence (ms)")
plt.xticks(x, labels)
plt.legend()
plt.grid(axis="y", linestyle="--", alpha=0.5)
plt.tight_layout()
plt.savefig("latence_ns3.png")
plt.show()
plt.close()

# ========== COURBE 3 : Jitter ==========
plt.figure(figsize=(8, 5))
bars = plt.bar(labels, jitter, color=couleurs, width=0.5)
plt.title("Jitter moyen selon le nombre de drones")
plt.xlabel("Scenario")
plt.ylabel("Jitter (ms)")
plt.ylim(0, 3.5)
for bar, val in zip(bars, jitter):
    plt.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 0.05,
             f"{val} ms", ha="center", fontweight="bold")
plt.grid(axis="y", linestyle="--", alpha=0.5)
plt.tight_layout()
plt.savefig("jitter.png")
plt.show()
plt.close()

# ========== COURBE 4 : Perte paquets ==========
plt.figure(figsize=(8, 5))
bars = plt.bar(labels, pertes, color=couleurs, width=0.5)
plt.title("Taux de perte de paquets selon le nombre de drones")
plt.xlabel("Scenario")
plt.ylabel("Perte (%)")
plt.ylim(0, 0.5)
for bar, val in zip(bars, pertes):
    plt.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 0.005,
             f"{val}%", ha="center", fontweight="bold")
plt.grid(axis="y", linestyle="--", alpha=0.5)
plt.tight_layout()
plt.savefig("pertes.png")
plt.show()
plt.close()

# ========== COURBE 5 : Resume comparatif ==========
fig, axes = plt.subplots(2, 2, figsize=(12, 8))
fig.suptitle("Comparaison des performances IoD - NS-3 (Architecture Multi-Edge)",
             fontsize=13, fontweight="bold")

# Debit
axes[0, 0].bar(labels, debit, color=couleurs)
axes[0, 0].set_title("Debit moyen (Mbps)")
axes[0, 0].set_ylabel("Mbps")
axes[0, 0].set_ylim(0, 10)
axes[0, 0].grid(axis="y", linestyle="--", alpha=0.5)
for i, v in enumerate(debit):
    axes[0, 0].text(i, v + 0.1, f"{v}", ha="center", fontweight="bold", fontsize=9)

# Latence totale
axes[0, 1].bar(labels, ltotal, color=couleurs)
axes[0, 1].set_title("Latence totale (ms)")
axes[0, 1].set_ylabel("ms")
axes[0, 1].set_ylim(0, 160)
axes[0, 1].grid(axis="y", linestyle="--", alpha=0.5)
for i, v in enumerate(ltotal):
    axes[0, 1].text(i, v + 1, f"{v}", ha="center", fontweight="bold", fontsize=9)

# Jitter
axes[1, 0].bar(labels, jitter, color=couleurs)
axes[1, 0].set_title("Jitter moyen (ms)")
axes[1, 0].set_ylabel("ms")
axes[1, 0].set_ylim(0, 3.5)
axes[1, 0].grid(axis="y", linestyle="--", alpha=0.5)
for i, v in enumerate(jitter):
    axes[1, 0].text(i, v + 0.05, f"{v}", ha="center", fontweight="bold", fontsize=9)

# Pertes
axes[1, 1].bar(labels, pertes, color=couleurs)
axes[1, 1].set_title("Perte de paquets (%)")
axes[1, 1].set_ylabel("%")
axes[1, 1].set_ylim(0, 0.5)
axes[1, 1].grid(axis="y", linestyle="--", alpha=0.5)
for i, v in enumerate(pertes):
    axes[1, 1].text(i, v + 0.005, f"{v}%", ha="center", fontweight="bold", fontsize=9)

plt.tight_layout()
plt.savefig("resume_comparatif.png")
plt.show()
plt.close()

# ========== TABLEAU RECAPITULATIF ==========
print("\n========== TABLEAU RECAPITULATIF ==========")
print(f"{'Scenario':<20} {'Debit':>10} {'L_tx':>10} {'L_total':>10} {'Jitter':>10} {'Pertes':>10}")
print("-" * 70)
scenarios = ["N=5 (1 Edge)", "N=10 (1 Edge)", "N=20 (2 Edges)"]
for i in range(3):
    print(f"{scenarios[i]:<20} {debit[i]:>8.2f} Mbps {ltx[i]:>7.2f} ms "
          f"{ltotal[i]:>7.2f} ms {jitter[i]:>7.3f} ms {pertes[i]:>8.3f} %")

print("\nCourbes sauvegardees :")
print("- debit.png")
print("- latence_ns3.png")
print("- jitter.png")
print("- pertes.png")
print("- resume_comparatif.png")
