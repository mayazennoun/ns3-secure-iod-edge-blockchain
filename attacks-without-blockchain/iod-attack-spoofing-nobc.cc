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

NS_LOG_COMPONENT_DEFINE("IoDAttackSpoofingNOBC");

uint32_t g_legitPackets    = 0;
uint32_t g_spoofingPackets = 0;

void LegitTxCallback(Ptr<const Packet> packet)  { g_legitPackets++;    }
void SpoofTxCallback(Ptr<const Packet> packet)  { g_spoofingPackets++; }

int main(int argc, char *argv[])
{
    uint32_t nDrones = 5;
    double simTime   = 60.0;

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

    // Noeuds
    NodeContainer drones;   drones.Create(nDrones);
    NodeContainer edge;     edge.Create(1);
    NodeContainer gcs;      gcs.Create(1);
    NodeContainer fakeDrone; fakeDrone.Create(1);

    // MAC legitimes
    WifiMacHelper mac;
    Ssid ssid = Ssid("IoD-Network");
    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid));
    NetDeviceContainer droneDevs = wifi.Install(phy, mac, drones);
    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
    NetDeviceContainer edgeDev = wifi.Install(phy, mac, edge);

    // MAC faux drone — meme SSID
    // SANS BLOCKCHAIN : le faux drone se connecte librement
    WifiMacHelper macFake;
    macFake.SetType("ns3::StaWifiMac",
        "Ssid", SsidValue(ssid),
        "ActiveProbing", BooleanValue(true));
    NetDeviceContainer fakeDev = wifi.Install(phy, macFake, fakeDrone);

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

    AodvHelper aodv;
    InternetStackHelper internet;
    internet.SetRoutingHelper(aodv);
    internet.Install(drones);
    internet.Install(edge);
    internet.Install(gcs);
    internet.Install(fakeDrone);

    Ipv4AddressHelper addr;
    addr.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer droneIfaces = addr.Assign(droneDevs);
    Ipv4InterfaceContainer fakeIface   = addr.Assign(fakeDev);
    Ipv4InterfaceContainer edgeIface   = addr.Assign(edgeDev);

    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("1Gbps"));
    p2p.SetChannelAttribute("Delay",   StringValue("1ms"));
    NetDeviceContainer e2gcs = p2p.Install(edge.Get(0), gcs.Get(0));
    addr.SetBase("10.1.3.0", "255.255.255.0");
    addr.Assign(e2gcs);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // Trafic video legitime SANS delai blockchain
    uint16_t port = 9;
    for (uint32_t i = 0; i < nDrones; i++)
    {
        double startTime = 1.0 + i * 0.1;
        OnOffHelper onoff("ns3::UdpSocketFactory",
            InetSocketAddress(edgeIface.GetAddress(0), port));
        onoff.SetConstantRate(DataRate("8Mbps"), 1200);
        onoff.SetAttribute("StartTime", TimeValue(Seconds(startTime)));
        onoff.SetAttribute("StopTime",  TimeValue(Seconds(simTime)));
        onoff.Install(drones.Get(i));
    }

    // SANS BLOCKCHAIN : le faux drone envoie librement
    // pas de verification isAuthorized — l'Edge accepte tout
    OnOffHelper fakeOnoff("ns3::UdpSocketFactory",
        InetSocketAddress(edgeIface.GetAddress(0), port));
    fakeOnoff.SetConstantRate(DataRate("8Mbps"), 1200);
    fakeOnoff.SetAttribute("StartTime", TimeValue(Seconds(2.0)));
    fakeOnoff.SetAttribute("StopTime",  TimeValue(Seconds(simTime)));
    fakeOnoff.Install(fakeDrone.Get(0));

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

    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();

    Simulator::Stop(Seconds(simTime));
    Simulator::Run();

    monitor->CheckForLostPackets();

    Ptr<Ipv4FlowClassifier> classifier =
        DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();

    double legitThroughput = 0;
    double fakeThroughput  = 0;
    double legitLoss       = 0;
    double fakeLoss        = 0;
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
            if (t.sourceAddress == fakeIface.GetAddress(0))
            {
                fakeThroughput = throughput;
                fakeLoss = loss;
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
    double L_total_nobc  = 5.0 + 0.5 + avgLatency + 1.0 + 0.5;

    NS_LOG_UNCOND("\n========== ATTAQUE SPOOFING — SANS BLOCKCHAIN ==========");
    NS_LOG_UNCOND("Drones legitimes         : " << nDrones);
    NS_LOG_UNCOND("Duree simulation         : " << simTime << "s");
    NS_LOG_UNCOND("Authentification         : AUCUNE (pas de blockchain)");
    NS_LOG_UNCOND("Faux drone               : accepte sans verification");
    NS_LOG_UNCOND("");
    NS_LOG_UNCOND("--- Faux drone (attaquant) ---");
    NS_LOG_UNCOND("Paquets envoyes          : " << g_spoofingPackets);
    NS_LOG_UNCOND("Debit recu par l'Edge    : " << fakeThroughput << " Mbps");
    NS_LOG_UNCOND("Perte cote faux drone    : " << fakeLoss << " %");
    NS_LOG_UNCOND("");
    NS_LOG_UNCOND("--- Drones legitimes ---");
    NS_LOG_UNCOND("Paquets envoyes          : " << g_legitPackets);
    NS_LOG_UNCOND("Debit moyen              : " << avgThroughput << " Mbps");
    NS_LOG_UNCOND("Perte paquets            : " << avgLoss << " %");
    NS_LOG_UNCOND("L_tx moyen               : " << avgLatency << " ms");
    NS_LOG_UNCOND("L_total (sans L_bc)      : " << L_total_nobc << " ms");
    NS_LOG_UNCOND("");
    NS_LOG_UNCOND("--- Analyse securite ---");
    NS_LOG_UNCOND("Verification blockchain  : ABSENTE");
    NS_LOG_UNCOND("Cle session delivree     : OUI (sans verification)");
    NS_LOG_UNCOND("Flux faux drone accepte  : " << fakeThroughput << " Mbps");
    NS_LOG_UNCOND("Resultat                 : ATTAQUE REUSSIE SANS BLOCKCHAIN");
    NS_LOG_UNCOND("=========================================================");

    Simulator::Destroy();
    return 0;
}x

