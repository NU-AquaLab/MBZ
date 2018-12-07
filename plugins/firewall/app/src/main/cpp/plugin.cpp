#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <android/log.h>

#include <stdint.h>

extern "C" {

#define TAG "firewall"

#define MAX_ROUTES 100

static struct {
  int pid;
  int (*req)(void*, int);
  bool running;

  int nroutes;
  struct {
    uint32_t addr;
    uint32_t prefix;
  } routes[MAX_ROUTES];

  pthread_mutex_t cv_lock;
  pthread_cond_t cv;
  pthread_mutex_t route_lock;
} state;

void init(int pid, int (*req)(void*, int)) {
  __android_log_print(ANDROID_LOG_DEBUG, TAG, "init");

  memset(&state, 0, sizeof(state));

  state.pid = pid;
  state.req = req;
  state.running = false;

  state.nroutes = 0;

  state.cv_lock = PTHREAD_MUTEX_INITIALIZER;
  state.cv = PTHREAD_COND_INITIALIZER;
  state.route_lock = PTHREAD_MUTEX_INITIALIZER;
}

void run() {
  __android_log_print(ANDROID_LOG_DEBUG, TAG, "run");

  state.running = true;

  pthread_mutex_lock(&state.cv_lock);
  while (state.running) {
    pthread_cond_wait(&state.cv, &state.cv_lock);
  }

  pthread_mutex_unlock(&state.cv_lock);

  __android_log_print(ANDROID_LOG_DEBUG, TAG, "run complete");
}

void stop() {
  __android_log_print(ANDROID_LOG_DEBUG, TAG, "stop");

  pthread_mutex_lock(&state.cv_lock);
  state.running = false;
  pthread_cond_signal(&state.cv);
  pthread_mutex_unlock(&state.cv_lock);

  __android_log_print(ANDROID_LOG_DEBUG, TAG, "stop complete");
}

void deinit() {
  __android_log_print(ANDROID_LOG_DEBUG, TAG, "deinit");

  pthread_mutex_destroy(&state.cv_lock);
  pthread_cond_destroy(&state.cv);
  pthread_mutex_destroy(&state.route_lock);

  memset(&state, 0, sizeof(state));
}

void update(char *buf, int n) {
  __android_log_print(ANDROID_LOG_DEBUG, TAG, "update");

  if (!state.running) {
    _android_log_print(ANDROID_LOG_ERROR, TAG, "called while not nunning");
    return;
  }

  pthread_mutex_lock(&state.route_lock);

  state.nroutes = 0;

  for (int i = 0, j = 0; i < n; i += 20, j++) {
    struct in_addr addr;
    const char *s = (const char *) &buf[i];
    if (!inet_aton(s, &addr)) {
      __android_log_print(ANDROID_LOG_ERROR, TAG, "bad addr: %s", s);
      state.routes[j].addr = 0;
    }
    else {
      state.routes[j].addr = addr.s_addr;
    }

    uint32_t prefix = (uint32_t) buf[i+16];
    if (prefix > 32) {
      __android_log_print(ANDROID_LOG_ERROR, TAG, "bad prefix: %s", s);
      state.routes[j].prefix = 32;
    }
    else {
      state.routes[j].prefix = prefix;
    }

    __android_log_print(ANDROID_LOG_DEBUG, TAG, "got route: %s/%d", s, prefix);
  }

  state.nroutes = n/20;

  pthread_mutex_unlock(&state.route_lock);

  __android_log_print(ANDROID_LOG_DEBUG, TAG, "got %d routes", state.nroutes);
}

void procout(uint32_t dstip, uint16_t dstport, int *drop) {
  __android_log_print(ANDROID_LOG_DEBUG, TAG, "procout");

  *drop = 0;

  if (!state.running) {
    __android_log_print(ANDROID_LOG_ERROR, TAG, "called while not running");
    return;
  }

  pthread_mutex_lock(&state.route_lock);

  for (int i = 0; i < state.nroutes; i++) {
    uint32_t dstpre = ntohl(dstip) &
      (uint32_t) (0xffffffffu << (32 - state.routes[i].prefix));
    uint32_t routepre = ntohl(state.routes[i].addr) &
      (uint32_t) (0xffffffffu << (32 - state.routes[i].prefix));

    __android_log_print(ANDROID_LOG_DEBUG, TAG, "dstpre = %x, routepre = %x",
                        dstpre, routepre);

    if (dstpre == routepre) {
      *drop = 1;
      break;
    }
  }

  pthread_mutex_unlock(&state.route_lock);

  in_addr addr;
  addr.s_addr = dstip;
  __android_log_print(ANDROID_LOG_DEBUG, TAG, "drop %s:%d? %s",
                      inet_ntoa(addr), ntohs(dstport), *drop ? "yes" : "no");
}

}

