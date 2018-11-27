#include <arpa/inet.h>

#include "InPacket.h"
#include "common/utils.h"
#include "pcpp/IPv4Layer.h"
#include "pcpp/IPv6Layer.h"
#include "pcpp/PayloadLayer.h"
#include "pcpp/TcpLayer.h"
#include "pcpp/UdpLayer.h"

#define TAG "InPacket"

namespace routing {
  InPacket::InPacket(uint32_t srcip, uint16_t srcport,
                     uint32_t dstip, uint16_t dstport,
                     bool istcp,
                     const uint8_t *data, size_t len) : m_packet(len + 100) {
    // setup network layer
    pcpp::IPv4Layer *ipv4 = new pcpp::IPv4Layer();
    ipv4->getIPv4Header()->ipSrc = srcip;
    ipv4->getIPv4Header()->ipDst = dstip;
    ipv4->getIPv4Header()->timeToLive = 64;
    m_net = ipv4;

    // setup transport layer
    if (istcp) {
      pcpp::TcpLayer *tcp = new pcpp::TcpLayer();
      tcp->getTcpHeader()->portSrc = srcport;
      tcp->getTcpHeader()->portDst = dstport;
      m_trans = tcp;
    }
    else {
      pcpp::UdpLayer *udp = new pcpp::UdpLayer(ntohs(srcport), ntohs(dstport));
      m_trans = udp;
    }

    // create packet
    m_packet.addLayer(m_net);
    m_packet.addLayer(m_trans);
    if (data != NULL && len > 0) {
      pcpp::PayloadLayer *payload = new pcpp::PayloadLayer(data, len, true);
      m_packet.addLayer(payload);
    }

    m_packet.computeCalculateFields();
  }

  const uint8_t *InPacket::getRawData() const {
    return m_packet.getRawPacketReadOnly()->getRawData();
  }

  size_t InPacket::getRawDataLen() const {
    return (size_t) m_packet.getRawPacketReadOnly()->getRawDataLen();
  }

  void InPacket::createSynAck(uint32_t seqnum, uint32_t acknum, uint16_t win) {
    pcpp::TcpLayer *tcp = (pcpp::TcpLayer*) m_trans;

    tcp->getTcpHeader()->synFlag = 1;
    tcp->getTcpHeader()->ackFlag = 1;
    tcp->getTcpHeader()->sequenceNumber = seqnum;
    tcp->getTcpHeader()->ackNumber = acknum;
    tcp->getTcpHeader()->windowSize = win;

    m_packet.computeCalculateFields();
  }

  void InPacket::createFinAck(uint32_t seqnum, uint32_t acknum, uint16_t win) {
    pcpp::TcpLayer *tcp = (pcpp::TcpLayer*) m_trans;

    tcp->getTcpHeader()->finFlag = 1;
    tcp->getTcpHeader()->ackFlag = 1;
    tcp->getTcpHeader()->sequenceNumber = seqnum;
    tcp->getTcpHeader()->ackNumber = acknum;
    tcp->getTcpHeader()->windowSize = win;

    m_packet.computeCalculateFields();
  }

  void InPacket::createAck(uint32_t seqnum, uint32_t acknum, uint16_t win) {
    pcpp::TcpLayer *tcp = (pcpp::TcpLayer*) m_trans;

    tcp->getTcpHeader()->ackFlag = 1;
    tcp->getTcpHeader()->sequenceNumber = seqnum;
    tcp->getTcpHeader()->ackNumber = acknum;
    tcp->getTcpHeader()->windowSize = win;

    m_packet.computeCalculateFields();
  }

  void InPacket::createTcpData(uint32_t seqnum, uint32_t acknum, uint16_t win) {
    pcpp::TcpLayer *tcp = (pcpp::TcpLayer*) m_trans;

    tcp->getTcpHeader()->ackFlag = 1;
    tcp->getTcpHeader()->sequenceNumber = seqnum;
    tcp->getTcpHeader()->ackNumber = acknum;
    tcp->getTcpHeader()->windowSize = win;

    m_packet.computeCalculateFields();
  }

  void InPacket::createRst(uint32_t seqnum, uint32_t acknum, uint16_t win) {
    pcpp::TcpLayer *tcp = (pcpp::TcpLayer*) m_trans;

    tcp->getTcpHeader()->rstFlag = 1;
    tcp->getTcpHeader()->ackFlag = 1;
    tcp->getTcpHeader()->sequenceNumber = seqnum;
    tcp->getTcpHeader()->ackNumber = acknum;
    tcp->getTcpHeader()->windowSize = win;

    m_packet.computeCalculateFields();
  }

  void InPacket::print() {
    DEBUG_PRINT(TAG, "Printing constructed packet...");
    DEBUG_PRINT(TAG, "%s", m_packet.toString().c_str());
  }
}
