import { buildModule } from "@nomicfoundation/hardhat-ignition/modules";

const DroneRegistryModule = buildModule("DroneRegistry", (m) => {
  const droneRegistry = m.contract("DroneRegistry");
  return { droneRegistry };
});

export default DroneRegistryModule;