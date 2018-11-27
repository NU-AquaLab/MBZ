#ifndef MBZ_SERVICE_H
#define MBZ_SERVICE_H

#include <pthread.h>

#define SERV_PARAM_FLOW_STATS_GET_N   50
#define SERV_PARAM_UI_PUSH_DATA_BUF_N 3072

namespace services {
  struct flow_stats {
    int nflows;
    struct {
      uint32_t srcip;
      uint32_t dstip;
      uint16_t srcport;
      uint16_t dstport;
      size_t   sent;
      size_t   recv;
      int      istcp;
    } flows[SERV_PARAM_FLOW_STATS_GET_N];
  };

  struct ui_data {
    int pid;
    int len;
    uint8_t buf[SERV_PARAM_UI_PUSH_DATA_BUF_N];
  };

  struct wifi_signal_level {
    double level;
  };

  struct wifi_disconnect {
    bool success;
  };

  class Service {
  public:
    static Service *createService(int sid,
                                  bool (*getFlowStats)(flow_stats *),
                                  bool (*pushUiData)(ui_data *),
                                  bool (*getWifiSignalLevel)(wifi_signal_level *),
                                  bool (*disconnectWifi)(wifi_disconnect *),
                                  pthread_t *t);

    virtual ~Service();

    virtual void init() = 0;
    void run();
    void stop();
    virtual void deinit() = 0;

    bool postReq(int fid, void *buf,
                 pthread_mutex_t *lock, pthread_cond_t *cv, int *status);

    pthread_t *getThread();

  protected:
    Service(int sid, int rreqfd, int wreqfd,
            pthread_t *t, int timeout_secs);

    bool processReq();
    virtual bool execReq(int fid, void *buf) = 0;

  private:
    bool m_running;
    int m_sid;
    int m_rreqfd;
    int m_wreqfd;
    pthread_t *m_thread;
    int m_timeout_secs;
  };
}


#endif //MBZ_SERVICE_H
