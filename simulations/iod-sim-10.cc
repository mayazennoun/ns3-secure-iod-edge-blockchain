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

NS_LOG_COMPONENT_DEFINE("IoDSim10");

int main(int argc, char *argv[])
{
    uint32_t nDrones = 10;
    double simTime = 300.0;

    CommandLine cmd;
    cmd.AddValue("nDrones", "Nombre de drones", nDrones);
    cmd.Parse(argc, argv);

    // Wi-Fi 802.11ac 80 MHz
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211ac);
    wifi.SetRemoteStationManager("ns3::MinstrelHtWifiManager");

    // Canal : Log-distance + Nakagami fading
    YansWifiChannelHelper channel;
    channel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
    channel.AddPropagationLoss("ns3::LogDistancePropagationLossModel",
        "Exponent", DoubleValue(2.5),
        "ReferenceDistance", DoubleValue(1.0),
        "ReferenceLoss", DoubleValue(46.67));
    channel.AddPropagationLoss("ns3::NakagamiPropagationLossModel",
        "m0", DoubleValue(1.0),
        "m1", DoubleValue(1.0),
        "m2", DoubleValue(1.0));

    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());
    phy.Set("TxPowerStart", DoubleValue(23.0));
    phy.Set("TxPowerEnd", DoubleValue(23.0));
    // 80 MHz bandwidth
    phy.Set("ChannelSettings", StringValue("{0, 80, BAND_5GHZ, 0}"));

    // Nœuds
    NodeContainer drones;
    drones.Create(nDrones);
    NodeContainer edge;
    edge.Create(1);
    NodeContainer gcs;
    gcs.Create(1);

    // MAC
    WifiMacHelper mac;
    Ssid ssid = Ssid("IoD-Network-10");

    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid));
    NetDeviceContainer droneDevices = wifi.Install(phy, mac, drones);

    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
    NetDeviceContainer edgeDevice = wifi.Install(phy, mac, edge);

    // Mobilité Random Waypoint
    MobilityHelper mobility;
    ObjectFactory pos;
    pos.SetTypeId("ns3::RandomRectanglePositionAllocator");
    pos.Set("X", StringValue("ns3::UniformRandomVariable[Min=0|Max=100]"));
    pos.Set("Y", StringValue("ns3::UniformRandomVariable[Min=0|Max=100]"));
    Ptr<PositionAllocator> posAlloc = pos.Create()->GetObject<PositionAllocator>();

    mobility.SetMobilityModel("ns3::RandomWaypointMobilityModel",
        "Speed", StringValue("ns3::UniformRandomVariable[Min=5|Max=15]"),
        "Pause", StringValue("ns3::UniformRandomVariable[Min=0|Max=2]"),
        "PositionAllocator", PointerValue(posAlloc));
    mobility.SetPositionAllocator(posAlloc);
    mobility.Install(drones);

    MobilityHelper fixedMobility;
    fixedMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    fixedMobility.Install(edge);
    fixedMobility.Install(gcs);

    Ptr<ConstantPositionMobilityModel> edgePos = edge.Get(0)->GetObject<ConstantPositionMobilityModel>();
    edgePos->SetPosition(Vector(50.0, 50.0, 0.0));

    // Internet + AODV
    AodvHelper aodv;
    InternetStackHelper internet;
    internet.SetRoutingHelper(aodv);
    internet.Install(drones);
    internet.Install(edge);
    internet.Install(gcs);

    // Adresses IP
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer droneInterfaces = address.Assign(droneDevices);
    Ipv4InterfaceContainer edgeWifiInterface = address.Assign(edgeDevice);

    // Lien filaire Edge → GCS
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("1Gbps"));
    p2p.SetChannelAttribute("Delay", StringValue("1ms"));
    NetDeviceContainer edgeGcsDevices = p2p.Install(edge.Get(0), gcs.Get(0));

    address.SetBase("10.1.2.0", "255.255.255.0");
    address.Assign(edgeGcsDevices);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // Trafic vidéo UDP 6Mbps par drone
    uint16_t port = 9;
    for (uint32_t i = 0; i < nDrones; i++)
    {
        // délai blockchain aléatoire 50-150ms
        double bcDelay = 0.05 + (rand() % 100) / 1000.0;
        double startTime = 1.0 + i * 0.1 + bcDelay;

        OnOffHelper onoff("ns3::UdpSocketFactory",
            InetSocketAddress(edgeWifiInterface.GetAddress(0), port));
        onoff.SetConstantRate(DataRate("6Mbps"), 1200);
        onoff.SetAttribute("StartTime", TimeValue(Seconds(startTime)));
        onoff.SetAttribute("StopTime", TimeValue(Seconds(simTime)));
        onoff.Install(drones.Get(i));
    }

    // Récepteur sur Edge
    PacketSinkHelper sink("ns3::UdpSocketFactory",
        InetSocketAddress(Ipv4Address::GetAny(), port));
    ApplicationContainer sinkApp = sink.Install(edge.Get(0));
    sinkApp.Start(Seconds(0.0));
    sinkApp.Stop(Seconds(simTime));

    // FlowMonitor
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();

    Simulator::Stop(Seconds(simTime));
    Simulator::Run();

    monitor->CheckForLostPackets();
    monitor->SerializeToXmlFile("iod-results-10.xml", true, true);

    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();

    // constantes
    double L_cap = 5.0;
    double L_enc = 0.5;
    double L_dec = 0.5;
    double L_bc  = 100.0;
    double L_tx2 = 1.0;

    NS_LOG_UNCOND("\n========== RÉSULTATS SIMULATION IoD - 10 DRONES ==========");
    NS_LOG_UNCOND("Durée simulation : " << simTime << "s\n");

    for (auto &flow : stats)
    {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(flow.first);

        if (flow.second.rxPackets == 0) continue;

        if (t.destinationAddress != edgeWifiInterface.GetAddress(0)) continue;

        double throughput = flow.second.rxBytes * 8.0 / simTime / 1e6;
        if (throughput < 1.0) continue;

        double L_tx = flow.second.delaySum.GetSeconds() * 1000.0 / flow.second.rxPackets;
        double L_total = L_cap + L_enc + L_tx + L_bc + L_tx2 + L_dec;
        double jitter = flow.second.rxPackets > 1 ?
            flow.second.jitterSum.GetSeconds() * 1000.0 / (flow.second.rxPackets - 1) : 0;
        double lossRate = 100.0 * flow.second.lostPackets /
            (flow.second.lostPackets + flow.second.rxPackets);

        NS_LOG_UNCOND("Flow " << flow.first
            << " (" << t.sourceAddress << " → " << t.destinationAddress << ")"
            << "\n  Débit          : " << throughput << " Mbps"
            << "\n  L_tx (Wi-Fi)   : " << L_tx << " ms"
            << "\n  L_total        : " << L_total << " ms"
            << "\n  Jitter         : " << jitter << " ms"
            << "\n  Perte paquets  : " << lossRate << " %"
            << "\n  Blockchain     : " << L_bc << " ms (modélisé)\n");
    }

    Simulator::Destroy();
    return 0;
}
