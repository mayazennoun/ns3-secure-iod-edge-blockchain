import hardhat from "hardhat";

async function main() {
  console.log("Déploiement en cours...");

  const artifact = await hardhat.artifacts.readArtifact("DroneRegistry");
  console.log("✅ Artifact lu :", artifact.contractName);
}

main().catch((error) => {
  console.error("❌ Erreur :", error);
  process.exit(1);
});