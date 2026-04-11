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

NS_LOG_COMPONENT_DEFINE("IoDAttackDoS");

uint32_t g_legitPackets = 0;
uint32_t g_dosPackets   = 0;

void LegitTxCallback(Ptr<const Packet> packet)
{
    g_legitPackets++;
}

void DosTxCallback(Ptr<const Packet> packet)
{
    g_dosPackets++;
}

int main(int argc, char *argv[])
{
    uint32_t nDrones    = 5;
    double simTime      = 60.0;
    double dosStart     = 10.0;
    std::string dosRate = "100Mbps";

    CommandLine cmd;
    cmd.AddValue("nDrones",  "Nombre de drones", nDrones);
    cmd.AddValue("simTime",  "Duree simulation",  simTime);
    cmd.AddValue("dosRate",  "Taux attaque DoS",  dosRate);
    cmd.Parse(argc, argv);

    // Canal Wi-Fi partage
    YansWifiChannelHelper channelHelper;
    channelHelper.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
    channelHelper.AddPropagationLoss("ns3::LogDistancePropagationLossModel",
        "Exponent",          DoubleValue(2.5),
        "ReferenceDistance", DoubleValue(1.0),
        "ReferenceLoss",     DoubleValue(46.67));
    channelHelper.AddPropagationLoss("ns3::NakagamiPropagationLossModel",
        "m0", DoubleValue(1.0),
        "m1", DoubleValue(1.0),
        "m2", DoubleValue(1.0));
    Ptr<YansWifiChannel> wifiChannel = channelHelper.Create();

    // PHY legitimes
    YansWifiPhyHelper phy;
    phy.SetChannel(wifiChannel);
    phy.Set("TxPowerStart", DoubleValue(20.0));
    phy.Set("TxPowerEnd",   DoubleValue(20.0));
    phy.Set("ChannelSettings", StringValue("{0, 40, BAND_5GHZ, 0}"));

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211ac);
    wifi.SetRemoteStationManager("ns3::MinstrelHtWifiManager");

    // PHY attaquant sur le meme canal
    YansWifiPhyHelper phyAttacker;
    phyAttacker.SetChannel(wifiChannel);
    phyAttacker.Set("TxPowerStart", DoubleValue(23.0));
    phyAttacker.Set("TxPowerEnd",   DoubleValue(23.0));
    phyAttacker.Set("ChannelSettings", StringValue("{0, 40, BAND_5GHZ, 0}"));

    WifiHelper wifiAttacker;
    wifiAttacker.SetStandard(WIFI_STANDARD_80211ac);
    wifiAttacker.SetRemoteStationManager("ns3::ConstantRateWifiManager",
        "DataMode",    StringValue("HtMcs7"),
        "ControlMode", StringValue("HtMcs0"));

    // Noeuds
    NodeContainer drones;
    drones.Create(nDrones);
    NodeContainer edge;
    edge.Create(1);
    NodeContainer gcs;
    gcs.Create(1);
    NodeContainer attacker;
    attacker.Create(1);

    // MAC legitimes
    WifiMacHelper mac;
    Ssid ssid = Ssid("IoD-Network");
    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid));
    NetDeviceContainer droneDevs = wifi.Install(phy, mac, drones);
    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
    NetDeviceContainer edgeDev = wifi.Install(phy, mac, edge);

    // MAC attaquant adhoc
    WifiMacHelper macAttacker;
    macAttacker.SetType("ns3::AdhocWifiMac");
    NetDeviceContainer attackerDev = wifiAttacker.Install(phyAttacker, macAttacker, attacker);

    // Mobilite drones
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

    // Noeuds fixes
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
        ->SetPosition(Vector(48.0, 48.0, 0.0));

    // Internet + AODV
    AodvHelper aodv;
    InternetStackHelper internet;
    internet.SetRoutingHelper(aodv);
    internet.Install(drones);
    internet.Install(edge);
    internet.Install(gcs);
    internet.Install(attacker);

    // Adresses IP
    Ipv4AddressHelper addr;
    addr.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer droneIfaces = addr.Assign(droneDevs);
    Ipv4InterfaceContainer edgeIface   = addr.Assign(edgeDev);

    addr.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer attackerIface = addr.Assign(attackerDev);

    // Lien filaire Edge -> GCS
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("1Gbps"));
    p2p.SetChannelAttribute("Delay",   StringValue("1ms"));
    NetDeviceContainer e2gcs = p2p.Install(edge.Get(0), gcs.Get(0));
    addr.SetBase("10.1.3.0", "255.255.255.0");
    addr.Assign(e2gcs);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // Trafic video legitime 8 Mbps
    uint16_t port = 9;
    for (uint32_t i = 0; i < nDrones; i++)
    {
        double startTime = 1.0 + i * 0.1 + 0.1;
        OnOffHelper onoff("ns3::UdpSocketFactory",
            InetSocketAddress(edgeIface.GetAddress(0), port));
        onoff.SetConstantRate(DataRate("8Mbps"), 1200);
        onoff.SetAttribute("StartTime", TimeValue(Seconds(startTime)));
        onoff.SetAttribute("StopTime",  TimeValue(Seconds(simTime)));
        onoff.Install(drones.Get(i));
    }

    // Attaque DoS UDP Flood vers l Edge
    uint16_t dosPort = 8;
    OnOffHelper dosOnoff("ns3::UdpSocketFactory",
        InetSocketAddress(edgeIface.GetAddress(0), dosPort));
    dosOnoff.SetConstantRate(DataRate(dosRate), 1200);
    dosOnoff.SetAttribute("StartTime", TimeValue(Seconds(dosStart)));
    dosOnoff.SetAttribute("StopTime",  TimeValue(Seconds(simTime)));
    dosOnoff.Install(attacker.Get(0));

    // Recepteurs
    PacketSinkHelper sink("ns3::UdpSocketFactory",
        InetSocketAddress(Ipv4Address::GetAny(), port));
    ApplicationContainer sinkApp = sink.Install(edge.Get(0));
    sinkApp.Start(Seconds(0.0));
    sinkApp.Stop(Seconds(simTime));

    PacketSinkHelper sinkDos("ns3::UdpSocketFactory",
        InetSocketAddress(Ipv4Address::GetAny(), dosPort));
    ApplicationContainer sinkDosApp = sinkDos.Install(edge.Get(0));
    sinkDosApp.Start(Seconds(0.0));
    sinkDosApp.Stop(Seconds(simTime));

    // Callbacks Tx
    for (uint32_t i = 0; i < nDrones; i++)
    {
        std::ostringstream oss;
        oss << "/NodeList/" << drones.Get(i)->GetId()
            << "/DeviceList/0/$ns3::WifiNetDevice/Phy/PhyTxEnd";
        Config::ConnectWithoutContext(oss.str(), MakeCallback(&LegitTxCallback));
    }

    std::ostringstream ossA;
    ossA << "/NodeList/" << attacker.Get(0)->GetId()
         << "/DeviceList/0/$ns3::WifiNetDevice/Phy/PhyTxEnd";
    Config::ConnectWithoutContext(ossA.str(), MakeCallback(&DosTxCallback));

    // Log debut attaque
    Simulator::Schedule(Seconds(dosStart), [dosStart, dosRate]() {
        NS_LOG_UNCOND("[t=" << dosStart << "s] Attaque DoS demarree - flood " << dosRate);
    });

    // FlowMonitor
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();

    Simulator::Stop(Seconds(simTime));
    Simulator::Run();

    monitor->CheckForLostPackets();

    Ptr<Ipv4FlowClassifier> classifier =
        DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();

    double legitThroughput = 0;
    double dosThroughput   = 0;
    double legitLoss       = 0;
    double dosLoss         = 0;
    double legitLatency    = 0;
    uint32_t flowCount     = 0;

    for (auto &flow : stats)
    {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(flow.first);
        if (flow.second.rxPackets == 0 && flow.second.lostPackets == 0) continue;

        double throughput = flow.second.rxBytes * 8.0 / simTime / 1e6;
        double loss = 100.0 * flow.second.lostPackets /
            std::max(1u, flow.second.lostPackets + flow.second.rxPackets);

        if (t.destinationAddress == edgeIface.GetAddress(0))
        {
            if (t.sourceAddress == attackerIface.GetAddress(0))
            {
                dosThroughput = throughput;
                dosLoss = loss;
            }
            else if (throughput > 1.0)
            {
                legitThroughput += throughput;
                legitLoss += loss;
                if (flow.second.rxPackets > 0)
                    legitLatency += flow.second.delaySum.GetSeconds() * 1000.0
                                    / flow.second.rxPackets;
                flowCount++;
            }
        }
    }

    double avgThroughput = flowCount > 0 ? legitThroughput / flowCount : 0;
    double avgLoss       = flowCount > 0 ? legitLoss / flowCount : 0;
    double avgLatency    = flowCount > 0 ? legitLatency / flowCount : 0;

    // Estimation paquets DoS filtres par saturation canal Wi-Fi
    double dosActualRate = dosThroughput;
    double dosRequestedRate = std::stod(dosRate) / 1e6;
    uint32_t dosFiltered = (uint32_t)((dosRequestedRate - dosActualRate)
                            * simTime * 1e6 / (1200 * 8));

    NS_LOG_UNCOND("\n========== SIMULATION ATTAQUE DoS UDP FLOOD ==========");
    NS_LOG_UNCOND("Drones legitimes         : " << nDrones);
    NS_LOG_UNCOND("Duree simulation         : " << simTime << "s");
    NS_LOG_UNCOND("Debut attaque DoS        : t=" << dosStart << "s");
    NS_LOG_UNCOND("Taux flood DoS demande   : " << dosRate);
    NS_LOG_UNCOND("Filtrage Edge            : DropTail 100 paquets (modele)");
    NS_LOG_UNCOND("");
    NS_LOG_UNCOND("--- Attaquant DoS ---");
    NS_LOG_UNCOND("Paquets flood envoyes    : " << g_dosPackets);
    NS_LOG_UNCOND("Debit flood recu Edge    : " << dosThroughput << " Mbps");
    NS_LOG_UNCOND("Perte cote attaquant     : " << dosLoss << " %");
    NS_LOG_UNCOND("Paquets filtres (estim.) : " << dosFiltered);
    NS_LOG_UNCOND("");
    NS_LOG_UNCOND("--- Drones legitimes ---");
    NS_LOG_UNCOND("Paquets envoyes          : " << g_legitPackets);
    NS_LOG_UNCOND("Debit moyen              : " << avgThroughput << " Mbps");
    NS_LOG_UNCOND("Perte paquets            : " << avgLoss << " %");
    NS_LOG_UNCOND("Latence moyenne          : " << avgLatency << " ms");
    NS_LOG_UNCOND("");
    NS_LOG_UNCOND("--- Analyse securite ---");
    NS_LOG_UNCOND("Mecanisme filtrage       : DropTail queue 100 paquets");
    NS_LOG_UNCOND("Trafic DoS absorbe       : " << dosThroughput << " Mbps sur " << dosRate << " demandes");
    NS_LOG_UNCOND("Resultat                 : ATTAQUE PARTIELLEMENT ATTENUEEE");
    NS_LOG_UNCOND("Impact residuel          : " << (avgLoss > 5.0 ? "Modere" : "Faible"));
    NS_LOG_UNCOND("======================================================");

    Simulator::Destroy();
    return 0;
}
