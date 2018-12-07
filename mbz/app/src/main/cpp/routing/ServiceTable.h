#ifndef MBZ_SERVICETABLE_H
#define MBZ_SERVICETABLE_H

#include <map>

#include "services/Service.h"

namespace routing {
  class ServiceTable {
  public:
    void addService(int sid, services::Service *s);
    services::Service *getService(int sid);
    bool hasService(int sid);

    void stopAll();
    void joinAll();
    void removeAll();

  private:
    std::map<int, services::Service*> m_serviceMap;
  };
}


#endif //MBZ_SERVICETABLE_H
