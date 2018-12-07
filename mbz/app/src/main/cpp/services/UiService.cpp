#include <string.h>

#include "UiService.h"
#include "common/utils.h"

#define SERV_FUNC_UI_PUSH_DATA 0

#define TIMEOUT_SECS 3

#define TAG "UiService"

namespace services {
  struct plugin_req_push {
    int pid;
    int len;
    uint8_t buf[SERV_PARAM_UI_PUSH_DATA_BUF_N];
  };

  UiService::UiService(int sid, int rreqfd, int wreqfd,
                       pthread_t *t,
                       bool (*pushUiData)(ui_data *)) :
    Service(sid, rreqfd, wreqfd, t, TIMEOUT_SECS),
    m_pushuidata(pushUiData) {}

  void UiService::init() {

  }

  void UiService::deinit() {

  }

  bool UiService::execReq(int fid, void *buf) {
    bool ok;

    switch (fid) {
      case SERV_FUNC_UI_PUSH_DATA:
        ok = execReqPushUiData(buf);
        break;
      default:
        ERROR_PRINT(TAG, "Unknown function: %d", fid);
        ok = false;
        break;
    }

    return ok;
  }

  bool UiService::execReqPushUiData(void *buf) {
    DEBUG_PRINT(TAG, "Executing request type 0 (push-ui-data)...");

    plugin_req_push *r = (plugin_req_push *) buf;

    DEBUG_PRINT(TAG, "Request: pid=%d, len=%d", r->pid, r->len);

    ui_data uidata;
    memset(&uidata, 0, sizeof(uidata));

    uidata.pid = r->pid;
    uidata.len = r->len;
    memcpy(uidata.buf, r->buf, (size_t) r->len);

    DEBUG_PRINT(TAG, "Request type 0 (push-ui-data) processed.");

    return m_pushuidata(&uidata);
  }
}
