#ifndef MBZ_FLOWID_H
#define MBZ_FLOWID_H

#include <string>
#include <stdint.h>

namespace routing {
  union FlowId {
  public:
    FlowId();
    FlowId(const FlowId &other);
    FlowId(uint16_t ipver, uint16_t istcp,
           uint32_t srcip, uint32_t dstip,
           uint16_t srcport, uint16_t dstport);
    FlowId(uint16_t ipver, uint16_t istcp,
           uint8_t *srcip, uint8_t *dstip,
           uint16_t srcport, uint16_t dstport);

    bool operator<(const FlowId &other)  const;
    bool operator>(const FlowId &other)  const;
    bool operator==(const FlowId &other) const;
    bool operator!=(const FlowId &other) const;
    bool operator>=(const FlowId &other) const;
    bool operator<=(const FlowId &other) const;

    std::string toString() const;

    uint8_t m_data[40];
    struct {
      uint16_t m_ipver;
      uint16_t m_istcp;
      union {
        struct {
          uint32_t m_srcip4;
          uint8_t  m_pad1[12];
        };
        uint8_t m_srcip6[16];
      };
      union {
        struct {
          uint32_t m_dstip4;
          uint8_t  m_pad2[12];
        };
        uint8_t m_dstip6[16];
      };
      uint16_t m_srcport;
      uint16_t m_dstport;
    };

  private:
    int cmp(const FlowId &other) const;
  };
}


#endif //MBZ_FLOWID_H
