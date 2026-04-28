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

NS_LOG_COMPONENT_DEFINE("IoDSim20_3Edges");

int main(int argc, char *argv[])
{
    uint32_t nDrones = 20;
    double simTime   = 300.0;

    CommandLine cmd;
    cmd.AddValue("nDrones", "Nombre de drones", nDrones);
    cmd.Parse(argc, argv);

    // ===== CANAL 1 =====
    YansWifiChannelHelper ch1;
    ch1.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
    ch1.AddPropagationLoss("ns3::LogDistancePropagationLossModel",
        "Exponent", DoubleValue(2.5),
        "ReferenceDistance", DoubleValue(1.0),
        "ReferenceLoss", DoubleValue(46.67));
    ch1.AddPropagationLoss("ns3::NakagamiPropagationLossModel",
        "m0", DoubleValue(1.0), "m1", DoubleValue(1.0), "m2", DoubleValue(1.0));
    Ptr<YansWifiChannel> wCh1 = ch1.Create();

    YansWifiPhyHelper phy1;
    phy1.SetChannel(wCh1);
    phy1.Set("TxPowerStart", DoubleValue(23.0));
    phy1.Set("TxPowerEnd",   DoubleValue(23.0));
    phy1.Set("ChannelSettings", StringValue("{0, 80, BAND_5GHZ, 0}"));

    WifiHelper wifi1;
    wifi1.SetStandard(WIFI_STANDARD_80211ac);
    wifi1.SetRemoteStationManager("ns3::MinstrelHtWifiManager");

    // ===== CANAL 2 =====
    YansWifiChannelHelper ch2;
    ch2.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
    ch2.AddPropagationLoss("ns3::LogDistancePropagationLossModel",
        "Exponent", DoubleValue(2.5),
        "ReferenceDistance", DoubleValue(1.0),
        "ReferenceLoss", DoubleValue(46.67));
    ch2.AddPropagationLoss("ns3::NakagamiPropagationLossModel",
        "m0", DoubleValue(1.0), "m1", DoubleValue(1.0), "m2", DoubleValue(1.0));
    Ptr<YansWifiChannel> wCh2 = ch2.Create();

    YansWifiPhyHelper phy2;
    phy2.SetChannel(wCh2);
    phy2.Set("TxPowerStart", DoubleValue(23.0));
    phy2.Set("TxPowerEnd",   DoubleValue(23.0));
    phy2.Set("ChannelSettings", StringValue("{0, 80, BAND_5GHZ, 0}"));

    WifiHelper wifi2;
    wifi2.SetStandard(WIFI_STANDARD_80211ac);
    wifi2.SetRemoteStationManager("ns3::MinstrelHtWifiManager");

    // ===== CANAL 3 =====
    YansWifiChannelHelper ch3;
    ch3.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
    ch3.AddPropagationLoss("ns3::LogDistancePropagationLossModel",
        "Exponent", DoubleValue(2.5),
        "ReferenceDistance", DoubleValue(1.0),
        "ReferenceLoss", DoubleValue(46.67));
    ch3.AddPropagationLoss("ns3::NakagamiPropagationLossModel",
        "m0", DoubleValue(1.0), "m1", DoubleValue(1.0), "m2", DoubleValue(1.0));
    Ptr<YansWifiChannel> wCh3 = ch3.Create();

    YansWifiPhyHelper phy3;
    phy3.SetChannel(wCh3);
    phy3.Set("TxPowerStart", DoubleValue(23.0));
    phy3.Set("TxPowerEnd",   DoubleValue(23.0));
    phy3.Set("ChannelSettings", StringValue("{0, 80, BAND_5GHZ, 0}"));

    WifiHelper wifi3;
    wifi3.SetStandard(WIFI_STANDARD_80211ac);
    wifi3.SetRemoteStationManager("ns3::MinstrelHtWifiManager");

    // ===== NOEUDS =====
    NodeContainer drones1; drones1.Create(7);
    NodeContainer drones2; drones2.Create(7);
    NodeContainer drones3; drones3.Create(6);
    NodeContainer edge1;   edge1.Create(1);
    NodeContainer edge2;   edge2.Create(1);
    NodeContainer edge3;   edge3.Create(1);
    NodeContainer gcs;     gcs.Create(1);

    // ===== MAC GROUPE 1 =====
    WifiMacHelper mac1;
    Ssid ssid1 = Ssid("IoD-Edge1");
    mac1.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid1));
    NetDeviceContainer droneDevs1 = wifi1.Install(phy1, mac1, drones1);
    mac1.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid1));
    NetDeviceContainer edgeDev1 = wifi1.Install(phy1, mac1, edge1);

    // ===== MAC GROUPE 2 =====
    WifiMacHelper mac2;
    Ssid ssid2 = Ssid("IoD-Edge2");
    mac2.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid2));
    NetDeviceContainer droneDevs2 = wifi2.Install(phy2, mac2, drones2);
    mac2.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid2));
    NetDeviceContainer edgeDev2 = wifi2.Install(phy2, mac2, edge2);

    // ===== MAC GROUPE 3 =====
    WifiMacHelper mac3;
    Ssid ssid3 = Ssid("IoD-Edge3");
    mac3.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid3));
    NetDeviceContainer droneDevs3 = wifi3.Install(phy3, mac3, drones3);
    mac3.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid3));
    NetDeviceContainer edgeDev3 = wifi3.Install(phy3, mac3, edge3);

    // ===== MOBILITE =====
    auto installMobility = [](NodeContainer& nodes, double xMin, double xMax,
                               double yMin, double yMax) {
        MobilityHelper mob;
        ObjectFactory pos;
        pos.SetTypeId("ns3::RandomRectanglePositionAllocator");
        pos.Set("X", StringValue("ns3::UniformRandomVariable[Min="
            + std::to_string(xMin) + "|Max=" + std::to_string(xMax) + "]"));
        pos.Set("Y", StringValue("ns3::UniformRandomVariable[Min="
            + std::to_string(yMin) + "|Max=" + std::to_string(yMax) + "]"));
        Ptr<PositionAllocator> alloc = pos.Create()->GetObject<PositionAllocator>();
        mob.SetMobilityModel("ns3::RandomWaypointMobilityModel",
            "Speed", StringValue("ns3::UniformRandomVariable[Min=5|Max=15]"),
            "Pause", StringValue("ns3::UniformRandomVariable[Min=0|Max=2]"),
            "PositionAllocator", PointerValue(alloc));
        mob.SetPositionAllocator(alloc);
        mob.Install(nodes);
    };

    // Groupe 1 : zone gauche
    installMobility(drones1, 0, 100, 0, 100);
    // Groupe 2 : zone milieu
    installMobility(drones2, 100, 200, 0, 100);
    // Groupe 3 : zone droite
    installMobility(drones3, 200, 300, 0, 100);

    // Edges et GCS fixes
    MobilityHelper fixedMob;
    fixedMob.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    fixedMob.Install(edge1);
    fixedMob.Install(edge2);
    fixedMob.Install(edge3);
    fixedMob.Install(gcs);

    edge1.Get(0)->GetObject<ConstantPositionMobilityModel>()
        ->SetPosition(Vector(50.0, 50.0, 0.0));
    edge2.Get(0)->GetObject<ConstantPositionMobilityModel>()
        ->SetPosition(Vector(150.0, 50.0, 0.0));
    edge3.Get(0)->GetObject<ConstantPositionMobilityModel>()
        ->SetPosition(Vector(250.0, 50.0, 0.0));
    gcs.Get(0)->GetObject<ConstantPositionMobilityModel>()
        ->SetPosition(Vector(150.0, 0.0, 0.0));

    // ===== INTERNET + AODV =====
    AodvHelper aodv;
    InternetStackHelper internet;
    internet.SetRoutingHelper(aodv);
    internet.Install(drones1);
    internet.Install(drones2);
    internet.Install(drones3);
    internet.Install(edge1);
    internet.Install(edge2);
    internet.Install(edge3);
    internet.Install(gcs);

    // ===== ADRESSES IP =====
    Ipv4AddressHelper addr;

    addr.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer droneIf1 = addr.Assign(droneDevs1);
    Ipv4InterfaceContainer edgeIf1  = addr.Assign(edgeDev1);

    addr.SetBase("10.2.1.0", "255.255.255.0");
    Ipv4InterfaceContainer droneIf2 = addr.Assign(droneDevs2);

    Ipv4InterfaceContainer edgeIf2  = addr.Assign(edgeDev2);

    addr.SetBase("10.3.1.0", "255.255.255.0");
    Ipv4InterfaceContainer droneIf3 = addr.Assign(droneDevs3);
    Ipv4InterfaceContainer edgeIf3  = addr.Assign(edgeDev3);

    // ===== LIENS FILAIRES EDGE -> GCS =====
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("1Gbps"));
    p2p.SetChannelAttribute("Delay",   StringValue("1ms"));

    NetDeviceContainer e1gcs = p2p.Install(edge1.Get(0), gcs.Get(0));
    NetDeviceContainer e2gcs = p2p.Install(edge2.Get(0), gcs.Get(0));
    NetDeviceContainer e3gcs = p2p.Install(edge3.Get(0), gcs.Get(0));

    addr.SetBase("10.4.1.0", "255.255.255.0"); addr.Assign(e1gcs);
    addr.SetBase("10.4.2.0", "255.255.255.0"); addr.Assign(e2gcs);
    addr.SetBase("10.4.3.0", "255.255.255.0"); addr.Assign(e3gcs);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // ===== TRAFIC VIDEO =====
    uint16_t port = 9;

    for (uint32_t i = 0; i < 7; i++)
    {
        double bcDelay = 0.05 + (rand() % 100) / 1000.0;
        double startTime = 1.0 + i * 0.1 + bcDelay;
        OnOffHelper onoff("ns3::UdpSocketFactory",
            InetSocketAddress(edgeIf1.GetAddress(0), port));
        onoff.SetConstantRate(DataRate("6Mbps"), 1200);
        onoff.SetAttribute("StartTime", TimeValue(Seconds(startTime)));
        onoff.SetAttribute("StopTime",  TimeValue(Seconds(simTime)));
        onoff.Install(drones1.Get(i));
    }

    for (uint32_t i = 0; i < 7; i++)
    {
        double bcDelay = 0.05 + (rand() % 100) / 1000.0;
        double startTime = 1.0 + i * 0.1 + bcDelay;
        OnOffHelper onoff("ns3::UdpSocketFactory",
            InetSocketAddress(edgeIf2.GetAddress(0), port));
        onoff.SetConstantRate(DataRate("6Mbps"), 1200);
        onoff.SetAttribute("StartTime", TimeValue(Seconds(startTime)));
        onoff.SetAttribute("StopTime",  TimeValue(Seconds(simTime)));
        onoff.Install(drones2.Get(i));
    }

    for (uint32_t i = 0; i < 6; i++)
    {
        double bcDelay = 0.05 + (rand() % 100) / 1000.0;
        double startTime = 1.0 + i * 0.1 + bcDelay;
        OnOffHelper onoff("ns3::UdpSocketFactory",
            InetSocketAddress(edgeIf3.GetAddress(0), port));
        onoff.SetConstantRate(DataRate("6Mbps"), 1200);
        onoff.SetAttribute("StartTime", TimeValue(Seconds(startTime)));
        onoff.SetAttribute("StopTime",  TimeValue(Seconds(simTime)));
        onoff.Install(drones3.Get(i));
    }

    // ===== RECEPTEURS =====
    PacketSinkHelper sink("ns3::UdpSocketFactory",
        InetSocketAddress(Ipv4Address::GetAny(), port));

    ApplicationContainer s1 = sink.Install(edge1.Get(0));
    s1.Start(Seconds(0.0)); s1.Stop(Seconds(simTime));

    ApplicationContainer s2 = sink.Install(edge2.Get(0));
    s2.Start(Seconds(0.0)); s2.Stop(Seconds(simTime));

    ApplicationContainer s3 = sink.Install(edge3.Get(0));
    s3.Start(Seconds(0.0)); s3.Stop(Seconds(simTime));

    // ===== FLOWMONITOR =====
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();

    Simulator::Stop(Seconds(simTime));
    Simulator::Run();

    monitor->CheckForLostPackets();
    monitor->SerializeToXmlFile("iod-results-20-3edges.xml", true, true);

    Ptr<Ipv4FlowClassifier> classifier =
        DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();

    double L_cap=5.0, L_enc=0.5, L_dec=0.5, L_bc=100.0, L_tx2=1.0;

    NS_LOG_UNCOND("\n========== RESULTATS - 20 DRONES 3 EDGES ==========");
    NS_LOG_UNCOND("Architecture : 3 Edges (7+7+6 drones par Edge)");
    NS_LOG_UNCOND("Standard     : 802.11ac | 80 MHz | canaux physiques separes");
    NS_LOG_UNCOND("Duree        : " << simTime << "s\n");

    for (auto &flow : stats)
    {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(flow.first);
        if (flow.second.rxPackets == 0) continue;

        double throughput = flow.second.rxBytes * 8.0 / simTime / 1e6;
        if (throughput < 1.0) continue;

        bool toE1 = (t.destinationAddress == edgeIf1.GetAddress(0));
        bool toE2 = (t.destinationAddress == edgeIf2.GetAddress(0));
        bool toE3 = (t.destinationAddress == edgeIf3.GetAddress(0));
        if (!toE1 && !toE2 && !toE3) continue;

        double L_tx = flow.second.delaySum.GetSeconds() * 1000.0
                      / flow.second.rxPackets;
        double L_total = L_cap + L_enc + L_tx + L_bc + L_tx2 + L_dec;
        double jitter  = flow.second.rxPackets > 1
            ? flow.second.jitterSum.GetSeconds() * 1000.0
              / (flow.second.rxPackets - 1) : 0;
        double lossRate = 100.0 * flow.second.lostPackets
            / (flow.second.lostPackets + flow.second.rxPackets);

        std::string label = toE1 ? "Edge1" : (toE2 ? "Edge2" : "Edge3");

        NS_LOG_UNCOND("Flow " << flow.first
            << " (" << t.sourceAddress << " -> " << t.destinationAddress
            << " [" << label << "])"
            << "\n  Debit          : " << throughput << " Mbps"
            << "\n  L_tx (Wi-Fi)   : " << L_tx << " ms"
            << "\n  L_total        : " << L_total << " ms"
            << "\n  Jitter         : " << jitter << " ms"
            << "\n  Perte paquets  : " << lossRate << " %"
            << "\n  Blockchain     : " << L_bc << " ms (modelise)\n");
    }

    Simulator::Destroy();
    return 0;
}
