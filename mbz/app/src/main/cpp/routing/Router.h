#ifndef MBZ_ROUTER_H
#define MBZ_ROUTER_H

#include <jni.h>

#include "FlowTable.h"
#include "InPacket.h"
#include "Jni.h"
#include "OutPacket.h"
#include "PluginTable.h"
#include "ServiceTable.h"
#include "services/FlowStatsService.h"
#include "services/Service.h"
#include "services/UiService.h"

namespace routing {
  bool flowDone(const FlowId &fid);
  bool getFlowStats(services::flow_stats *stats);
  bool pushUiData(services::ui_data *uidata);
  bool getWifiSignalLevel(services::wifi_signal_level *level);
  bool disconnectWifi(services::wifi_disconnect *status);
  int postPluginReq(void *req, int sync);

  void *runFlow(void *arg);
  void *runService(void *arg);
  void *runPlugin(void *arg);

  class Router {
  public:
    void init();
    void run(JNIEnv *jenv, jobject jrouter, int tunfd);
    void stop();
    void startPlugin(int pid, int pipeline, int *services, int len, const char *libpath);
    void stopPlugin(int pid);
    void updatePlugin(int pid, char *buf, int n);

    bool post(int source, int type, bool sync, void *data);

  private:
    bool m_stopping;
    uint32_t m_tunaddripv4;
    FlowTable m_flowTable;
    PluginTable m_pluginTable;
    ServiceTable m_serviceTable;
    Jni m_jni;
    int m_wreqfd;
    int m_rreqfd;

    bool initJni(JNIEnv *jenv, jobject jrouter);
    bool initTunAddr();
    bool initPipe();

    bool processTun(int tunfd);
    bool processTunNew(int tunfd, const OutPacket *outpkt);
    bool processTunReset(int tunfd, const OutPacket *outpkt);
    bool processTunPacket(const OutPacket *outpkt);

    bool processReq(int reqfd);
    bool processReqFlow(int type, void *data);
    bool processReqFlowDone(const FlowId &fid);
    bool processReqUi(int type, void *data);
    bool processReqUiStartPlugin(int pid, int pipeline,
                                 int *services, int n, const char *libpath);
    bool processReqUiStopPlugin(int pid);
    bool processReqUiUpdatePlugin(int pid, char *buf, int n);
    bool processReqService(int type, void *data);
    bool processReqServiceGetFlowStats(services::flow_stats *stats);
    bool processReqServicePushUiData(services::ui_data *uidata);
    bool processReqServiceGetWifiSignalLevel(double *level);
    bool processReqServiceDisconnectWifi(bool *success);
    bool processReqPlugin(int type, void *data,
                          pthread_mutex_t *lock, pthread_cond_t *cv, int *status);
    bool processReqPluginReq(int pid, int sid, int fid, void *buf,
                             pthread_mutex_t *lock, pthread_cond_t *cv, int *status);

    Plugin *createPlugin(int pid, int pipeline,
                         int *services, int n, const char *libpath);
    bool startServices(int *services, int n);
    bool startService(int sid);
  };
}

#endif //MBZ_ROUTER_H
