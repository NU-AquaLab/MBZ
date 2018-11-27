#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>

#include "FlowStatsService.h"
#include "Service.h"
#include "UiService.h"
#include "WifiService.h"
#include "common/utils.h"

#define REQ_STATUS_OK      1
#define REQ_STATUS_ERR     2

#define SERV_ID_FLOW_STATS 0
#define SERV_ID_UI         1
#define SERV_ID_WIFI       2

#define TAG "Service"

namespace services {
  struct req {
    int fid;
    void *buf;
    pthread_mutex_t *lock;
    pthread_cond_t *cv;
    int *status;
  };

  Service *Service::createService(int sid,
                                  bool (*getFlowStats)(flow_stats *),
                                  bool (*pushUiData)(ui_data *),
                                  bool (*getWifiSignalLevel)(wifi_signal_level *),
                                  bool (*disconnectWifi)(wifi_disconnect *),
                                  pthread_t *t) {
    int fds[2];
    if (pipe(fds) < 0) {
      common::printSyscallError(TAG, "Unable to create pipe: %s");
      return NULL;
    }

    Service *s;
    switch (sid) {
      case SERV_ID_FLOW_STATS:
        s = new FlowStatsService(sid, fds[0], fds[1], t, getFlowStats);
        break;
      case SERV_ID_UI:
        s = new UiService(sid, fds[0], fds[1], t, pushUiData);
        break;
      case SERV_ID_WIFI:
        s = new WifiService(sid, fds[0], fds[1], t, getWifiSignalLevel, disconnectWifi);
        break;
      default:
        ERROR_PRINT(TAG, "Unknown service ID: %d", sid);
        close(fds[0]);
        close(fds[1]);
        return NULL;
    }

    return s;
  }

  Service::~Service() {
    delete m_thread;
    close(m_rreqfd);
    close(m_wreqfd);
  }

  void Service::run() {
    fd_set fds, readfds;
    int nfds = m_rreqfd + 1;
    FD_ZERO(&fds);
    FD_SET(m_rreqfd, &fds);
    FD_ZERO(&readfds);

    m_running = true;

    while (m_running) {
      readfds = fds;
      timeval tv;
      tv.tv_sec = m_timeout_secs;
      tv.tv_usec = 0;

      int rc = select(nfds, &readfds, NULL, NULL, &tv);

      if (rc < 0) {
        common::printSyscallError(TAG, "Select error: %s");
        break;
      }
      else if (rc == 0) {
        DEBUG_PRINT(TAG, "Select timed out.");
      }
      else {
        if (FD_ISSET(m_rreqfd, &readfds)) {
          processReq();
        }
      }
    }
  }

  void Service::stop() {
    m_running = false;
  }

  bool Service::postReq(int fid, void *buf,
                        pthread_mutex_t *lock, pthread_cond_t *cv, int *status) {
    req r;
    r.fid = fid;
    r.buf = buf;
    r.lock = lock;
    r.cv = cv;
    r.status = status;

    if (!common::writeAllFd(m_wreqfd, (uint8_t *) &r, sizeof(r))) {
      ERROR_PRINT(TAG, "Unable to post request.");
      return false;
    }

    DEBUG_PRINT(TAG, "Request sid=%d, fid=%d, sync=%d posted.",
                m_sid, fid, r.status ? 1 : 0);

    return true;
  }

  pthread_t *Service::getThread() {
    return m_thread;
  }

  Service::Service(int sid, int rreqfd, int wreqfd,
                   pthread_t *t, int timeout_secs):
    m_sid(sid), m_rreqfd(rreqfd), m_wreqfd(wreqfd),
    m_thread(t), m_timeout_secs(timeout_secs) {}

  bool Service::processReq() {
    req r;

    if (read(m_rreqfd, &r, sizeof(r)) < 0) {
      common::printSyscallError(TAG, "Unable to read request: %s");
      return false;
    }

    DEBUG_PRINT(TAG, "Executing request fid=%d, buf=%lx...",
                r.fid, (unsigned long) r.buf);

    bool ok = execReq(r.fid, r.buf);
    if (r.status != NULL) {
      pthread_mutex_lock(r.lock);
      *r.status = ok ? REQ_STATUS_OK : REQ_STATUS_ERR;
      pthread_cond_signal(r.cv);
      pthread_mutex_unlock(r.lock);
    }

    DEBUG_PRINT(TAG, "Request sid=%d, fid=%d, buf=%lx executed.",
                m_sid, r.fid, (unsigned long) r.buf);

    return ok;
  }
}
