#ifndef MBZ_UDPFLOW_H
#define MBZ_UDPFLOW_H

#include <sys/time.h>

#include "Flow.h"
#include "FlowId.h"

namespace routing {
  class UdpFlow : public Flow {
  public:
    UdpFlow(const FlowId &fid,
            int sockfd, int tunfd,
            timeval *tv, pthread_t *t,
            std::vector<Plugin *> *plugins,
            bool (*done)(const FlowId &),
            int rreqfd, int wreqfd);
    virtual ~UdpFlow() {}

  protected:
    virtual bool prepareSock(const OutPacket *outpkt);
    virtual bool processTunPacket(const OutPacket *outpkt, bool *done);
    virtual bool processSockReset(bool *done);
    virtual bool processSockClose(bool *done);
    virtual bool processSockData(uint8_t *buf, size_t len);

  private:
    void dataSent(size_t n);
    void dataRecv(size_t n);
  };
}


#endif //MBZ_UDPFLOW_H
