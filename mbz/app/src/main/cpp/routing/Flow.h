#ifndef MBZ_FLOW_H
#define MBZ_FLOW_H

#include <sys/time.h>

#include <queue>
#include <set>
#include <vector>

#include "FlowId.h"
#include "InPacket.h"
#include "OutPacket.h"
#include "Plugin.h"

namespace routing {
  class Flow {
  public:
    static Flow *createFlow(const FlowId &fid,
                            int sockfd, int tunfd,
                            timeval *tv, pthread_t *t,
                            std::vector<Plugin *> *plugins,
                            bool (*done)(const FlowId &));

    virtual ~Flow();

    void run();
    bool processPacket(const OutPacket *outpkt);
    bool insertIntoPipeline(Plugin *p);
    bool removeFromPipeline(Plugin *p);
    bool abort();
    const FlowId &getFlowId() const;
    pthread_t *getThread() const;
    size_t getSent() const;
    size_t getRecv() const;

    virtual std::string toString() const;

  protected:
    Flow(const FlowId &fid,
         int sockfd, int tunfd,
         timeval *tv, pthread_t *t,
         std::vector<Plugin *> *plugins,
         bool (*done)(const FlowId &),
         int rreqfd, int wreqfd,
         int timeout_secs);

    virtual bool prepareSock(const OutPacket *outpkt) = 0;
    virtual bool processTunPacket(const OutPacket *outpkt, bool *done) = 0;
    virtual bool processSockReset(bool *done) = 0;
    virtual bool processSockClose(bool *done) = 0;
    virtual bool processSockData(uint8_t *buf, size_t len) = 0;

    void updateTime();
    bool sockOpen() const;
    bool closeSock();
    InPacket *generatePacket(uint8_t *buf, size_t len);
    bool writeTun(const InPacket *inpkt);
    bool writeSock(const OutPacket *outpkt);

    FlowId m_fid;
    int m_sockfd;
    int m_tunfd;
    pthread_t *m_thread;
    std::set<Plugin *> m_plugins;
    bool (*m_done)(const FlowId &);
    int m_timeout_secs;
    timeval m_timeout;

    int m_rreqfd;
    int m_wreqfd;

    bool m_sock_ready;
    bool m_remote_closed;
    bool m_remote_reset;

    size_t m_sent;
    size_t m_recv;
    timeval m_tseen;
    timeval m_tcreate;
    timeval m_tfirstbyte;
    timeval m_tlast;

  private:
    bool sockReadable() const;
    timeval *getTimeout();
    bool processRouterReq(bool *done);
    bool applyPluginsOut(OutPacket *outpkt, int *drop);
    bool addPlugin(Plugin *p);
    bool removePlugin(Plugin *p);
    bool processSockData(bool *done);
    bool postReq(int type, void *data);
  };
}

#endif //MBZ_FLOW_H
