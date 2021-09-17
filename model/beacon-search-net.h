#ifndef BEACON_SEARCH_NET_H
#define BEACON_SEARCH_NET_H
#include "ns3/application.h"
#include "ns3/wave-net-device.h"
#include "ns3/wifi-phy.h"
#include "custom-data-tag.h"
#include <vector>

namespace ns3 {

class BeaconSearchNet : public ns3::Application
{

  struct BEACONRECEIVED
  { 
    uint32_t rsuId;
    uint32_t ipAddr;
    uint32_t mask;
    ns3::Time timestamp;
    double signal;
    double noise;
  };

public:
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;

  std::vector<BEACONRECEIVED> beaconsReceived;

  BeaconSearchNet ();
  ~BeaconSearchNet ();

  void CheckHandoverProcess ();

  uint32_t HandoverStrategy (); /**< Handover Strategy */

  void PromiscRx (Ptr<const Packet> packet, uint16_t channelFreq, WifiTxVector tx, MpduInfo mpdu,
                  SignalNoiseDbm sn);

  bool ReceivePacket (Ptr<NetDevice> device, Ptr<const Packet> packet, uint16_t protocol,
                      const Address &sender);

private:
  /** \brief This is an inherited function. Code that executes once the application starts */
  void StartApplication ();

  Time m_broadcast_time; /**< How often do you broadcast messages */
  uint32_t m_packetSize; /**< Packet size in bytes */
  uint32_t m_nodeId; /**< Node's Id */
  uint32_t m_rsuConnected; /**< Stores which RSU the node is connected to */

  Ptr<WifiNetDevice> m_wifiDevice; /**< wifi device */
};
} // namespace ns3
#endif