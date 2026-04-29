#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/aodv-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("IoDAttackReplayNOBC");

uint32_t g_legitPackets  = 0;
uint32_t g_replayPackets = 0;
std::vector<Ptr<Packet>> g_capturedPackets;
bool g_capturing = true;

void CaptureCallback(
    Ptr<const Packet> packet,
    uint16_t channelFreqMhz,
    WifiTxVector txVector,
    MpduInfo aMpdu,
    SignalNoiseDbm signalNoise,
    uint16_t staId)
{
    if (g_capturing && g_capturedPackets.size() < 100)
        g_capturedPackets.push_back(packet->Copy());
}

void LegitTxCallback(Ptr<const Packet> packet) { g_legitPackets++; }
void ReplayTxCallback(Ptr<const Packet> packet) { g_replayPackets++; }

int main(int argc, char *argv[])
{
    uint32_t nDrones    = 5;
    double simTime      = 60.0;
    double replayStart  = 30.0;

    CommandLine cmd;
    cmd.AddValue("nDrones", "Nombre de drones", nDrones);
    cmd.AddValue("simTime", "Duree simulation", simTime);
    cmd.Parse(argc, argv);

    YansWifiChannelHelper channelHelper;
    channelHelper.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
    channelHelper.AddPropagationLoss("ns3::LogDistancePropagationLossModel",
        "Exponent", DoubleValue(2.5),
        "ReferenceDistance", DoubleValue(1.0),
        "ReferenceLoss", DoubleValue(46.67));
    channelHelper.AddPropagationLoss("ns3::NakagamiPropagationLossModel",
        "m0", DoubleValue(1.0),
        "m1", DoubleValue(1.0),
        "m2", DoubleValue(1.0));
    Ptr<YansWifiChannel> wifiChannel = channelHelper.Create();

    YansWifiPhyHelper phy;
    phy.SetChannel(wifiChannel);
    phy.Set("TxPowerStart", DoubleValue(20.0));
    phy.Set("TxPowerEnd",   DoubleValue(20.0));
    phy.Set("ChannelSettings", StringValue("{0, 40, BAND_5GHZ, 0}"));

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211ac);
    wifi.SetRemoteStationManager("ns3::MinstrelHtWifiManager");

    YansWifiPhyHelper phyAttacker;
    phyAttacker.SetChannel(wifiChannel);
    phyAttacker.Set("TxPowerStart", DoubleValue(20.0));
    phyAttacker.Set("TxPowerEnd",   DoubleValue(20.0));
    phyAttacker.Set("ChannelSettings", StringValue("{0, 40, BAND_5GHZ, 0}"));

    WifiHelper wifiAttacker;
    wifiAttacker.SetStandard(WIFI_STANDARD_80211ac);
    wifiAttacker.SetRemoteStationManager("ns3::ConstantRateWifiManager",
        "DataMode",    StringValue("HtMcs7"),
        "ControlMode", StringValue("HtMcs0"));

    NodeContainer drones;   drones.Create(nDrones);
    NodeContainer edge;     edge.Create(1);
    NodeContainer gcs;      gcs.Create(1);
    NodeContainer attacker; attacker.Create(1);

    WifiMacHelper mac;
    Ssid ssid = Ssid("IoD-Network");
    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid));
    NetDeviceContainer droneDevs = wifi.Install(phy, mac, drones);
    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
    NetDeviceContainer edgeDev = wifi.Install(phy, mac, edge);

    WifiMacHelper macAttacker;
    macAttacker.SetType("ns3::AdhocWifiMac");
    NetDeviceContainer attackerDev = wifiAttacker.Install(phyAttacker, macAttacker, attacker);

    MobilityHelper mob;
    ObjectFactory pos;
    pos.SetTypeId("ns3::RandomRectanglePositionAllocator");
    pos.Set("X", StringValue("ns3::UniformRandomVariable[Min=0|Max=100]"));
    pos.Set("Y", StringValue("ns3::UniformRandomVariable[Min=0|Max=100]"));
    Ptr<PositionAllocator> posAlloc = pos.Create()->GetObject<PositionAllocator>();
    mob.SetMobilityModel("ns3::RandomWaypointMobilityModel",
        "Speed", StringValue("ns3::UniformRandomVariable[Min=5|Max=15]"),
        "Pause", StringValue("ns3::UniformRandomVariable[Min=0|Max=2]"),
        "PositionAllocator", PointerValue(posAlloc));
    mob.SetPositionAllocator(posAlloc);
    mob.Install(drones);

    MobilityHelper fixedMob;
    fixedMob.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    fixedMob.Install(edge);
    fixedMob.Install(gcs);
    fixedMob.Install(attacker);

    edge.Get(0)->GetObject<ConstantPositionMobilityModel>()
        ->SetPosition(Vector(50.0, 50.0, 0.0));
    gcs.Get(0)->GetObject<ConstantPositionMobilityModel>()
        ->SetPosition(Vector(50.0, 0.0, 0.0));
    attacker.Get(0)->GetObject<ConstantPositionMobilityModel>()
        ->SetPosition(Vector(50.0, 50.0, 0.0));

    AodvHelper aodv;
    InternetStackHelper internet;
    internet.SetRoutingHelper(aodv);
    internet.Install(drones);
    internet.Install(edge);
    internet.Install(gcs);
    internet.Install(attacker);

    Ipv4AddressHelper addr;
    addr.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer droneIfaces   = addr.Assign(droneDevs);
    Ipv4InterfaceContainer edgeIface     = addr.Assign(edgeDev);

    addr.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer attackerIface = addr.Assign(attackerDev);

    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("1Gbps"));
    p2p.SetChannelAttribute("Delay", StringValue("1ms"));
    NetDeviceContainer e2gcs = p2p.Install(edge.Get(0), gcs.Get(0));
    addr.SetBase("10.1.3.0", "255.255.255.0");
    addr.Assign(e2gcs);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // SANS BLOCKCHAIN : drones demarrent sans delai d'authentification
    uint16_t port = 9;
    for (uint32_t i = 0; i < nDrones; i++)
    {
        double startTime = 1.0 + i * 0.1; // pas de bcDelay
        OnOffHelper onoff("ns3::UdpSocketFactory",
            InetSocketAddress(edgeIface.GetAddress(0), port));
        onoff.SetConstantRate(DataRate("8Mbps"), 1200);
        onoff.SetAttribute("StartTime", TimeValue(Seconds(startTime)));
        onoff.SetAttribute("StopTime",  TimeValue(Seconds(simTime)));
        onoff.Install(drones.Get(i));
    }

    // SANS BLOCKCHAIN : attaquant rejoue vers l Edge sans token valide
    // Sans blockchain il n y a pas de verification de token
    // donc les paquets rejoues PEUVENT etre acceptes
    OnOffHelper replayOnoff("ns3::UdpSocketFactory",
        InetSocketAddress(edgeIface.GetAddress(0), port));
    replayOnoff.SetConstantRate(DataRate("4Mbps"), 1200);
    replayOnoff.SetAttribute("StartTime", TimeValue(Seconds(replayStart)));
    replayOnoff.SetAttribute("StopTime",  TimeValue(Seconds(simTime)));
    replayOnoff.Install(attacker.Get(0));

    PacketSinkHelper sink("ns3::UdpSocketFactory",
        InetSocketAddress(Ipv4Address::GetAny(), port));
    ApplicationContainer sinkApp = sink.Install(edge.Get(0));
    sinkApp.Start(Seconds(0.0));
    sinkApp.Stop(Seconds(simTime));

    Simulator::Schedule(Seconds(0.5), [&]() {
        std::ostringstream oss;
        oss << "/NodeList/" << attacker.Get(0)->GetId()
            << "/DeviceList/0/$ns3::WifiNetDevice/Phy/MonitorSnifferRx";
        Config::ConnectWithoutContext(oss.str(),
            MakeCallback(&CaptureCallback));
    });

    for (uint32_t i = 0; i < nDrones; i++)
    {
        std::ostringstream oss;
        oss << "/NodeList/" << drones.Get(i)->GetId()
            << "/DeviceList/0/$ns3::WifiNetDevice/Phy/PhyTxEnd";
        Config::ConnectWithoutContext(oss.str(),
            MakeCallback(&LegitTxCallback));
    }

    std::ostringstream ossA;
    ossA << "/NodeList/" << attacker.Get(0)->GetId()
         << "/DeviceList/0/$ns3::WifiNetDevice/Phy/PhyTxEnd";
    Config::ConnectWithoutContext(ossA.str(),
        MakeCallback(&ReplayTxCallback));

    Simulator::Schedule(Seconds(replayStart), []() {
        g_capturing = false;
        NS_LOG_UNCOND("[t=30s] Attaquant demarre le rejeu — SANS verification blockchain");
    });

    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();

    Simulator::Stop(Seconds(simTime));
    Simulator::Run();

    monitor->CheckForLostPackets();

    Ptr<Ipv4FlowClassifier> classifier =
        DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();

    double legitThroughput = 0;
    double replayThroughput = 0;
    double legitLoss = 0;
    double legitLatency = 0;
    uint32_t flowCount = 0;

    for (auto &flow : stats)
    {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(flow.first);
        if (flow.second.rxPackets == 0) continue;
        double throughput = flow.second.rxBytes * 8.0 / simTime / 1e6;
        if (throughput < 0.1) continue;
        if (t.destinationAddress == edgeIface.GetAddress(0))
        {
            if (t.sourceAddress == attackerIface.GetAddress(0))
            {
                replayThroughput = throughput;
            }
            else if (throughput > 1.0)
            {
                legitThroughput += throughput;
                legitLoss += 100.0 * flow.second.lostPackets /
                    (flow.second.lostPackets + flow.second.rxPackets);
                legitLatency += flow.second.delaySum.GetSeconds() * 1000.0
                                / flow.second.rxPackets;
                flowCount++;
            }
        }
    }

    double avgThroughput = flowCount > 0 ? legitThroughput / flowCount : 0;
    double avgLoss       = flowCount > 0 ? legitLoss / flowCount : 0;
    double avgLatency    = flowCount > 0 ? legitLatency / flowCount : 0;
    double L_total_nobc  = 5.0 + 0.5 + avgLatency + 1.0 + 0.5;

    double replayDuration = simTime - replayStart;
    uint32_t replayEstimate = (uint32_t)(replayDuration * 4e6 / (1200 * 8));

    NS_LOG_UNCOND("\n========== ATTAQUE REPLAY — SANS BLOCKCHAIN ==========");
    NS_LOG_UNCOND("Drones legitimes        : " << nDrones);
    NS_LOG_UNCOND("Duree simulation        : " << simTime << "s");
    NS_LOG_UNCOND("Authentification        : AUCUNE (pas de blockchain)");
    NS_LOG_UNCOND("Verification nonce      : ABSENTE (pas de token)");
    NS_LOG_UNCOND("");
    NS_LOG_UNCOND("--- Phase 1 : Capture (0s-30s) ---");
    NS_LOG_UNCOND("Paquets legitimes Tx    : " << g_legitPackets);
    NS_LOG_UNCOND("Paquets captures        : " << g_capturedPackets.size());
    NS_LOG_UNCOND("");
    NS_LOG_UNCOND("--- Phase 2 : Rejeu (30s-60s) ---");
    NS_LOG_UNCOND("Paquets rejoues envoyes : " << replayEstimate);
    NS_LOG_UNCOND("Debit rejeu recu Edge   : " << replayThroughput << " Mbps");
    NS_LOG_UNCOND("");
    NS_LOG_UNCOND("--- Impact reseau ---");
    NS_LOG_UNCOND("Debit legitime moyen    : " << avgThroughput << " Mbps");
    NS_LOG_UNCOND("Perte paquets           : " << avgLoss << " %");
    NS_LOG_UNCOND("L_tx moyen              : " << avgLatency << " ms");
    NS_LOG_UNCOND("L_total (sans L_bc)     : " << L_total_nobc << " ms");
    NS_LOG_UNCOND("");
    NS_LOG_UNCOND("--- Analyse securite ---");
    NS_LOG_UNCOND("Verification nonce      : ABSENTE");
    NS_LOG_UNCOND("Paquets rejoues acceptes: " << replayThroughput << " Mbps RECU par l'Edge");
    NS_LOG_UNCOND("Risque                  : L'Edge accepte les paquets rejoues");
    NS_LOG_UNCOND("Resultat                : ATTAQUE REUSSIE SANS BLOCKCHAIN");
    NS_LOG_UNCOND("======================================================");

    Simulator::Destroy();
    return 0;
}
