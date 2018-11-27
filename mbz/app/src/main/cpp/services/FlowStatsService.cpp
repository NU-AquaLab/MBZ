#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>

#include "FlowStatsService.h"
#include "common/utils.h"
#include "Service.h"

#define SERV_FUN_FLOW_STATS_GET 0

#define TIMEOUT_SECS 3

#define TAG "FlowStatsService"

namespace services {
  struct plugin_req_get {
    int nflows;
    struct {
      int uid;
      char srcip[20];
      char dstip[20];
      int srcport;
      int dstport;
      int istcp;
    } flows[SERV_PARAM_FLOW_STATS_GET_N];
  };

  FlowStatsService::FlowStatsService(int sid,
                                     int rreqfd, int wreqfd,
                                     pthread_t *t,
                                     bool (*getFlowStats)(flow_stats *)):
    Service(sid, rreqfd, wreqfd, t, TIMEOUT_SECS),
    m_getflowstats(getFlowStats) {}

  void FlowStatsService::init() {

  }

  void FlowStatsService::deinit() {

  }

  bool FlowStatsService::execReq(int fid, void *buf) {
    bool ok;

    switch (fid) {
      case SERV_FUN_FLOW_STATS_GET:
        ok = execReqGetFlowStats(buf);
        break;
      default:
        ERROR_PRINT(TAG, "Unknown function: %d", fid);
        ok = false;
        break;
    }

    return ok;
  }

  bool FlowStatsService::execReqGetFlowStats(void *buf) {
    DEBUG_PRINT(TAG, "Executing request type 0 (get-flow-stats)...");

    plugin_req_get *r = (plugin_req_get *) buf;

    flow_stats stats;
    memset(&stats, 0, sizeof(stats));

    if (!m_getflowstats(&stats)) {
      return false;
    }

    r->nflows = stats.nflows;

    for (int i = 0; i < stats.nflows; i++) {
      in_addr srcip, dstip;
      srcip.s_addr = stats.flows[i].srcip;
      dstip.s_addr = stats.flows[i].dstip;

      r->flows[i].uid = -1;
      strcpy(r->flows[i].srcip, inet_ntoa(srcip));
      strcpy(r->flows[i].dstip, inet_ntoa(dstip));
      r->flows[i].srcport = ntohs(stats.flows[i].srcport);
      r->flows[i].dstport = ntohs(stats.flows[i].dstport);
      r->flows[i].istcp = stats.flows[i].istcp;
    }

    DEBUG_PRINT(TAG, "Result: nflows=%d", r->nflows);

    DEBUG_PRINT(TAG, "Request type 0 (get-flow-stats) executed.");

    return true;
  }
}
