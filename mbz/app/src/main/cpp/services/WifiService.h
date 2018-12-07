#ifndef MBZ_WIFISERVICE_H
#define MBZ_WIFISERVICE_H

#include "Service.h"

namespace services {
  class WifiService : public Service {
  public:
    WifiService(int sid, int rreqfd, int wreqfd,
                pthread_t *t,
                bool (*getWifiSignalLevel)(wifi_signal_level *),
                bool (*disconnectWifi)(wifi_disconnect *));

    virtual void init();
    virtual void deinit();

  protected:
    virtual bool execReq(int fid, void *buf);
    bool execReqGetSignalLevel(void *buf);
    bool execReqDisconnect(void *buf);

  private:
    bool (*m_getWifiSignalLevel)(wifi_signal_level *);
    bool (*m_disconnectWifi)(wifi_disconnect *);
  };
}

#endif //MBZ_WIFISERVICE_H
