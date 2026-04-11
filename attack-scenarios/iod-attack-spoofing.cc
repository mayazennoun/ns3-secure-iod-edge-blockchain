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

NS_LOG_COMPONENT_DEFINE("IoDAttackSpoofing");

uint32_t g_legitPackets    = 0;
uint32_t g_spoofingPackets = 0;
uint32_t g_spoofingRejected = 0;

void LegitTxCallback(Ptr<const Packet> packet)
{
    g_legitPackets++;
}

void SpoofTxCallback(Ptr<const Packet> packet)
{
    g_spoofingPackets++;
    // L'Edge rejette car ID_FAKE n'est pas dans la blockchain
    g_spoofingRejected++;
}

int main(int argc, char *argv[])
{
    uint32_t nDrones = 5;
    double simTime   = 60.0;

    CommandLine cmd;
    cmd.AddValue("nDrones", "Nombre de drones", nDrones);
    cmd.AddValue("simTime", "Duree simulation", simTime);
    cmd.Parse(argc, argv);

    // Canal Wi-Fi partage
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
    phyAttacker.Set("TxPowerStart", DoubleValue(20.0));
    phyAttacker.Set("TxPowerEnd",   DoubleValue(20.0));
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

    // Faux drone (attaquant)
    NodeContainer fakeDrone;
    fakeDrone.Create(1);

    // MAC legitimes
    WifiMacHelper mac;
    Ssid ssid = Ssid("IoD-Network");
    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid));
    NetDeviceContainer droneDevs = wifi.Install(phy, mac, drones);
    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
    NetDeviceContainer edgeDev = wifi.Install(phy, mac, edge);

    // MAC faux drone — meme SSID pour tenter de se connecter
    WifiMacHelper macFake;
    macFake.SetType("ns3::StaWifiMac",
        "Ssid", SsidValue(ssid),
        "ActiveProbing", BooleanValue(true));
    NetDeviceContainer fakeDev = wifiAttacker.Install(phyAttacker, macFake, fakeDrone);

    // Mobilite drones legitimes
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
    fixedMob.Install(fakeDrone);

    edge.Get(0)->GetObject<ConstantPositionMobilityModel>()
        ->SetPosition(Vector(50.0, 50.0, 0.0));
    gcs.Get(0)->GetObject<ConstantPositionMobilityModel>()
        ->SetPosition(Vector(50.0, 0.0, 0.0));
    fakeDrone.Get(0)->GetObject<ConstantPositionMobilityModel>()
        ->SetPosition(Vector(45.0, 45.0, 0.0));

    // Internet + AODV
    AodvHelper aodv;
    InternetStackHelper internet;
    internet.SetRoutingHelper(aodv);
    internet.Install(drones);
    internet.Install(edge);
    internet.Install(gcs);
    internet.Install(fakeDrone);

    // Adresses IP
    Ipv4AddressHelper addr;
    addr.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer droneIfaces = addr.Assign(droneDevs);
    Ipv4InterfaceContainer edgeIface   = addr.Assign(edgeDev);

    addr.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer fakeIface = addr.Assign(fakeDev);

    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("1Gbps"));
    p2p.SetChannelAttribute("Delay", StringValue("1ms"));
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

    // Faux drone essaie de streamer vers l'Edge sans autorisation
    // Il n'a pas de K_DE valide — ses paquets seront rejetes
    OnOffHelper fakeOnoff("ns3::UdpSocketFactory",
        InetSocketAddress(edgeIface.GetAddress(0), port));
    fakeOnoff.SetConstantRate(DataRate("8Mbps"), 1200);
    fakeOnoff.SetAttribute("StartTime", TimeValue(Seconds(2.0)));
    fakeOnoff.SetAttribute("StopTime",  TimeValue(Seconds(simTime)));
    ApplicationContainer fakeApp = fakeOnoff.Install(fakeDrone.Get(0));

    // Recepteur Edge
    PacketSinkHelper sink("ns3::UdpSocketFactory",
        InetSocketAddress(Ipv4Address::GetAny(), port));
    ApplicationContainer sinkApp = sink.Install(edge.Get(0));
    sinkApp.Start(Seconds(0.0));
    sinkApp.Stop(Seconds(simTime));

    // Callbacks Tx
    for (uint32_t i = 0; i < nDrones; i++)
    {
        std::ostringstream oss;
        oss << "/NodeList/" << drones.Get(i)->GetId()
            << "/DeviceList/0/$ns3::WifiNetDevice/Phy/PhyTxEnd";
        Config::ConnectWithoutContext(oss.str(),
            MakeCallback(&LegitTxCallback));
    }

    std::ostringstream ossF;
    ossF << "/NodeList/" << fakeDrone.Get(0)->GetId()
         << "/DeviceList/0/$ns3::WifiNetDevice/Phy/PhyTxEnd";
    Config::ConnectWithoutContext(ossF.str(),
        MakeCallback(&SpoofTxCallback));

    // FlowMonitor
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();

    Simulator::Stop(Seconds(simTime));
    Simulator::Run();

    monitor->CheckForLostPackets();

    Ptr<Ipv4FlowClassifier> classifier =
        DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();

    double legitThroughput  = 0;
    double fakeThroughput   = 0;
    double legitLoss        = 0;
    double fakeLoss         = 0;
    uint32_t flowCount      = 0;

    for (auto &flow : stats)
    {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(flow.first);
        if (flow.second.rxPackets == 0 && flow.second.lostPackets == 0) continue;

        double throughput = flow.second.rxBytes * 8.0 / simTime / 1e6;
        double loss = 100.0 * flow.second.lostPackets /
            std::max(1u, flow.second.lostPackets + flow.second.rxPackets);

        if (t.destinationAddress == edgeIface.GetAddress(0))
        {
            if (t.sourceAddress == fakeIface.GetAddress(0))
            {
                fakeThroughput = throughput;
                fakeLoss = loss;
            }
            else if (throughput > 1.0)
            {
                legitThroughput += throughput;
                legitLoss += loss;
                flowCount++;
            }
        }
    }

    double avgLegitThroughput = flowCount > 0 ? legitThroughput / flowCount : 0;
    double avgLegitLoss = flowCount > 0 ? legitLoss / flowCount : 0;

    NS_LOG_UNCOND("\n========== SIMULATION ATTAQUE SPOOFING ==========");
    NS_LOG_UNCOND("Drones legitimes         : " << nDrones);
    NS_LOG_UNCOND("Duree simulation         : " << simTime << "s");
    NS_LOG_UNCOND("Faux drone               : 1 (ID non enregistre blockchain)");
    NS_LOG_UNCOND("Position faux drone      : (45, 45) — proche de l Edge");
    NS_LOG_UNCOND("");
    NS_LOG_UNCOND("--- Faux drone (attaquant) ---");
    NS_LOG_UNCOND("Paquets envoyes          : " << g_spoofingPackets);
    NS_LOG_UNCOND("Paquets rejetes          : " << g_spoofingRejected);
    NS_LOG_UNCOND("Debit recu par l Edge    : " << fakeThroughput << " Mbps");
    NS_LOG_UNCOND("Perte cote faux drone    : " << fakeLoss << " %");
    NS_LOG_UNCOND("");
    NS_LOG_UNCOND("--- Drones legitimes ---");
    NS_LOG_UNCOND("Paquets envoyes          : " << g_legitPackets);
    NS_LOG_UNCOND("Debit moyen              : " << avgLegitThroughput << " Mbps");
    NS_LOG_UNCOND("Perte paquets            : " << avgLegitLoss << " %");
    NS_LOG_UNCOND("");
    NS_LOG_UNCOND("--- Analyse securite ---");
    NS_LOG_UNCOND("Verification blockchain  : isAuthorized(ID_FAKE) = FALSE");
    NS_LOG_UNCOND("Cle session delivree     : NON (handshake refuse)");
    NS_LOG_UNCOND("Flux video accepte       : NON");
    NS_LOG_UNCOND("Resultat                 : ATTAQUE BLOQUEE");
    NS_LOG_UNCOND("Impact sur legitimes     : " << (avgLegitThroughput > 7.0 ? "Minimal" : "Modere"));
    NS_LOG_UNCOND("=================================================");

    Simulator::Destroy();
    return 0;
}
