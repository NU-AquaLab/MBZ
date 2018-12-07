#ifndef MBZ_FLOWSTATSSERVICE_H
#define MBZ_FLOWSTATSSERVICE_H

#include <stdint.h>

#include "Service.h"

namespace services {
  class FlowStatsService : public Service {
  public:
    FlowStatsService(int sid, int rreqfd, int wreqfd,
                     pthread_t *t,
                     bool (*getFlowStats)(flow_stats *));

    virtual void init();
    virtual void deinit();

  protected:
    virtual bool execReq(int fid, void *buf);
    virtual bool execReqGetFlowStats(void *buf);

  private:
    bool (*m_getflowstats)(flow_stats *);
  };
}


#endif //MBZ_FLOWSTATSSERVICE_H
