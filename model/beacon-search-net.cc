#include "ns3/mobility-model.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "beacon-search-net.h"
#include "custom-data-tag.h"
#include "ns3/random-variable-stream.h"
#include "ns3/internet-module.h"
#include "ns3/udp-echo-client.h"

#include <bitset>
#include <bits/stdc++.h>

#define RED_CODE "\033[91m"
#define GREEN_CODE "\033[32m"
#define END_CODE "\033[0m"

namespace ns3 {
NS_LOG_COMPONENT_DEFINE ("beacon-search-net");
NS_OBJECT_ENSURE_REGISTERED (BeaconSearchNet);

TypeId
BeaconSearchNet::GetTypeId ()
{
  static TypeId tid =
      TypeId ("ns3::BeaconSearchNet")
          .SetParent<Application> ()
          .AddConstructor<BeaconSearchNet> ()
          .AddAttribute ("Interval", "Broadcast Interval", TimeValue (MilliSeconds (1000)),
                         MakeTimeAccessor (&BeaconSearchNet::m_broadcast_time), MakeTimeChecker ())
          .AddAttribute ("Pktsize", "Packet Size", IntegerValue (1000),
                         MakeIntegerAccessor (&BeaconSearchNet::m_packetSize),
                         MakeIntegerChecker<uint32_t> ());
  return tid;
}

TypeId
BeaconSearchNet::GetInstanceTypeId () const
{
  return BeaconSearchNet::GetTypeId ();
}

BeaconSearchNet::BeaconSearchNet ()
{
}

BeaconSearchNet::~BeaconSearchNet ()
{
}

void
BeaconSearchNet::StartApplication ()
{
  NS_LOG_FUNCTION (this);

  Ptr<Node> n = GetNode ();

  for (uint32_t i = 0; i < n->GetNDevices (); i++)
    {
      Ptr<NetDevice> dev = n->GetDevice (i);
      //NS_LOG_INFO ("" << dev->GetInstanceTypeId ().GetName ());

      if (dev->GetInstanceTypeId () == WifiNetDevice::GetTypeId ())
        {
          m_wifiDevice = DynamicCast<WifiNetDevice> (dev);
          m_rsuConnected = 9999; // out - disconnected
          //ReceivePacket will be called when a packet is received
          dev->SetReceiveCallback (MakeCallback (&BeaconSearchNet::ReceivePacket, this));

          /*
            If you want promiscous receive callback, connect to this trace. 
            For every packet received, both functions ReceivePacket & PromiscRx will be called. with PromicRx being called first!
            */
          Ptr<WifiPhy> phy = m_wifiDevice->GetPhy (); //default, there's only one PHY
          phy->TraceConnectWithoutContext ("MonitorSnifferRx",
                                           MakeCallback (&BeaconSearchNet::PromiscRx, this));
          break;
        }
    }
  if (m_wifiDevice)
    {
      //Let's create a bit of randomness with the first broadcast packet time to avoid collision
      Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable> ();
      Time random_offset = MicroSeconds (rand->GetValue (50, 200));

      Simulator::Schedule (m_broadcast_time + random_offset, &BeaconSearchNet::CheckHandoverProcess,
                           this);
    }
  else
    {
      NS_FATAL_ERROR ("There's no WifiNetDevice in your node");
    }
}

void
BeaconSearchNet::CheckHandoverProcess ()
{
  NS_LOG_FUNCTION (this);

  uint32_t ipRSUHandover = HandoverStrategy ();

  if (ipRSUHandover) // if 0 >> handover is not necessary
    {
      Ptr<Packet> packet = Create<Packet> (m_packetSize);
      CustomDataTag tag;

      tag.SetNodeId (GetNode ()->GetId ());
      tag.SetPosition (GetNode ()->GetObject<MobilityModel> ()->GetPosition ());
      //timestamp is set in the default constructor of the CustomDataTag class as Simulator::Now()
      tag.SetIpAddr (ipRSUHandover); //RSU ip address responsible to manager the handover
      tag.PrepareHeaderDhcpMessage ();

      //attach the tag to the packet
      packet->AddPacketTag (tag);
      m_wifiDevice->Send (packet, Mac48Address::GetBroadcast (), 0xFE);
    }
  //Schedule next handover event
  Simulator::Schedule (m_broadcast_time, &BeaconSearchNet::CheckHandoverProcess, this);
}

bool
BeaconSearchNet::ReceivePacket (Ptr<NetDevice> device, Ptr<const Packet> packet, uint16_t protocol,
                                const Address &sender)
{
  NS_LOG_FUNCTION (device << packet << protocol << sender);

  //Let's check if packet has a tag attached!
  CustomDataTag tag;
  if (packet->PeekPacketTag (tag) && tag.isDhcpMessage ())
    {
      Ipv4Address newIpAddr;
      newIpAddr.Set (tag.GetIpAddr ());

      //NS_LOG_INFO ("Teste var1: " << tag.GetNodeId ());
      //NS_LOG_INFO ("Teste var2 : " << std::bitset<32> (newIpAddr.Get ()));

      Ptr<Ipv4> ipv4 = GetNode ()->GetObject<Ipv4> (); // Get Ipv4 instance of the node
      int32_t interface = ipv4->GetInterfaceForDevice (device);
      std::string convertMask = "/" + std::to_string (tag.GetMask ());

      Ipv4InterfaceAddress ipv4Addr =
          Ipv4InterfaceAddress (Ipv4Address (newIpAddr), Ipv4Mask (convertMask.c_str ()));

      Ipv4StaticRoutingHelper helper;
      Ptr<Ipv4StaticRouting> Ipv4stat;

      Ipv4stat = helper.GetStaticRouting (ipv4);
      //Ipv4stat->UpdateSortedArray ();

      //changing ip address
      ipv4->RemoveAddress (interface, 0);

      ipv4->AddAddress (interface, ipv4Addr);
      ipv4->SetMetric (interface, 1);
      //ipv4->SetUp (interface);
      m_rsuConnected = tag.GetNodeId ();

      ipv4 = GetNode ()->GetObject<Ipv4> ();
      Ipv4stat = helper.GetStaticRouting (ipv4);
      //Ipv4stat->RemoveRoute (1);

      NS_LOG_INFO (GREEN_CODE << "vehicle-id=" << GetNode ()->GetId ()
                              << " is now connected to RSU-id=" << m_rsuConnected << END_CODE);
      NS_LOG_INFO (GREEN_CODE << "vehicle-id=" << GetNode ()->GetId ()
                              << " has new ipv4 address: " << newIpAddr << " ("
                              << std::bitset<32> (newIpAddr.Get ()) << ")" << END_CODE);
    }
  return true;
}

void
BeaconSearchNet::PromiscRx (Ptr<const Packet> packet, uint16_t channelFreq, WifiTxVector tx,
                            MpduInfo mpdu, SignalNoiseDbm sn)
{
  NS_LOG_FUNCTION (packet << channelFreq << tx);

  {
    CustomDataTag tag;
    //Let's check if packet has a tag attached and it is HelloMessage
    if (packet->PeekPacketTag (tag) && tag.isHelloMessage ())
      {
        BEACONRECEIVED beaconRecvTemp;

        beaconRecvTemp.rsuId = tag.GetNodeId ();
        beaconRecvTemp.ipAddr = tag.GetIpAddr ();
        beaconRecvTemp.mask = tag.GetMask ();
        // can be tag.GetTimestamp ();
        beaconRecvTemp.timestamp = (ns3::Time) ns3::Simulator::Now ();

        beaconRecvTemp.signal = sn.signal;
        beaconRecvTemp.noise = sn.noise;
        // store the beacon received
        beaconsReceived.emplace_back (beaconRecvTemp);
      }
  }
}

//** Customize your RSU handover strategy here */
u_int32_t
BeaconSearchNet::HandoverStrategy ()
{
  //NS_LOG_INFO (RED_CODE << "m_rsuConnected=" << m_rsuConnected << END_CODE);
  Time max_interval = 2 * m_broadcast_time; // ms
  uint32_t ipRSUHandover = 0;
  bool isHandoverNecessary = true;

  for (size_t i = 0; i < beaconsReceived.size (); i++)
    {
      if (m_rsuConnected == beaconsReceived.at (i).rsuId &&
          ((Now ().GetMilliSeconds () - beaconsReceived.at (i).timestamp.GetMilliSeconds ()) <
           max_interval.GetMilliSeconds ()))
        {
          isHandoverNecessary = false;
        }
    }

  if (isHandoverNecessary)
    for (size_t i = 0; i < beaconsReceived.size (); i++)
      if (Now ().GetMilliSeconds () - beaconsReceived.at (i).timestamp.GetMilliSeconds () <
              max_interval.GetMilliSeconds () ||
          m_rsuConnected == 9999)
        {
          NS_LOG_INFO (RED_CODE << "Handover process will be necessary for nodeId="
                                << GetNode ()->GetId () << END_CODE);
          ipRSUHandover = beaconsReceived.at (i).ipAddr;
          break;
        }

  return ipRSUHandover;
}

} // namespace ns3
