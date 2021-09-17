#ifndef BEACON_RSU_NET_H
#define BEACON_RSU_NET_H
#include "ns3/application.h"
#include "ns3/wave-net-device.h"
#include "ns3/wifi-phy.h"
#include <map>
#include <ns3/simulator.h>

#include <bitset>
#include <bits/stdc++.h>

namespace ns3 {

class BeaconRsuNet : public ns3::Application
{
public:
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;

  BeaconRsuNet ();
  ~BeaconRsuNet ();

  void BroadcastInformation ();

  void PromiscRx (Ptr<const Packet> packet, uint16_t channelFreq, WifiTxVector tx, MpduInfo mpdu,
                  SignalNoiseDbm sn);

  bool ReceivePacket (Ptr<NetDevice> device, Ptr<const Packet> packet, uint16_t protocol,
                      const Address &sender);

  uint32_t DhcpService ();

private:
  typedef std::map<uint32_t, uint32_t> DhcpMap;

  /** \brief This is an inherited function. Code that executes once the application starts */
  void StartApplication ();

  Time m_broadcast_time; /**< How often do you broadcast messages */
  uint32_t m_packetSize; /**< Packet size in bytes */
  uint32_t m_nodeId; /**< Node's Id */

  Ptr<WifiNetDevice> m_wifiDevice; /**< wifi device */
  DhcpMap m_ipAddrUsed; /** Dhcp IP control*/
};
} // namespace ns3
#endif