#ifndef MBZ_TCPFLOW_H
#define MBZ_TCPFLOW_H

#include "Flow.h"
#include "FlowId.h"

namespace routing {
  enum TcpState {
    CLOSED      = 0,
    SYN_SENT    = 1,
    ESTABLISHED = 2,
    FIN_WAIT_1  = 3,
    TIME_WAIT   = 4,
    CLOSE_WAIT  = 5,
    LAST_ACK    = 6
  };

  class TcpFlow : public Flow {
  public:
    TcpFlow(const FlowId &fid,
            int sockfd, int tunfd,
            timeval *tv, pthread_t *t,
            std::vector<Plugin *> *plugins,
            bool (*done)(const FlowId &),
            int rreqfd, int wreqfd);
    virtual ~TcpFlow() {}

    virtual std::string toString() const;

  protected:
    virtual bool prepareSock(const OutPacket *outpkt);
    virtual bool processTunPacket(const OutPacket *outpkt, bool *done);
    virtual bool processSockReset(bool *done);
    virtual bool processSockClose(bool *done);
    virtual bool processSockData(uint8_t *buf, size_t len);

  private:

    bool processTunRst(bool *done);
    bool processTunSyn(const OutPacket *outpkt);
    bool processTunAck(const OutPacket *outpkt, bool *done);
    bool processTunFin(const OutPacket *outpkt, bool *done);
    bool processTunFinActive();
    bool processTunFinPassive(const OutPacket *outpkt, bool *done);
    bool processTunData(const OutPacket *outpkt);

    void synSent(uint32_t seqnum_out);
    void synAckRecv(uint32_t seqnum_in, uint32_t acknum_in, uint16_t win_in);
    void ackSent(uint32_t acknum_out);
    void finSent(uint32_t seqnum_out);
    void finAckRecv(uint32_t seqnum_in, uint32_t acknum_in);
    void ackRecv(uint32_t acknum_in);
    void dataSent(uint32_t seqnum_out, size_t len);
    void dataRecv(uint32_t seqnum_in, size_t len, uint32_t acknum_in);

    const char *describeState(TcpState state) const;

    TcpState m_state;
    uint32_t m_seqnum_out;
    uint32_t m_acknum_out;
    uint32_t m_seqnum_in;
    uint32_t m_acknum_in;
    uint16_t m_win_in;
  };
}

#endif //MBZ_TCPFLOW_H
