#include "ns3/mobility-model.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "beacon-search-net.h"
#include "custom-data-tag.h"
#include "ns3/random-variable-stream.h"

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
      TypeId ("ns3::Beacon")
          .SetParent<Application> ()
          .AddConstructor<BeaconSearchNet> ()
          .AddAttribute ("Interval", "Broadcast Interval", TimeValue (MilliSeconds (1000)),
                         MakeTimeAccessor (&BeaconSearchNet::m_broadcast_time), MakeTimeChecker ());
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
  //Set A Receive callback
  Ptr<Node> n = GetNode ();
  NS_LOG_INFO (">>>" << n->GetId ());
  for (uint32_t i = 0; i < n->GetNDevices (); i++)
    {
      Ptr<NetDevice> dev = n->GetDevice (i);
      NS_LOG_INFO ("" << dev->GetInstanceTypeId ().GetName ());
      if (dev->GetInstanceTypeId () == WifiNetDevice::GetTypeId ())
        {
          m_wifiDevice = DynamicCast<WifiNetDevice> (dev);
          //ReceivePacket will be called when a packet is received
          dev->SetReceiveCallback (MakeCallback (&BeaconSearchNet::ReceivePacket, this));

          /*
            If you want promiscous receive callback, connect to this trace. 
            For every packet received, both functions ReceivePacket & PromiscRx will be called. with PromicRx being called first!
            */
          Ptr<WifiPhy> phy =
              m_wifiDevice->GetPhy (); //default, there's only one PHY in a WaveNetDevice
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

      Simulator::Schedule (m_broadcast_time + random_offset, &BeaconSearchNet::BroadcastInformation,
                           this);
    }
  else
    {
      NS_FATAL_ERROR ("There's no WaveNetDevice in your node");
    }
}

void
BeaconSearchNet::BroadcastInformation ()
{
  NS_LOG_FUNCTION (this);
  //Setup transmission parameters
  TxInfo tx;
  tx.channelNumber = CCH;
  tx.preamble = WIFI_PREAMBLE_LONG;
  tx.priority = 7; //highest priority.
  tx.txPowerLevel = 7;
  tx.dataRate = WifiMode ("OfdmRate6MbpsBW10MHz");

  m_packetSize = 1000;
  Ptr<Packet> packet = Create<Packet> (m_packetSize);

  //let's attach our custom data tag to it
  CustomDataTag tag;
  tag.SetNodeId (GetNode ()->GetId ());
  tag.SetPosition (GetNode ()->GetObject<MobilityModel> ()->GetPosition ());
  //timestamp is set in the default constructor of the CustomDataTag class as Simulator::Now()

  //attach the tag to the packet
  packet->AddPacketTag (tag);

  //Broadcast the packet
  Ptr<Node> n = GetNode ();
  if (n->GetId () > 5) // only vehicles send beacons - create a node type for this app
    m_wifiDevice->Send (packet, Mac48Address::GetBroadcast (), 0xFE);

  //Schedule next broadcast
  Simulator::Schedule (m_broadcast_time, &BeaconSearchNet::BroadcastInformation, this);
}

bool
BeaconSearchNet::ReceivePacket (Ptr<NetDevice> device, Ptr<const Packet> packet, uint16_t protocol,
                                const Address &sender)
{
  NS_LOG_FUNCTION (device << packet << protocol << sender);
  /*
        Packets received here only have Application data, no WifiMacHeader. 
        We created packets with 1000 bytes payload, so we'll get 1000 bytes of payload.
    */
  NS_LOG_INFO ("ReceivePacket() : Node " << GetNode ()->GetId () << " : Received a packet from "
                                         << sender << " Size:" << packet->GetSize ());

  //Let's check if packet has a tag attached!
  CustomDataTag tag;
  if (packet->PeekPacketTag (tag))
    {
      NS_LOG_INFO ("\tFrom Node Id: " << tag.GetNodeId () << " at " << tag.GetPosition ()
                                      << "\tPacket Timestamp: " << tag.GetTimestamp ()
                                      << " delay=" << Now () - tag.GetTimestamp ());
    }

  return true;
}
void
BeaconSearchNet::PromiscRx (Ptr<const Packet> packet, uint16_t channelFreq, WifiTxVector tx,
                            MpduInfo mpdu, SignalNoiseDbm sn)
{
  //This is a promiscous trace. It will pick broadcast packets, and packets not directed to this node's MAC address.
  /*
        Packets received here have MAC headers and payload.
        If packets are created with 1000 bytes payload, the size here is about 38 bytes larger. 
    */
  NS_LOG_DEBUG (Now () << " PromiscRx() : Node " << GetNode ()->GetId ()
                       << " : ChannelFreq: " << channelFreq << " Mode: " << tx.GetMode ()
                       << " Signal: " << sn.signal << " Noise: " << sn.noise
                       << " Size: " << packet->GetSize () << " Mode " << tx.GetMode ());
  WifiMacHeader hdr;
  if (packet->PeekHeader (hdr))
    {
      //Let's see if this packet is intended to this node
      Mac48Address destination = hdr.GetAddr1 ();
      Mac48Address source = hdr.GetAddr2 ();

      Mac48Address myMacAddress = m_wifiDevice->GetMac ()->GetAddress ();
      //A packet is intened to me if it targets my MAC address, or it's a broadcast message
      if (destination == Mac48Address::GetBroadcast () || destination == myMacAddress)
        {
          NS_LOG_DEBUG ("\tFrom: " << source << "\n\tSeq. No. " << hdr.GetSequenceNumber ());
          //Do something for this type of packets
        }
      else //Well, this packet is not intended for me
        {
          //Maybe record some information about neighbors
        }
    }
}

} // namespace ns3
