#ifndef BEACON_SEARCH_NET_H
#define BEACON_SEARCH_NET_H
#include "ns3/application.h"
#include "ns3/wave-net-device.h"
#include "ns3/wifi-phy.h"
#include <vector>

namespace ns3 {

class BeaconSearchNet : public ns3::Application
{
public:
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;

  BeaconSearchNet ();
  ~BeaconSearchNet ();

  /** \brief Broadcast some information */
  void BroadcastInformation ();

  void PromiscRx (Ptr<const Packet> packet, uint16_t channelFreq, WifiTxVector tx, MpduInfo mpdu,
                  SignalNoiseDbm sn);

  /** 
   * \brief This function is called when a net device receives a packet. 
   * It will be connected to the callback in StartApplication. 
   * This matches the signiture of NetDevice receive.
   */
  bool ReceivePacket (Ptr<NetDevice> device, Ptr<const Packet> packet, uint16_t protocol,
                      const Address &sender);

private:
  /** \brief This is an inherited function. Code that executes once the application starts */
  void StartApplication ();
  Time m_broadcast_time; /**< How often do you broadcast messages */
  uint32_t m_packetSize; /**< Packet size in bytes */
  Ptr<WifiNetDevice> m_wifiDevice; /**< A WaveNetDevice that is attached to this device */
};
} // namespace ns3
#endif