#include <arpa/inet.h>
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>

#include <android/log.h>

#include "FlowTable.h"
#include "common/utils.h"

#define TAG "FlowTable"

namespace routing {
  bool FlowTable::hasFlow(const FlowId &fid) const {
    return m_flowMap.find(fid) != m_flowMap.end();
  }

  void FlowTable::addFlow(const FlowId &fid, Flow *f) {
    m_flowMap[fid] = f;
  }

  void FlowTable::removeFlow(const FlowId &fid) {
    auto it = m_flowMap.find(fid);

    if (it != m_flowMap.end()) {
      Flow *f = it->second;
      it->second = NULL;
      m_flowMap.erase(it);

      delete f;
    }
  }

  Flow *FlowTable::getFlow(const FlowId &fid) const {
    return m_flowMap.find(fid)->second;
  }

  void FlowTable::getAllFlows(std::vector<Flow *> *flows) {
    for (auto it = m_flowMap.begin(); it != m_flowMap.end(); it++) {
      flows->push_back(it->second);
    }
  }

  void FlowTable::abortAll() {
    for (auto it = m_flowMap.begin(); it != m_flowMap.end(); it++) {
      it->second->abort();
    }
  }

  void FlowTable::joinAll() {
    for (auto it = m_flowMap.begin(); it != m_flowMap.end(); it++) {
      int rc = pthread_join(*it->second->getThread(), NULL);
      if (rc < 0) {
        ERROR_PRINT(TAG, "Error joining flow thread: %d", rc);
      }
    }
  }

  void FlowTable::removeAll() {
    for (auto it = m_flowMap.begin(); it != m_flowMap.end(); it++) {
      Flow *f = it->second;
      it->second = NULL;
      delete f;
    }

    m_flowMap.clear();
  }

  void FlowTable::print() {
    DEBUG_PRINT(TAG, "Flow count: %u", (unsigned) m_flowMap.size());

    for (auto it = m_flowMap.begin(); it != m_flowMap.end(); it++) {
      DEBUG_PRINT(TAG,
                  "%s -> %s",
                  it->first.toString().c_str(),
                  it->second->toString().c_str());
    }
  }
}
