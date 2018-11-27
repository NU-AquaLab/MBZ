#include <dlfcn.h>

#include "Plugin.h"

namespace routing {
  Plugin::Plugin(int pid, int pipeline, void *handle, int (*req)(void*, int),
                 void (*init)(int, int (*)(void*, int)),
                 void (*run)(void),
                 void (*stop)(void),
                 void (*deinit)(void),
                 void (*update)(char *, int),
                 void (*procout)(uint32_t, uint16_t, int *)):
    m_pid(pid), m_pipeline(pipeline), m_running(false),
    m_handle(handle), m_req(req),
    m_init(init), m_run(run), m_stop(stop), m_deinit(deinit),
    m_update(update), m_procout(procout),
    m_thread(NULL) {}

  Plugin::~Plugin() {
    dlclose(m_handle);
    delete m_thread;
  }

  void Plugin::init() {
    m_init(m_pid, m_req);
  }

  void Plugin::run() {
    m_run();
  }

  void Plugin::stop() {
    m_stop();
  }

  void Plugin::deinit() {
    m_deinit();
  }

  void Plugin::update(char *buf, int n) {
    m_update(buf, n);
  }

  void Plugin::procout(uint32_t dstip, uint16_t dstport, int *drop) {
    m_procout(dstip, dstport, drop);
  }

  int Plugin::getPid() {
    return m_pid;
  }

  bool Plugin::isRunning() {
    return m_running;
  }

  int Plugin::isPipeline() {
    return m_pipeline;
  }

  pthread_t *Plugin::getThread() {
    return m_thread;
  }

  void Plugin::setRunState(bool running, pthread_t *t) {
    m_running = running;
    delete m_thread;
    m_thread = t;
  }
}
