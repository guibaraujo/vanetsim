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
#include "../model/beacon-rsu-net.h"

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

      //LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_ALL);
      //LogComponentEnable ("UdpEchoClientApplication", LOG_PREFIX_ALL);

      //LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);

      LogComponentEnable ("beacon-search-net", LOG_LEVEL_ALL);
      LogComponentEnable ("beacon-search-net", LOG_PREFIX_ALL);
      LogComponentEnable ("beacon-rsu-net", LOG_LEVEL_ALL);
      LogComponentEnable ("beacon-rsu-net", LOG_PREFIX_ALL);
    }

  /*** 1. Create node pool and counter; large enough to cover all sumo vehicles ***/
  ns3::Time simulationTime (ns3::Seconds (500));
  NodeContainer nodePool, nodesVehicles;
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

  nodesVehicles.Add (CAR1);
  nodesVehicles.Add (CAR2);
  nodesVehicles.Add (CAR3);
  nodesVehicles.Add (CAR4);
  nodesVehicles.Add (CAR5);

  /*** 2. Create and setup channel ***/
  PointToPointHelper p2p;
  p2p.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));

  std::string phyMode ("OfdmRate6MbpsBW10MHz");
  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  wifiPhy.Set ("TxPowerStart", DoubleValue (25));
  wifiPhy.Set ("TxPowerEnd", DoubleValue (25));
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
  Ptr<YansWifiChannel> channel = wifiChannel.Create ();
  wifiPhy.SetChannel (channel);

  /*** 3. Create and setup MAC ***/
  wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11);
  NqosWaveMacHelper wifi80211pMac = NqosWaveMacHelper::Default ();
  Wifi80211pHelper wifi80211p = Wifi80211pHelper::Default ();
  wifi80211p.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode",
                                      StringValue (phyMode), "ControlMode", StringValue (phyMode));

  NetDeviceContainer wifiDevicesArea1 = wifi80211p.Install (wifiPhy, wifi80211pMac, RSU1);
  NetDeviceContainer wifiDevicesArea2 = wifi80211p.Install (wifiPhy, wifi80211pMac, RSU2);
  NetDeviceContainer wifiDevicesArea3 = wifi80211p.Install (wifiPhy, wifi80211pMac, RSU3);
  NetDeviceContainer wifiDevicesArea4 = wifi80211p.Install (wifiPhy, wifi80211pMac, RSU4);
  NetDeviceContainer wifiDevicesArea5 = wifi80211p.Install (wifiPhy, wifi80211pMac, RSU5);
  NetDeviceContainer wifiDevicesVehicles =
      wifi80211p.Install (wifiPhy, wifi80211pMac, nodesVehicles);

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
  Ipv4InterfaceContainer wifiInterfaces;
  Ipv4InterfaceContainer p2pInterfaces;

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

  address.SetBase ("169.254.0.0", "255.255.255.0");
  wifiInterfaces = address.Assign (wifiDevicesVehicles);

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

  Ptr<BeaconSearchNet> appBeaconSearchNet[5];
  for (size_t i = 0; i < 1; i++)
    {
      appBeaconSearchNet[i] = CreateObject<BeaconSearchNet> ();
      appBeaconSearchNet[i]->SetStartTime (Seconds (5));
      appBeaconSearchNet[i]->SetStopTime (Seconds (500));
    }
  CAR1->AddApplication (appBeaconSearchNet[0]);

  Ptr<BeaconRsuNet> appBeaconRsuNet[5];
  for (size_t i = 0; i < 5; i++)
    {
      appBeaconRsuNet[i] = CreateObject<BeaconRsuNet> ();
      appBeaconRsuNet[i]->SetStartTime (Seconds (5));
      appBeaconRsuNet[i]->SetStopTime (Seconds (500));
    }
  RSU1->AddApplication (appBeaconRsuNet[0]);
  RSU2->AddApplication (appBeaconRsuNet[1]);
  RSU3->AddApplication (appBeaconRsuNet[2]);
  RSU4->AddApplication (appBeaconRsuNet[3]);
  RSU5->AddApplication (appBeaconRsuNet[4]);

  // start traci client with given function pointers
  sumoClient->SumoSetup (setupNewWifiNode, shutdownWifiNode);
  PrintNodeRoutingTable (0, 10.0);
  PrintNodeRoutingTable (6, 1.0);
  PrintNodeRoutingTable (6, 10.0);

  /*** 10. Setup and Start Simulation + Animation ***/
  Simulator::Stop (simulationTime);

  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}