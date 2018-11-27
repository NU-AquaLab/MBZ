#ifndef MBZ_PLUGIN_H
#define MBZ_PLUGIN_H

#include <pthread.h>

namespace routing {
  class Plugin {
  public:
    Plugin(int pid, int pipeline, void *handle, int (*req)(void*, int),
           void (*init)(int, int (*)(void*, int)),
           void (*run)(void),
           void (*stop)(void),
           void (*deinit)(void),
           void (*update)(char *, int),
           void (*procout)(uint32_t, uint16_t, int *));
    ~Plugin();

    void init();
    void run();
    void stop();
    void deinit();
    void update(char *buf, int n);
    void procout(uint32_t dstip, uint16_t dstport, int *drop);

    int getPid();
    bool isRunning();
    int isPipeline();
    pthread_t *getThread();
    void setRunState(bool running, pthread_t *t);

  private:
    int m_pid;
    int m_pipeline;
    bool m_running;
    void *m_handle;
    int (*m_req)(void*, int);
    void (*m_init)(int, int (*)(void*, int));
    void (*m_run)(void);
    void (*m_stop)(void);
    void (*m_deinit)(void);
    void (*m_update)(char *, int);
    void (*m_procout)(uint32_t, uint16_t, int *);
    pthread_t *m_thread;
  };
}

#endif //MBZ_PLUGIN_H
