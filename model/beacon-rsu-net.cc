#include "ns3/mobility-model.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "beacon-rsu-net.h"
#include "custom-data-tag.h"
#include "ns3/random-variable-stream.h"
#include "ns3/internet-module.h"

#include <bitset>
#include <bits/stdc++.h>

#define RED_CODE "\033[91m"
#define GREEN_CODE "\033[32m"
#define END_CODE "\033[0m"

namespace ns3 {
NS_LOG_COMPONENT_DEFINE ("beacon-rsu-net");
NS_OBJECT_ENSURE_REGISTERED (BeaconRsuNet);

TypeId
BeaconRsuNet::GetTypeId ()
{
  static TypeId tid =
      TypeId ("ns3::BeaconRsuNet")
          .SetParent<Application> ()
          .AddConstructor<BeaconRsuNet> ()
          .AddAttribute ("Interval", "Broadcast Interval", TimeValue (MilliSeconds (1000)),
                         MakeTimeAccessor (&BeaconRsuNet::m_broadcast_time), MakeTimeChecker ())
          .AddAttribute ("Pktsize", "Packet Size", IntegerValue (1000),
                         MakeIntegerAccessor (&BeaconRsuNet::m_packetSize),
                         MakeIntegerChecker<uint32_t> ());
  return tid;
}

TypeId
BeaconRsuNet::GetInstanceTypeId () const
{
  return BeaconRsuNet::GetTypeId ();
}

BeaconRsuNet::BeaconRsuNet ()
{
}

BeaconRsuNet::~BeaconRsuNet ()
{
}

void
BeaconRsuNet::StartApplication ()
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
          //ReceivePacket will be called when a packet is received
          dev->SetReceiveCallback (MakeCallback (&BeaconRsuNet::ReceivePacket, this));

          /*
            If you want promiscous receive callback, connect to this trace. 
            For every packet received, both functions ReceivePacket & PromiscRx will be called. with PromicRx being called first!
            */
          Ptr<WifiPhy> phy = m_wifiDevice->GetPhy (); //default, there's only one PHY
          phy->TraceConnectWithoutContext ("MonitorSnifferRx",
                                           MakeCallback (&BeaconRsuNet::PromiscRx, this));
          break;
        }
    }
  if (m_wifiDevice)
    {
      //Let's create a bit of randomness with the first broadcast packet time to avoid collision
      Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable> ();
      Time random_offset = MicroSeconds (rand->GetValue (50, 200));

      Simulator::Schedule (m_broadcast_time + random_offset, &BeaconRsuNet::BroadcastInformation,
                           this);
    }
  else
    {
      NS_FATAL_ERROR ("There's no WifiNetDevice in your node");
    }
}

void
BeaconRsuNet::BroadcastInformation ()
{
  NS_LOG_FUNCTION (this);

  //Broadcast packets - beacon or hello message (RSU area alert)
  Ptr<Packet> packet = Create<Packet> (m_packetSize);
  Ptr<Ipv4> ipv4 = GetNode ()->GetObject<Ipv4> ();
  //The interface index 0 is a loopback interface which gives 127.0.0.1 address
  Ipv4InterfaceAddress iaddr = ipv4->GetAddress (1, 0);
  Ipv4Address ipAddr = iaddr.GetLocal ();

  //Let's attach our custom data tag to it
  CustomDataTag tag;
  tag.SetNodeId (GetNode ()->GetId ());
  tag.SetPosition (GetNode ()->GetObject<MobilityModel> ()->GetPosition ());
  //timestamp is set in the default constructor of the CustomDataTag class as Simulator::Now()
  tag.SetIpAddr (ipAddr.Get ()); //RSU ip address
  tag.SetMask (iaddr.GetMask ().GetPrefixLength ()); //RSU
  tag.PrepareHeaderHelloMessage ();

  //attach the tag to the packet
  packet->AddPacketTag (tag);
  m_wifiDevice->Send (packet, Mac48Address::GetBroadcast (), 0xFE);
  //Schedule next broadcast event
  Simulator::Schedule (m_broadcast_time, &BeaconRsuNet::BroadcastInformation, this);
}

bool
BeaconRsuNet::ReceivePacket (Ptr<NetDevice> device, Ptr<const Packet> packet, uint16_t protocol,
                             const Address &sender)
{
  NS_LOG_FUNCTION (device << packet << protocol << sender);
  return true;
}

void
BeaconRsuNet::PromiscRx (Ptr<const Packet> packet, uint16_t channelFreq, WifiTxVector tx,
                         MpduInfo mpdu, SignalNoiseDbm sn)
{

  NS_LOG_FUNCTION (packet << channelFreq << tx);
}

uint32_t
BeaconRsuNet::DhcpService ()
{
  Ptr<Ipv4> ipv4 = GetNode ()->GetObject<Ipv4> ();
  Ipv4InterfaceAddress iaddr = ipv4->GetAddress (1, 0);
  Ipv4Address ipAddr = iaddr.GetLocal ();

  std::bitset<32> sum; //rename unknown
  do
    {
      std::bitset<32> first (ipAddr.Get ());
      std::bitset<1> plusOne (1);
      sum = first.to_ulong () + plusOne.to_ulong ();

      ipAddr.Set (sum.to_ulong ());

  } while (m_ipAddrUsed.find (ipAddr.Get ()) != m_ipAddrUsed.end ());

  m_ipAddrUsed.emplace (ipAddr.Get (), 0);
  NS_LOG_INFO ("____) " << m_ipAddrUsed.size ());

  return (uint32_t) sum.to_ulong ();
}

} // namespace ns3
