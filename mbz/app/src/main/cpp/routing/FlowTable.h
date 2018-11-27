#ifndef MBZ_FLOWTABLE_H
#define MBZ_FLOWTABLE_H

#include <pthread.h>

#include <map>
#include <vector>

#include "Flow.h"
#include "FlowId.h"

namespace routing {
  class FlowTable {
  public:
    bool hasFlow(const FlowId &fid) const;
    void addFlow(const FlowId &fid, Flow *f);
    void removeFlow(const FlowId &fid);
    Flow *getFlow(const FlowId &fid) const;
    void getAllFlows(std::vector<Flow*> *flows);

    void abortAll();
    void joinAll();
    void removeAll();

    void print();

  private:
    std::map<FlowId, Flow*> m_flowMap;
  };
}


#endif //MBZ_FLOWTABLE_H
