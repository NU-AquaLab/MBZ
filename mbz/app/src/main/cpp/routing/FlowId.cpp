#include <arpa/inet.h>
#include <sys/socket.h>

#include <sstream>
#include <string.h>

#include "FlowId.h"

namespace routing {
  FlowId::FlowId() {
    memset(m_data, 0, sizeof(m_data));
  }

  FlowId::FlowId(const FlowId &other) {
    memcpy(m_data, other.m_data, sizeof(m_data));
  }

  FlowId::FlowId(uint16_t ipver, uint16_t istcp,
                 uint32_t srcip, uint32_t dstip,
                 uint16_t srcport, uint16_t dstport): FlowId() {
    m_ipver = ipver;
    m_istcp = istcp;
    m_srcip4 = srcip;
    m_dstip4 = dstip;
    m_srcport = srcport;
    m_dstport = dstport;
  }

  FlowId::FlowId(uint16_t ipver, uint16_t istcp,
                 uint8_t *srcip, uint8_t *dstip,
                 uint16_t srcport, uint16_t dstport): FlowId() {
    m_ipver = ipver;
    m_istcp = istcp;
    memcpy(&m_srcip6, srcip, 16);
    memcpy(&m_dstip6, dstip, 16);
    m_srcport = srcport;
    m_dstport = dstport;
  }

  bool FlowId::operator<(const FlowId &other) const {
    return cmp(other) < 0;
  }

  bool FlowId::operator>(const FlowId &other) const {
    return cmp(other) > 0;
  }

  bool FlowId::operator==(const FlowId &other) const {
    return cmp(other) == 0;
  }

  bool FlowId::operator!=(const FlowId &other) const {
    return cmp(other) != 0;
  }

  bool FlowId::operator<=(const FlowId &other) const {
    return cmp(other) <= 0;
  }

  bool FlowId::operator>=(const FlowId &other) const {
    return cmp(other) >= 0;
  }

  std::string FlowId::toString() const {
    std::stringstream ss;

    in_addr srcaddr {m_srcip4};
    in_addr dstaddr {m_dstip4};

    ss << "ipver="  << m_ipver;
    ss << " proto=" << (m_istcp ? "TCP" : "UDP");
    ss << " src="   << inet_ntoa(srcaddr) << ":" << ntohs(m_srcport);
    ss << " dst="   << inet_ntoa(dstaddr) << ":" << ntohs(m_dstport);

    return ss.str();
  }

  int FlowId::cmp(const FlowId &other) const {
    return memcmp((const char *)m_data,
                  (const char *)other.m_data,
                  sizeof(m_data));
  }
}
