#include "ServiceTable.h"
#include "common/utils.h"

#define TAG "ServiceTable"

namespace routing {
  void ServiceTable::addService(int sid, services::Service *s) {
    m_serviceMap[sid] = s;
  }

  services::Service *ServiceTable::getService(int sid) {
    return m_serviceMap.find(sid)->second;
  }

  bool ServiceTable::hasService(int sid) {
    return m_serviceMap.find(sid) != m_serviceMap.end();
  }

  void ServiceTable::stopAll() {
    for (auto it = m_serviceMap.begin(); it != m_serviceMap.end(); it++) {
      it->second->stop();
    }
  }

  void ServiceTable::joinAll() {
    for (auto it = m_serviceMap.begin(); it != m_serviceMap.end(); it++) {
      int rc = pthread_join(*it->second->getThread(), NULL);
      if (rc < 0) {
        ERROR_PRINT(TAG, "Error joining service thread: %d", rc);
      }
    }
  }

  void ServiceTable::removeAll() {
    for (auto it = m_serviceMap.begin(); it != m_serviceMap.end(); it++) {
      services::Service *s = it->second;
      s->deinit();
      it->second = NULL;
      delete s;
    }

    m_serviceMap.clear();
  }
}
