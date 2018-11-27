#ifndef MBZ_INPACKET_H
#define MBZ_INPACKET_H

#include <stdint.h>

#include "pcpp/Layer.h"
#include "pcpp/Packet.h"

namespace routing {
  class InPacket {
  public:
    InPacket(uint32_t srcip, uint16_t srcport,
             uint32_t dstip, uint16_t dstport,
             bool istcp,
             const uint8_t *data, size_t len);

    const uint8_t *getRawData() const;
    size_t getRawDataLen() const;

    void createSynAck(uint32_t seqnum, uint32_t acknum, uint16_t win);
    void createFinAck(uint32_t seqnum, uint32_t acknum, uint16_t win);
    void createAck(uint32_t seqnum, uint32_t acknum, uint16_t win);
    void createTcpData(uint32_t seqnum, uint32_t acknum, uint16_t win);
    void createRst(uint32_t seqnum, uint32_t acknum, uint16_t win);

    void print();

  private:
    pcpp::Packet m_packet;
    pcpp::Layer *m_net;
    pcpp::Layer *m_trans;
  };
}

#endif //MBZ_INPACKET_H
