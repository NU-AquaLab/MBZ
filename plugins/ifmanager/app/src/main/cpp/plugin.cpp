#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <android/log.h>

#include <stdint.h>

extern "C" {

#define DATA_WIFI_SIGNAL 0

#define FUNCTION_LT 0
#define FUNCTION_EQ 1
#define FUNCTION_GT 2

#define ACTION_DISCONNECT_WIFI 0

#define SERV_ID_WIFI              2
#define SERV_FUNC_WIFI_GET_SIGNAL 0
#define SERV_FUNC_WIFI_DISCONNECT 1

#define TAG "ifmanager"

#define MAX_RULES 100

struct req {
  int pid;
  int sid;
  int fid;
  void *buf;
};

struct req_wifi_signal {
  double level;
};

struct req_wifi_disconnect {
  int success;
};

struct rule {
  int data;
  int function;
  double value;
  int action;
};

static struct {
  int pid;
  int (*req)(void*, int);
  bool running;

  int nrules;
  rule rules[MAX_RULES];

  pthread_mutex_t rule_lock;
} state;

void init(int pid, int (*req)(void*, int)) {
  __android_log_print(ANDROID_LOG_DEBUG, TAG, "init");

  memset(&state, 0, sizeof(state));

  state.pid = pid;
  state.req = req;
  state.running = false;
  state.nrules = 0;

  state.rule_lock = PTHREAD_MUTEX_INITIALIZER;
}

static int get_val_wifi_signal(double *val) {
  req r;

  memset(&r, 0, sizeof(r));

  __android_log_print(ANDROID_LOG_DEBUG, TAG, "getting wifi signal...");

  req_wifi_signal signal;
  memset(&signal, 0, sizeof(signal));

  r.pid = state.pid;
  r.sid = SERV_ID_WIFI;
  r.fid = SERV_FUNC_WIFI_GET_SIGNAL;
  r.buf = &signal;

  if (state.req(&r, 1)) {
    __android_log_print(ANDROID_LOG_ERROR, TAG, "error getting wifi signal");
    return -1;
  }

  __android_log_print(ANDROID_LOG_DEBUG, TAG, "wifi signal = %f", signal.level);

  *val = signal.level;

  if (*val < 0) {
    return -1;
  }

  return 0;
}

static int get_val(const rule *r, double *val) {
  int rc;

  switch (r->data) {
    case DATA_WIFI_SIGNAL:
      rc = get_val_wifi_signal(val);
      break;
    default:
      __android_log_print(ANDROID_LOG_ERROR, TAG, "unknown data type %d",
                          r->data);
      rc = -1;
      break;
  }

  return rc;
}

static int check_trigger(const rule *r, double actual, int *trigger) {
  int rc = 0;

  switch (r->function) {
    case FUNCTION_LT:
      *trigger = actual < r->value;
      break;
    case FUNCTION_EQ:
      *trigger = actual == r->value; // do something more useful
      break;
    case FUNCTION_GT:
      *trigger = actual > r->value;
      break;
    default:
      __android_log_print(ANDROID_LOG_ERROR, TAG, "unknown function %d",
                          r->function);
      rc = -1;
      *trigger = 0;
      break;
  }

  return rc;
}

static int exec_action_disconnect_wifi() {
  req r;

  memset(&r, 0, sizeof(r));

  __android_log_print(ANDROID_LOG_DEBUG, TAG, "disconnecting wifi...");

  req_wifi_disconnect disconnect;
  memset(&disconnect, 0, sizeof(disconnect));

  r.pid = state.pid;
  r.sid = SERV_ID_WIFI;
  r.fid = SERV_FUNC_WIFI_DISCONNECT;
  r.buf = &disconnect;

  if (state.req(&r, 1)) {
    __android_log_print(ANDROID_LOG_ERROR, TAG, "error disconnecting wifi");
    return -1;
  }

  __android_log_print(ANDROID_LOG_DEBUG, TAG, "success = %d", disconnect.success);

  return 0;
}

static int exec_action(const rule *r) {
  int rc;

  switch (r->action) {
    case ACTION_DISCONNECT_WIFI:
      rc = exec_action_disconnect_wifi();
      break;
    default:
      __android_log_print(ANDROID_LOG_ERROR, TAG, "unknown action %d",
                          r->action);
      rc = -1;
      break;
  }

  return rc;
}

void run() {
  __android_log_print(ANDROID_LOG_DEBUG, TAG, "run");

  state.running = true;

  while (state.running) {
    for (int i = 0; i < state.nrules; i++) {
      __android_log_print(ANDROID_LOG_DEBUG, TAG, "running rule %d...", i);

      // get snapshot of rule
      // rule may be invalidated in update(), but coarser lock could deadlock
      rule r;
      pthread_mutex_lock(&state.rule_lock);
      r = state.rules[i];
      pthread_mutex_unlock(&state.rule_lock);

      double val;
      if (get_val(&r, &val)) {
        __android_log_print(ANDROID_LOG_ERROR, TAG,
                            "error getting value for rule %d", i);
        continue;
      }

      int trigger = 0;
      if (check_trigger(&r, val, &trigger)) {
        __android_log_print(ANDROID_LOG_ERROR, TAG,
                            "error evaluating trigger for rule %d", i);
        continue;
      }

      __android_log_print(ANDROID_LOG_DEBUG, TAG, "rule %d triggered? %s",
                          i, trigger ? "yes" : "no");

      if (trigger) {
        if (exec_action(&r)) {
          __android_log_print(ANDROID_LOG_ERROR, TAG,
                              "error executing action for rule %d", i);
          continue;
        }
      }

      __android_log_print(ANDROID_LOG_DEBUG, TAG, "ran rule %d", i);
    }

    sleep(5);
  }

  __android_log_print(ANDROID_LOG_DEBUG, TAG, "run complete");
}

void stop() {
  __android_log_print(ANDROID_LOG_DEBUG, TAG, "stop");

  state.running = false;
}

void deinit() {
  __android_log_print(ANDROID_LOG_DEBUG, TAG, "deinit");

  pthread_mutex_destroy(&state.rule_lock);

  memset(&state, 0, sizeof(state));
}

void update(char *buf, int n) {
  __android_log_print(ANDROID_LOG_DEBUG, TAG, "update");

  if (!state.running) {
    __android_log_print(ANDROID_LOG_ERROR, TAG, "called while not running");
    return;
  }

  pthread_mutex_lock(&state.rule_lock);

  state.nrules = 0;

  for (int i = 0, j = 0; i < n; i += 20, j++) {
    int data = *((int *) &buf[i]);
    int function = *((int *) &buf[i+4]);
    double value = *((double *) &buf[i+8]);
    int action = *((int *) &buf[i+16]);

    state.rules[j].data = data;
    state.rules[j].function = function;
    state.rules[j].value = value;
    state.rules[j].action = action;

    __android_log_print(ANDROID_LOG_DEBUG, TAG, "got rule: %d:%d:%f:%d",
                        data, function, value, action);
  }

  state.nrules = n/20;

  pthread_mutex_unlock(&state.rule_lock);

  __android_log_print(ANDROID_LOG_DEBUG, TAG, "got %d rules", state.nrules);
}

void procout(uint32_t dstip, uint16_t dstport, int *drop) {
  __android_log_print(ANDROID_LOG_DEBUG, TAG, "procout");

  *drop = 0;
}

}
