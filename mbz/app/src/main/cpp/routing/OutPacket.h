#ifndef MBZ_OUTPACKET_H
#define MBZ_OUTPACKET_H

#include <netinet/in.h>
#include <sys/time.h>

#include "FlowId.h"
#include "pcpp/IPv4Layer.h"
#include "pcpp/IPv6Layer.h"
#include "pcpp/RawPacket.h"
#include "pcpp/Packet.h"
#include "pcpp/ProtocolType.h"
#include "pcpp/UdpLayer.h"
#include "pcpp/TcpLayer.h"

namespace routing {
  class   OutPacket {
  public:
    OutPacket(char *buf, size_t n, timeval *tv);
    ~OutPacket();

    bool isDeliverable() const;
    bool matchesTun(uint32_t tunipv4) const;

    bool isIpv4() const;
    bool isIpv6() const;
    bool isTcp() const;
    bool isUdp() const;

    bool isRst() const;
    bool isSyn() const;
    bool isFin() const;
    bool isAck() const;
    bool hasData() const;

    const FlowId &getFlowId() const;
    uint32_t getSrcAddrIpv4() const;
    uint32_t getDstAddrIpv4() const;
    const uint8_t *getSrcAddrIpv6() const;
    const uint8_t *getDstAddrIpv6() const;
    uint16_t getSrcPort() const;
    uint16_t getDstPort() const;
    const uint8_t *getPayload() const;
    size_t getPayloadLen() const;
    uint32_t getSeqNum() const;
    uint32_t getAckNum() const;
    pcpp::IPv4Layer* getIpv4Layer() const;
    void print() const;
       pcpp::IPv4Layer *m_ipv42;
      std::string toString() const;


  private:
    pcpp::RawPacket *m_rawPacket;
    pcpp::Packet *m_packet;
    pcpp::Layer *m_net;

    pcpp::IPv6Layer *m_ipv6;
    pcpp::Layer *m_trans;
    pcpp::TcpLayer *m_tcp;
    pcpp::UdpLayer *m_udp;

    bool m_valid;
    FlowId m_fid;
      pcpp::IPv4Layer *m_ipv4;

    void computeFlowId();

  };
}

#endif //MBZ_OUTPACKET_H
