#include <string.h>

#include "WifiService.h"
#include "common/utils.h"

#define SERV_FUNC_WIFI_GET_SIGNAL_LEVEL 0
#define SERV_FUNC_WIFI_DISCONNECT       1

#define TIMEOUT_SECS 3

#define TAG "WifiService"

namespace services {
  struct plugin_req_get {
    double level;
  };

  struct plugin_req_disconnect {
    int success;
  };

  WifiService::WifiService(int sid, int rreqfd, int wreqfd, pthread_t *t,
                           bool (*getWifiSignalLevel)(wifi_signal_level *),
                           bool (*disconnectWifi)(wifi_disconnect *)) :
    Service(sid, rreqfd, wreqfd, t, TIMEOUT_SECS),
    m_getWifiSignalLevel(getWifiSignalLevel), m_disconnectWifi(disconnectWifi) {}

  void WifiService::init() {}

  void WifiService::deinit() {}

  bool WifiService::execReq(int fid, void *buf) {
    bool ok;

    switch (fid) {
      case SERV_FUNC_WIFI_GET_SIGNAL_LEVEL:
        ok = execReqGetSignalLevel(buf);
        break;
      case SERV_FUNC_WIFI_DISCONNECT:
        ok = execReqDisconnect(buf);
        break;
      default:
        ERROR_PRINT(TAG, "Unknown function: %d", fid);
        ok = false;
        break;
    }

    return ok;
  }

  bool WifiService::execReqGetSignalLevel(void *buf) {
    DEBUG_PRINT(TAG, "Executing request type 0 (get-signal-level)...");

    plugin_req_get *r = (plugin_req_get *) buf;

    wifi_signal_level level;
    memset(&level, 0, sizeof(level));

    if (!m_getWifiSignalLevel(&level)) {
      return false;
    }

    r->level = level.level;

    DEBUG_PRINT(TAG, "Request type 0 (get-signal-level) processed.");

    return true;
  }

  bool WifiService::execReqDisconnect(void *buf) {
    DEBUG_PRINT(TAG, "Executing request type 1 (disconnect)...");

    plugin_req_disconnect *r = (plugin_req_disconnect *) buf;

    wifi_disconnect status;
    memset(&status, 0, sizeof(status));

    if (!m_disconnectWifi(&status)) {
      return false;
    }

    r->success = status.success ? 1 : 0;

    DEBUG_PRINT(TAG, "Request type 1 (disconnect) processed.");

    return true;
  }
}
