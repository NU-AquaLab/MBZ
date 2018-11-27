#include <string.h>
#include <unistd.h>

#include <android/log.h>

#include <stdio.h>

extern "C" {

#define SERV_ID_FLOW_STATS      0
#define SERV_FUN_FLOW_STATS_GET 0

#define SERV_ID_UI              1
#define SERV_FUNC_UI_PUSH_DATA  0

#define FLOW_STATS_GET_N 500
#define UI_PUSH_DATA_N   3072

#define TAG "snitch"

struct req {
  int pid;
  int sid;
  int fid;
  void *buf;
};

struct req_flow_stats_get {
  int nflows;
  struct {
    int uid;
    char srcip[20];
    char dstip[20];
    int srcport;
    int dstport;
    int istcp;
  } flows[FLOW_STATS_GET_N];
};

struct req_ui_push_data {
  int pid;
  int len;
  uint8_t buf[UI_PUSH_DATA_N];
};

static struct {
  int pid;
  int (*req)(void*, int);
  bool running;
} state;

void init(int pid, int (*req)(void*, int)) {
  __android_log_print(ANDROID_LOG_DEBUG, TAG, "init");

  state.pid = pid;
  state.req = req;
  state.running = false;
}

void run() {
  __android_log_print(ANDROID_LOG_DEBUG, TAG, "run");

  state.running = true;

  while (state.running) {
    req r;
    memset(&r, 0, sizeof(r));

    __android_log_print(ANDROID_LOG_DEBUG, TAG, "getting flow stats...");

    req_flow_stats_get stats;
    memset(&stats, 0, sizeof(stats));

    r.pid = state.pid;
    r.sid = SERV_ID_FLOW_STATS;
    r.fid = SERV_FUN_FLOW_STATS_GET;
    r.buf = &stats;

    if (state.req(&r, 1)) {
      __android_log_print(ANDROID_LOG_ERROR, TAG, "error getting flow stats");
      sleep(5);
      continue;
    }

    __android_log_print(ANDROID_LOG_DEBUG, TAG, "pushing ui data...");

    req_ui_push_data uidata;
    memset(&uidata, 0, sizeof(uidata));
    uidata.pid = state.pid;
    uidata.len = stats.nflows * 56;
    for (int i = 0; i < stats.nflows; i++) {
      int j = i*56;

      *((int *) &uidata.buf[j]) = stats.flows[i].uid;

      char *src = (char *) &uidata.buf[j+4];
      sprintf(src, "%s:%d", stats.flows[i].srcip, stats.flows[i].srcport);

      char *dst = (char *) &uidata.buf[j+28];
      sprintf(dst, "%s:%d", stats.flows[i].dstip, stats.flows[i].dstport);

      char *proto = (char *) &uidata.buf[j+52];
      sprintf(proto, "%s", stats.flows[i].istcp ? "TCP" : "UDP");

      char summary [100];
      sprintf(summary, "%s, %s, %s", src, dst, proto);
      __android_log_print(ANDROID_LOG_DEBUG, "JTN-TEST", "%s", summary);
    }

    r.pid = state.pid;
    r.sid = SERV_ID_UI;
    r.fid = SERV_FUNC_UI_PUSH_DATA;
    r.buf= &uidata;

    if (state.req(&r, 1)) {
      __android_log_print(ANDROID_LOG_ERROR, TAG, "error pushing ui data");
      sleep(5);
      continue;
    }

    sleep(5);
  }
}

void stop() {
  __android_log_print(ANDROID_LOG_DEBUG, TAG, "stop");

  state.running = false;
}

void deinit() {
  __android_log_print(ANDROID_LOG_DEBUG, TAG, "deinit");
}

void update(char *buf, int n) {
  __android_log_print(ANDROID_LOG_DEBUG, TAG, "update");
}

void procout(uint32_t dstip, uint16_t dstport, int *drop) {
  __android_log_print(ANDROID_LOG_DEBUG, TAG, "procout");

  *drop = 0;
}

}
