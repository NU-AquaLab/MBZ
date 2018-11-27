#include <string.h>

#include "OutPacket.h"
#include "common/utils.h"
#include "pcpp/IPv4Layer.h"
#include "pcpp/IPv6Layer.h"
#include "pcpp/Layer.h"
#include "pcpp/ProtocolType.h"
#include "pcpp/TcpLayer.h"

#define TAG "OutPacket"

namespace routing {
  OutPacket::OutPacket(char *buf, size_t n, timeval *tv) {
    char *bufcpy = new char[n];
    memcpy(bufcpy, buf, n);

    // create packet
    m_rawPacket = new pcpp::RawPacket((uint8_t *) bufcpy,
                                      (int) n,
                                      *tv,
                                      true, // delete bufcpy in destructor
                                      pcpp::LINKTYPE_RAW);
    m_packet = new pcpp::Packet(m_rawPacket, pcpp::OsiModelTransportLayer);

    // extract network layer
    m_ipv4 = m_packet->getLayerOfType<pcpp::IPv4Layer>();
      OutPacket::m_ipv42 = m_packet->getLayerOfType<pcpp::IPv4Layer>();
    m_ipv6 = m_packet->getLayerOfType<pcpp::IPv6Layer>();
    if (m_ipv4 != NULL) {
      m_net = m_ipv4;
    } else if (m_ipv6 != NULL) {
      m_net = m_ipv6;
    } else {
      m_net = NULL;
    }

    // extract transport layer
    m_tcp = m_packet->getLayerOfType<pcpp::TcpLayer>();
    m_udp = m_packet->getLayerOfType<pcpp::UdpLayer>();
    if (m_tcp != NULL) {
      m_trans = m_tcp;
    } else if (m_udp != NULL) {
      m_trans = m_udp;
    } else {
      m_trans = NULL;
    }

    // ensure packet is valid
    pcpp::Layer *l = m_packet->getFirstLayer();
    m_valid = (isIpv4() || isIpv6()) &&
              (isTcp() || isUdp()) &&
              (l == m_net) &&
              (m_net->getNextLayer() == m_trans) &&
              (m_trans->getNextLayer() == NULL);

    // compute flow ID
    if (m_valid) {
      computeFlowId();
    }
  }

  OutPacket::~OutPacket() {
    delete m_packet;
    delete m_rawPacket;
  }

  bool OutPacket::isDeliverable() const {
    return m_valid;
  }

  bool OutPacket::matchesTun(uint32_t tunipv4) const {
    return isIpv4() && getSrcAddrIpv4() == tunipv4;
  }

  bool OutPacket::isIpv4() const {
    return m_ipv4 != NULL;
  }

  bool OutPacket::isIpv6() const {
    return m_ipv6 != NULL;
  }

  bool OutPacket::isTcp() const {
    return m_tcp != NULL;
  }

  bool OutPacket::isUdp() const {
    return m_udp != NULL;
  }

  bool OutPacket::isRst() const {
    return m_tcp->getTcpHeader()->rstFlag != 0;
  }

  bool OutPacket::isSyn() const {
    return m_tcp->getTcpHeader()->synFlag != 0;
  }

  bool OutPacket::isFin() const {
    return m_tcp->getTcpHeader()->finFlag != 0;
  }

  bool OutPacket::isAck() const {
    return m_tcp->getTcpHeader()->ackFlag != 0;
  }

  bool OutPacket::hasData() const {
    return m_trans->getLayerPayloadSize() > 0;
  }

  const FlowId &OutPacket::getFlowId() const {
    return m_fid;
  }

  uint32_t OutPacket::getSrcAddrIpv4() const {
    return m_ipv4->getIPv4Header()->ipSrc;
  }

  uint32_t OutPacket::getDstAddrIpv4() const {
    return m_ipv4->getIPv4Header()->ipDst;
  }

  const uint8_t *OutPacket::getSrcAddrIpv6() const {
    return m_ipv6->getIPv6Header()->ipSrc;
  }

  const uint8_t *OutPacket::getDstAddrIpv6() const {
    return m_ipv6->getIPv6Header()->ipDst;
  }

  uint16_t OutPacket::getSrcPort() const {
    if (isTcp()) {
      return m_tcp->getTcpHeader()->portSrc;
    } else {
      return m_udp->getUdpHeader()->portSrc;
    }
  }

  uint16_t OutPacket::getDstPort() const {
    if (isTcp()) {
      return m_tcp->getTcpHeader()->portDst;
    } else {
      return m_udp->getUdpHeader()->portDst;
    }
  }

  const uint8_t *OutPacket::getPayload() const {
    return m_trans->getLayerPayload();
  }

  size_t OutPacket::getPayloadLen() const {
    return m_trans->getLayerPayloadSize();
  }

  uint32_t OutPacket::getSeqNum() const {
    return m_tcp->getTcpHeader()->sequenceNumber;
  }

  uint32_t OutPacket::getAckNum() const {
    return m_tcp->getTcpHeader()->ackNumber;
  }

    pcpp::IPv4Layer* OutPacket::getIpv4Layer() const {
      return OutPacket::m_ipv42;
    }

  void OutPacket::print() const {
    DEBUG_PRINT(TAG,
                "Printing outbound packet...");

    for (pcpp::Layer *l = m_packet->getFirstLayer(); l != NULL; l = l->getNextLayer()) {
      __android_log_print(ANDROID_LOG_DEBUG, TAG,
                  "Protocol: %d | Total: %d | Header: %d | Payload: %d",
                  l->getProtocol(),
                  (int) l->getDataLen(),
                  (int) l->getHeaderLen(),
                  (int) l->getLayerPayloadSize());
      __android_log_print(ANDROID_LOG_DEBUG, TAG, "%s", l->toString().c_str());
    }
  }

  void OutPacket::computeFlowId() {
    uint16_t ipv;
    uint16_t istcp;

    uint16_t srcport;
    uint16_t dstport;

    istcp = (uint16_t) (isTcp() ? 1 : 0);
    srcport = getSrcPort();
    dstport = getDstPort();

    if (isIpv4()) {
      ipv = 4;
      uint32_t srcip;
      uint32_t dstip;

      srcip = getSrcAddrIpv4();
      dstip = getDstAddrIpv4();

      m_fid = FlowId(ipv,
                     istcp,
                     srcip,
                     dstip,
                     srcport,
                     dstport);
    } else {
      ipv = 6;
      uint8_t srcip[16];
      uint8_t dstip[16];

      memset(srcip, 0, sizeof(srcip));
      memset(dstip, 0, sizeof(dstip));

      memcpy(srcip, getSrcAddrIpv6(), sizeof(srcip));
      memcpy(dstip, getDstAddrIpv6(), sizeof(dstip));

      m_fid = FlowId(ipv,
                     istcp,
                     srcip,
                     dstip,
                     srcport,
                     dstport);
    }
  }
}
