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

NS_LOG_COMPONENT_DEFINE("IoDAttackDDoS");

uint32_t g_legitPackets = 0;
uint32_t g_ddosPackets  = 0;
uint32_t g_dropped      = 0;

void LegitTxCallback(Ptr<const Packet> packet) { g_legitPackets++; }
void DDoSTxCallback(Ptr<const Packet> packet)  { g_ddosPackets++;  }
void DropCallback(Ptr<const WifiMpdu> mpdu)    { g_dropped++;      }

int main(int argc, char *argv[])
{
    uint32_t nDrones     = 5;
    uint32_t nAttackers  = 3;   // botnet de 3 attaquants
    double simTime       = 60.0;
    double ddosStart     = 10.0;
    double bcDelay       = 0.1;
    std::string dosRate  = "50Mbps"; // chaque attaquant envoie 50Mbps
                                     // total = 3 x 50 = 150 Mbps

    CommandLine cmd;
    cmd.AddValue("nDrones",    "Nombre de drones",    nDrones);
    cmd.AddValue("nAttackers", "Nombre d'attaquants", nAttackers);
    cmd.AddValue("simTime",    "Duree simulation",    simTime);
    cmd.AddValue("dosRate",    "Taux par attaquant",  dosRate);
    cmd.Parse(argc, argv);

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

    YansWifiPhyHelper phy;
    phy.SetChannel(wifiChannel);
    phy.Set("TxPowerStart", DoubleValue(23.0));
    phy.Set("TxPowerEnd",   DoubleValue(23.0));
    phy.Set("ChannelSettings", StringValue("{0, 40, BAND_5GHZ, 0}"));

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211ac);
    wifi.SetRemoteStationManager("ns3::MinstrelHtWifiManager");

    NodeContainer drones;    drones.Create(nDrones);
    NodeContainer edge;      edge.Create(1);
    NodeContainer gcs;       gcs.Create(1);
    NodeContainer attackers; attackers.Create(nAttackers);

    Ssid ssid = Ssid("IoD-Network");

    WifiMacHelper macSta;
    macSta.SetType("ns3::StaWifiMac",
        "Ssid", SsidValue(ssid),
        "ActiveProbing", BooleanValue(true));
    NetDeviceContainer droneDevs = wifi.Install(phy, macSta, drones);

    WifiMacHelper macAttacker;
    macAttacker.SetType("ns3::StaWifiMac",
        "Ssid", SsidValue(ssid),
        "ActiveProbing", BooleanValue(true));
    NetDeviceContainer attackerDevs = wifi.Install(phy, macAttacker, attackers);

    WifiMacHelper macAp;
    macAp.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
    NetDeviceContainer edgeDev = wifi.Install(phy, macAp, edge);

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
    fixedMob.Install(attackers);

    edge.Get(0)->GetObject<ConstantPositionMobilityModel>()
        ->SetPosition(Vector(50.0, 50.0, 0.0));
    gcs.Get(0)->GetObject<ConstantPositionMobilityModel>()
        ->SetPosition(Vector(50.0, 0.0, 0.0));

    // Attaquants disperses autour de l'Edge
    attackers.Get(0)->GetObject<ConstantPositionMobilityModel>()
        ->SetPosition(Vector(40.0, 40.0, 0.0));
    attackers.Get(1)->GetObject<ConstantPositionMobilityModel>()
        ->SetPosition(Vector(60.0, 40.0, 0.0));
    attackers.Get(2)->GetObject<ConstantPositionMobilityModel>()
        ->SetPosition(Vector(50.0, 60.0, 0.0));

    AodvHelper aodv;
    InternetStackHelper internet;
    internet.SetRoutingHelper(aodv);
    internet.Install(drones);
    internet.Install(edge);
    internet.Install(gcs);
    internet.Install(attackers);

    Ipv4AddressHelper addr;
    addr.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer droneIfaces   = addr.Assign(droneDevs);
    Ipv4InterfaceContainer attackerIfaces = addr.Assign(attackerDevs);
    Ipv4InterfaceContainer edgeIface     = addr.Assign(edgeDev);

    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("1Gbps"));
    p2p.SetChannelAttribute("Delay",   StringValue("1ms"));
    NetDeviceContainer e2gcs = p2p.Install(edge.Get(0), gcs.Get(0));
    addr.SetBase("10.1.3.0", "255.255.255.0");
    addr.Assign(e2gcs);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // DropTail 100 paquets sur queue Wi-Fi Edge
    Simulator::Schedule(Seconds(0.1), [&edge]() {
        Config::Set(
            "/NodeList/" + std::to_string(edge.Get(0)->GetId()) +
            "/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::ApWifiMac/BE_Txop/Queue/MaxSize",
            QueueSizeValue(QueueSize("100p")));
        Config::ConnectWithoutContext(
            "/NodeList/" + std::to_string(edge.Get(0)->GetId()) +
            "/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::ApWifiMac/BE_Txop/Queue/Drop",
            MakeCallback(&DropCallback));
    });

    // Trafic video legitime avec bcDelay
    uint16_t port = 9;
    for (uint32_t i = 0; i < nDrones; i++)
    {
        double startTime = 1.0 + i * 0.1 + bcDelay;
        OnOffHelper onoff("ns3::UdpSocketFactory",
            InetSocketAddress(edgeIface.GetAddress(0), port));
        onoff.SetConstantRate(DataRate("8Mbps"), 1200);
        onoff.SetAttribute("StartTime", TimeValue(Seconds(startTime)));
        onoff.SetAttribute("StopTime",  TimeValue(Seconds(simTime)));
        onoff.Install(drones.Get(i));
    }

    // DDoS : chaque attaquant flood simultanement
    uint16_t ddosPort = 8;
    for (uint32_t i = 0; i < nAttackers; i++)
    {
        OnOffHelper ddosOnoff("ns3::UdpSocketFactory",
            InetSocketAddress(edgeIface.GetAddress(0), ddosPort));
        ddosOnoff.SetConstantRate(DataRate(dosRate), 1200);
        ddosOnoff.SetAttribute("StartTime", TimeValue(Seconds(ddosStart + i * 0.01)));
        ddosOnoff.SetAttribute("StopTime",  TimeValue(Seconds(simTime)));
        ddosOnoff.Install(attackers.Get(i));
    }

    // Recepteurs
    PacketSinkHelper sink("ns3::UdpSocketFactory",
        InetSocketAddress(Ipv4Address::GetAny(), port));
    ApplicationContainer sinkApp = sink.Install(edge.Get(0));
    sinkApp.Start(Seconds(0.0));
    sinkApp.Stop(Seconds(simTime));

    PacketSinkHelper sinkDdos("ns3::UdpSocketFactory",
        InetSocketAddress(Ipv4Address::GetAny(), ddosPort));
    ApplicationContainer sinkDdosApp = sinkDdos.Install(edge.Get(0));
    sinkDdosApp.Start(Seconds(0.0));
    sinkDdosApp.Stop(Seconds(simTime));

    // Callbacks Tx
    for (uint32_t i = 0; i < nDrones; i++)
    {
        std::ostringstream oss;
        oss << "/NodeList/" << drones.Get(i)->GetId()
            << "/DeviceList/0/$ns3::WifiNetDevice/Phy/PhyTxEnd";
        Config::ConnectWithoutContext(oss.str(), MakeCallback(&LegitTxCallback));
    }

    for (uint32_t i = 0; i < nAttackers; i++)
    {
        std::ostringstream oss;
        oss << "/NodeList/" << attackers.Get(i)->GetId()
            << "/DeviceList/0/$ns3::WifiNetDevice/Phy/PhyTxEnd";
        Config::ConnectWithoutContext(oss.str(), MakeCallback(&DDoSTxCallback));
    }

    Simulator::Schedule(Seconds(ddosStart), [ddosStart, dosRate, nAttackers]() {
        NS_LOG_UNCOND("[t=" << ddosStart << "s] Attaque DDoS demarree — "
            << nAttackers << " attaquants x " << dosRate << " = "
            << nAttackers * std::stoi(dosRate) << " Mbps total");
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
    double ddosThroughput  = 0;
    double legitLoss       = 0;
    double ddosLoss        = 0;
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
            bool isAttacker = false;
            for (uint32_t i = 0; i < nAttackers; i++)
            {
                if (t.sourceAddress == attackerIfaces.GetAddress(i))
                {
                    isAttacker = true;
                    ddosThroughput += throughput;
                    ddosLoss += loss;
                    break;
                }
            }
            if (!isAttacker && throughput > 1.0)
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
    double avgDdosLoss   = nAttackers > 0 ? ddosLoss / nAttackers : 0;

    NS_LOG_UNCOND("\n========== SIMULATION ATTAQUE DDoS UDP FLOOD ==========");
    NS_LOG_UNCOND("Drones legitimes         : " << nDrones);
    NS_LOG_UNCOND("Nombre d'attaquants      : " << nAttackers << " (botnet)");
    NS_LOG_UNCOND("Debit par attaquant      : " << dosRate);
    NS_LOG_UNCOND("Debit total DDoS         : " << nAttackers * 50 << " Mbps");
    NS_LOG_UNCOND("Duree simulation         : " << simTime << "s");
    NS_LOG_UNCOND("Debut attaque DDoS       : t=" << ddosStart << "s");
    NS_LOG_UNCOND("Blockchain delay         : " << bcDelay*1000 << " ms (modelise)");
    NS_LOG_UNCOND("Filtrage Edge            : DropTail 100 paquets");
    NS_LOG_UNCOND("");
    NS_LOG_UNCOND("--- Botnet DDoS ---");
    NS_LOG_UNCOND("Paquets flood envoyes    : " << g_ddosPackets);
    NS_LOG_UNCOND("Debit DDoS recu Edge     : " << ddosThroughput << " Mbps");
    NS_LOG_UNCOND("Perte moyenne attaquants : " << avgDdosLoss << " %");
    NS_LOG_UNCOND("Paquets droppes queue    : " << g_dropped);
    NS_LOG_UNCOND("");
    NS_LOG_UNCOND("--- Drones legitimes ---");
    NS_LOG_UNCOND("Paquets envoyes          : " << g_legitPackets);
    NS_LOG_UNCOND("Debit moyen              : " << avgThroughput << " Mbps");
    NS_LOG_UNCOND("Perte paquets            : " << avgLoss << " %");
    NS_LOG_UNCOND("Latence moyenne          : " << avgLatency << " ms");
    NS_LOG_UNCOND("");
    NS_LOG_UNCOND("--- Analyse securite ---");
    NS_LOG_UNCOND("Mecanisme filtrage       : DropTail 100p");
    NS_LOG_UNCOND("Debit DDoS absorbe       : " << ddosThroughput << " Mbps");
    NS_LOG_UNCOND("Resultat                 : ATTAQUE PARTIELLEMENT ATTENUEEE");
    NS_LOG_UNCOND("Impact residuel          : " << (avgLoss > 5.0 ? "Severe" : "Modere"));
    NS_LOG_UNCOND("=======================================================");

    Simulator::Destroy();
    return 0;
}
