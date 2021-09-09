#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/traci-applications-module.h"
#include "ns3/network-module.h"
#include "ns3/traci-module.h"
#include "ns3/wave-module.h"
#include "ns3/ocb-wifi-mac.h"
#include "ns3/wifi-80211p-helper.h"
#include "ns3/wave-mac-helper.h"
#include "ns3/netanim-module.h"
#include <functional>
#include <stdlib.h>

#include <fstream>
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"

#include "../model/beacon-search-net.h"

using namespace ns3;
NS_LOG_COMPONENT_DEFINE ("vanet-example-simple");

void
PrintNodeRoutingTable (uint32_t nodePoolId, double timeInSeconds)
{
  RipHelper routingHelper;
  Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> (&std::cout);
  routingHelper.PrintRoutingTableAt (Seconds (timeInSeconds), NodeList::GetNode (nodePoolId),
                                     routingStream);
}

int
main (int argc, char *argv[])
{
  /*** 0. Logging Options ***/
  bool verbose = true;

  CommandLine cmd;
  cmd.Parse (argc, argv);
  if (verbose)
    {
      LogComponentEnable ("TraciClient", LOG_LEVEL_INFO);
      ///LogComponentEnable ("TrafficControlApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("vanet-example-simple", LOG_LEVEL_INFO);
      LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_ALL);
      LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_ALL);

      LogComponentEnable ("beacon-search-net", LOG_LEVEL_ALL);
    }

  /*** 1. Create node pool and counter; large enough to cover all sumo vehicles ***/
  ns3::Time simulationTime (ns3::Seconds (500));
  NodeContainer nodePool, nodesArea1;
  nodePool.Create (11);
  uint32_t nodeCounter (0);

  //Road Side Units
  Ptr<Node> RSU1 = nodePool.Get (0);
  Ptr<Node> RSU2 = nodePool.Get (1);
  Ptr<Node> RSU3 = nodePool.Get (2);
  Ptr<Node> RSU4 = nodePool.Get (3);
  Ptr<Node> RSU5 = nodePool.Get (4);
  // Server
  Ptr<Node> SRV1 = nodePool.Get (5);
  // Vehicles - cargo
  Ptr<Node> CAR1 = nodePool.Get (6);
  Ptr<Node> CAR2 = nodePool.Get (7);
  Ptr<Node> CAR3 = nodePool.Get (8);
  Ptr<Node> CAR4 = nodePool.Get (9);
  Ptr<Node> CAR5 = nodePool.Get (10);

  nodesArea1.Add (RSU1);
  nodesArea1.Add (CAR1);

  /*** 2. Create and setup channel ***/
  PointToPointHelper p2p;
  p2p.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));

  std::string phyMode ("OfdmRate6MbpsBW10MHz");
  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  wifiPhy.Set ("TxPowerStart", DoubleValue (24));
  wifiPhy.Set ("TxPowerEnd", DoubleValue (24));
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
  Ptr<YansWifiChannel> channel = wifiChannel.Create ();
  wifiPhy.SetChannel (channel);

  /*** 3. Create and setup MAC ***/
  wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11);
  NqosWaveMacHelper wifi80211pMac = NqosWaveMacHelper::Default ();
  Wifi80211pHelper wifi80211p = Wifi80211pHelper::Default ();
  wifi80211p.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode",
                                      StringValue (phyMode), "ControlMode", StringValue (phyMode));

  NetDeviceContainer wifiDevicesArea1 = wifi80211p.Install (wifiPhy, wifi80211pMac, nodesArea1);
  NetDeviceContainer wifiDevicesArea2 = wifi80211p.Install (wifiPhy, wifi80211pMac, RSU2);
  NetDeviceContainer wifiDevicesArea3 = wifi80211p.Install (wifiPhy, wifi80211pMac, RSU3);
  NetDeviceContainer wifiDevicesArea4 = wifi80211p.Install (wifiPhy, wifi80211pMac, RSU4);
  NetDeviceContainer wifiDevicesArea5 = wifi80211p.Install (wifiPhy, wifi80211pMac, RSU5);

  NetDeviceContainer p2pDevices1 = p2p.Install (NodeContainer (RSU1, RSU2));
  NetDeviceContainer p2pDevices2 = p2p.Install (NodeContainer (RSU1, RSU3));
  NetDeviceContainer p2pDevices3 = p2p.Install (NodeContainer (RSU1, RSU4));
  NetDeviceContainer p2pDevices4 = p2p.Install (NodeContainer (RSU1, RSU5));
  NetDeviceContainer p2pDevices5 = p2p.Install (NodeContainer (RSU1, SRV1));

  /*** 4. Add Internet layers stack and routing ***/
  InternetStackHelper stack;
  stack.Install (nodePool);

  /*** 5. Assign IP address to each device ***/
  Ipv4AddressHelper address;
  Ipv4InterfaceContainer p2pInterfaces;
  Ipv4InterfaceContainer wifiInterfaces;

  address.SetBase ("189.10.10.0", "255.255.255.252");
  p2pInterfaces = address.Assign (p2pDevices1);
  address.SetBase ("189.10.10.4", "255.255.255.252");
  p2pInterfaces = address.Assign (p2pDevices2);
  address.SetBase ("189.10.10.8", "255.255.255.252");
  p2pInterfaces = address.Assign (p2pDevices3);
  address.SetBase ("189.10.10.12", "255.255.255.252");
  p2pInterfaces = address.Assign (p2pDevices4);
  address.SetBase ("189.10.10.16", "255.255.255.252");
  p2pInterfaces = address.Assign (p2pDevices5);

  address.SetBase ("172.16.0.0", "255.255.0.0");
  wifiInterfaces = address.Assign (wifiDevicesArea1);
  address.SetBase ("172.17.0.0", "255.255.0.0");
  wifiInterfaces = address.Assign (wifiDevicesArea2);
  address.SetBase ("172.18.0.0", "255.255.0.0");
  wifiInterfaces = address.Assign (wifiDevicesArea3);
  address.SetBase ("172.19.0.0", "255.255.0.0");
  wifiInterfaces = address.Assign (wifiDevicesArea4);
  address.SetBase ("172.20.0.0", "255.255.0.0");
  wifiInterfaces = address.Assign (wifiDevicesArea5);

  /*** 6. Routing ***/
  Ipv4StaticRoutingHelper helper;
  Ptr<Ipv4> ipv4;
  Ipv4InterfaceAddress iaddr;
  Ipv4Address ipAddr;
  Ptr<Ipv4StaticRouting> Ipv4stat;

  ipv4 = RSU2->GetObject<Ipv4> ();
  Ipv4stat = helper.GetStaticRouting (ipv4);
  Ipv4stat->SetDefaultRoute ("189.10.10.1", 1);

  /*** 7. Setup Mobility and position node pool ***/
  MobilityHelper mobility;
  Ptr<UniformDiscPositionAllocator> positionAlloc = CreateObject<UniformDiscPositionAllocator> ();
  positionAlloc->SetX (320.0);
  positionAlloc->SetY (320.0);
  positionAlloc->SetRho (25.0);
  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (nodePool);

  RSU1->GetObject<MobilityModel> ()->SetPosition (Vector (100, 100, 3.0));
  nodeCounter++;
  RSU2->GetObject<MobilityModel> ()->SetPosition (Vector (50, 150, 3.0));
  nodeCounter++;
  RSU3->GetObject<MobilityModel> ()->SetPosition (Vector (150, 150, 3.0));
  nodeCounter++;
  RSU4->GetObject<MobilityModel> ()->SetPosition (Vector (50, 50, 3.0));
  nodeCounter++;
  RSU5->GetObject<MobilityModel> ()->SetPosition (Vector (150, 50, 3.0));
  nodeCounter++;
  SRV1->GetObject<MobilityModel> ()->SetPosition (Vector (200, 100, 0));
  nodeCounter++;

  /*** 8. Setup Traci and start SUMO ***/
  Ptr<TraciClient> sumoClient = CreateObject<TraciClient> ();
  sumoClient->SetAttribute ("SumoConfigPath",
                            StringValue ("contrib/vanetsim/traces/grid-map/sim.sumocfg"));
  sumoClient->SetAttribute ("SumoBinaryPath",
                            StringValue ("")); // use system installation of sumonodePoolId
  sumoClient->SetAttribute ("SynchInterval", TimeValue (Seconds (0.1)));
  sumoClient->SetAttribute ("StartTime", TimeValue (Seconds (0.0)));
  sumoClient->SetAttribute ("SumoGUI", BooleanValue (true));
  sumoClient->SetAttribute ("SumoPort", UintegerValue (3400));
  sumoClient->SetAttribute ("PenetrationRate",
                            DoubleValue (1.0)); // portion of vehicles equipped with wifi
  sumoClient->SetAttribute ("SumoLogFile", BooleanValue (true));
  sumoClient->SetAttribute ("SumoStepLog", BooleanValue (false));
  sumoClient->SetAttribute ("SumoSeed", IntegerValue (10));
  sumoClient->SetAttribute ("SumoAdditionalCmdOptions", StringValue ("--verbose true"));
  sumoClient->SetAttribute ("SumoWaitForSocket", TimeValue (Seconds (1.0)));

  /*** 9. Create and Setup Applications for the RSU node and set position ***/
  ///RsuSpeedControlHelper rsuSpeedControlHelper (9); // Port #9
  ///rsuSpeedControlHelper.SetAttribute ("Velocity", UintegerValue (30));           // initial velocity value which is sent to vehicles
  ///rsuSpeedControlHelper.SetAttribute ("Interval", TimeValue (Seconds (7.0)));    // packet interval
  ///rsuSpeedControlHelper.SetAttribute ("Client", (PointerValue) (sumoClient));    // pass TraciClient object for accessing sumo in application

  ///ApplicationContainer rsuSpeedControlApps = rsuSpeedControlHelper.Install (nodePool.Get (0));
  ///rsuSpeedControlApps.Start (Seconds (1.0));
  ///rsuSpeedControlApps.Stop (simulationTime);

  /*** 10. Setup interface and application for dynamic nodes ***/
  ///VehicleSpeedControlHelper vehicleSpeedControlHelper (9);
  ///vehicleSpeedControlHelper.SetAttribute ("Client", (PointerValue) sumoClient); // pass TraciClient object for accessing sumo in application

  uint16_t port = 9; // well-known echo port number
  UdpEchoServerHelper server (port);
  ApplicationContainer apps = server.Install (nodePool.Get (0));
  apps.Start (Seconds (0.1));
  apps.Stop (Seconds (500.0));

  ipv4 = RSU1->GetObject<Ipv4> ();
  iaddr = ipv4->GetAddress (1, 0);
  ipAddr = iaddr.GetLocal ();

  Address serverAddress = Address (ipAddr);
  NS_LOG_INFO ("--> " << ipAddr);

  uint32_t packetSize = 1024;
  uint32_t maxPacketCount = 50;
  Time interPacketInterval = Seconds (5.);
  UdpEchoClientHelper client (serverAddress, port);
  client.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
  client.SetAttribute ("Interval", TimeValue (interPacketInterval));
  client.SetAttribute ("PacketSize", UintegerValue (packetSize));
  apps = client.Install (nodePool.Get (5));
  apps.Start (Seconds (2.0));
  apps.Stop (Seconds (500.0));

  // callback function for node creation
  std::function<Ptr<Node> ()> setupNewWifiNode = [&] () -> Ptr<Node> {
    if (nodeCounter >= nodePool.GetN ())
      NS_FATAL_ERROR ("Node Pool empty!: " << nodeCounter << " nodes created.");

    // don't create and install the protocol stack of the node at simulation time -> take from "node pool"
    Ptr<Node> includedNode = nodePool.Get (nodeCounter);
    ++nodeCounter; // increment counter for next node

    // Install Application
    ///ApplicationContainer vehicleSpeedControlApps = vehicleSpeedControlHelper.Install (includedNode);
    ///vehicleSpeedControlApps.Start (Seconds (0.0));
    ///vehicleSpeedControlApps.Stop (simulationTime);

    return includedNode;
  };

  // callback function for node shutdown
  std::function<void (Ptr<Node>)> shutdownWifiNode = [] (Ptr<Node> exNode) {
    // stop all applications
    ///Ptr<VehicleSpeedControl> vehicleSpeedControl = exNode->GetApplication(0)->GetObject<VehicleSpeedControl>();
    ///if(vehicleSpeedControl)
    ///  vehicleSpeedControl->StopApplicationNow();

    // set position outside communication range
    Ptr<ConstantPositionMobilityModel> mob = exNode->GetObject<ConstantPositionMobilityModel> ();
    mob->SetPosition (Vector (-100.0 + (rand () % 25), 320.0 + (rand () % 25),
                              250.0)); // rand() for visualization purposes

    // NOTE: further actions could be required for a save shut down!
  };

  Ptr<BeaconSearchNet> app_i = CreateObject<BeaconSearchNet> ();
  app_i->SetStartTime (Seconds (5));
  app_i->SetStopTime (Seconds (500));
  CAR1->AddApplication (app_i);

  Ptr<BeaconSearchNet> app_rsu = CreateObject<BeaconSearchNet> ();
  app_i->SetStartTime (Seconds (5));
  app_i->SetStopTime (Seconds (500));
  RSU1->AddApplication (app_rsu);
  

  // start traci client with given function pointers
  sumoClient->SumoSetup (setupNewWifiNode, shutdownWifiNode);
  PrintNodeRoutingTable (0, 10.0);
  PrintNodeRoutingTable (1, 10.0);

  /*** 10. Setup and Start Simulation + Animation ***/
  Simulator::Stop (simulationTime);

  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}