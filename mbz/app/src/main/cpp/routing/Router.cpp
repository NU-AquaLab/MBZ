#include <arpa/inet.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <linux/if.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>

#include <android/log.h>

#include <algorithm>
#include <stdlib.h>
#include <sstream>
#include <android/multinetwork.h>
//#include <quic/src/net/quic_protocol.h>
//#include <testing/gtest/include/gtest/internal/gtest-internal.h>
//#include <testing/gtest/include/gtest/internal/gtest-port.h>
//#include <testing/gtest/include/gtest/gtest.h>

#include "Flow.h"
#include "FlowId.h"
#include "Plugin.h"
#include "Router.h"
#include "common/utils.h"
#include "services/FlowStatsService.h"
#include "services/Service.h"
//#include "net/quic/core/crypto/crypto_framer.h"
//#include "net/quic/core/crypto/quic_random.h"
//#include "net/quic/core/crypto/crypto_handshake.h"
//#include "net/quic/core/crypto/crypto_utils.h"
//#include "net/quic/core/crypto/null_encrypter.h"
//#include "net/quic/core/crypto/quic_decrypter.h"
//#include "net/quic/core/crypto/quic_encrypter.h"
//#include "net/quic/core/quic_data_writer.h"
//#include "net/quic/core/quic_framer.h"
//#include "net/quic/core/quic_packet_creator.h"
//#include "net/quic/core/quic_utils.h"
//#include "net/quic/platform/api/quic_endian.h"
//#include "net/quic/platform/api/quic_flags.h"
//#include "net/quic/platform/api/quic_logging.h"
//#include "net/quic/platform/api/quic_ptr_util.h"
//#include "net/quic/test_tools/crypto_test_utils.h"
//#include "net/quic/test_tools/quic_connection_peer.h"
//#include "net/quic/test_tools/quic_test_utils.h"
//#include "net/spdy/core/spdy_frame_builder.h"
//#include "net/quic/core/quic_crypto_stream.h"
//#include "net/quic/core/quic_utils.h"
//#include "net/quic/core/tls_server_handshaker.h"
//#include "net/quic/platform/api/quic_test.h"
//#include "net/quic/test_tools/crypto_test_utils.h"
//#include "net/quic/test_tools/mock_quic_dispatcher.h"
//#include "net/quic/test_tools/quic_test_utils.h"
//#include "net/quic/tools/quic_memory_cache_backend.h"
//#include "net/tools/quic/quic_simple_server_session_helper.h"
//#include "net/quic/core/quic_packets.h"

#define PLUGIN_SYM_INIT    "init"
#define PLUGIN_SYM_RUN     "run"
#define PLUGIN_SYM_STOP    "stop"
#define PLUGIN_SYM_DEINIT  "deinit"
#define PLUGIN_SYM_UPDATE  "update"
#define PLUGIN_SYM_PROCOUT "procout"
#define PLUGIN_SYM_PROCOUT_FLEX "procout_flex"
#define PLUGIN_SYM_PROCIN "procin"

#define REQ_STATUS_PENDING 0
#define REQ_STATUS_OK      1
#define REQ_STATUS_ERR     2

#define MAX_PATH 1000
#define MAX_SERVICES 20
#define MAX_UPDATE_DATA 1000

#define MTU 1500
#define TIMEOUT_SECS 1

#define TAG "Router"

#define SERV_FLOW_STATS 0
#define SERV_UI 1
#define SERV_WIFI 2
#define SERV_FLEX_PROC_OUT 3
#define SERV_PROC_IN 4

namespace routing {

  static Router *instance = NULL;

  // request types

  enum req_source {
    REQ_SOURCE_FLOW,
    REQ_SOURCE_UI,
    REQ_SOURCE_SERVICE,
    REQ_SOURCE_PLUGIN,
  };

  enum req_type_flow {
    REQ_TYPE_FLOW_DONE,
  };

  enum req_type_ui {
    REQ_TYPE_UI_START_PLUGIN,
    REQ_TYPE_UI_STOP_PLUGIN,
    REQ_TYPE_UI_UPDATE_PLUGIN,
  };

  enum req_type_service {
    REQ_TYPE_SERVICE_GET_FLOW_STATS,
    REQ_TYPE_SERVICE_PUSH_UI_DATA,
    REQ_TYPE_SERVICE_GET_WIFI_SIGNAL_LEVEL,
    REQ_TYPE_SERVICE_DISCONNECT_WIFI,
  };

  enum req_type_plugin {
    REQ_TYPE_PLUGIN_REQ,
  };

  struct req {
    int source;
    int type;
    bool sync;
    pthread_mutex_t *lock;
    pthread_cond_t *cv;
    int *status;
    void *data;
  };

  struct flow_req_done {
    FlowId fid;
  };

  struct ui_req_start {
    int pid;
    int pipeline;
    int nservices;
    int services[MAX_SERVICES];
    char libpath[MAX_PATH];
  };

  struct ui_req_stop {
    int pid;
  };

  struct ui_req_update {
    int pid;
    int n;
    char buf[MAX_UPDATE_DATA];
  };

  struct serv_req_flow_stats {
    services::flow_stats *stats;
  };

  struct serv_req_ui_data {
    services::ui_data *uidata;
  };

  struct serv_req_wifi_signal_level {
    services::wifi_signal_level *level;
  };

  struct serv_req_wifi_disconnect {
    services::wifi_disconnect *status;
  };

  struct plugin_req {
    int pid;
    int sid;
    int fid;
    void *buf;
  };

  // interface to other S/W components

  bool flowDone(const FlowId &fid) {
    flow_req_done *r = new flow_req_done;
    r->fid = fid;

    return instance->post(REQ_SOURCE_FLOW,
                          REQ_TYPE_FLOW_DONE,
                          false, r);
  }

  bool getFlowStats(services::flow_stats *stats) {
    serv_req_flow_stats *r = new serv_req_flow_stats;
    r->stats = stats;

    return instance->post(REQ_SOURCE_SERVICE,
                          REQ_TYPE_SERVICE_GET_FLOW_STATS,
                          true, r);
  }

  bool pushUiData(services::ui_data *uidata) {
    serv_req_ui_data *r = new serv_req_ui_data;
    r->uidata = uidata;

    return instance->post(REQ_SOURCE_SERVICE,
                          REQ_TYPE_SERVICE_PUSH_UI_DATA,
                          true, r);
  }

  bool getWifiSignalLevel(services::wifi_signal_level *level) {
    serv_req_wifi_signal_level *r = new serv_req_wifi_signal_level;
    r->level = level;

    return instance->post(REQ_SOURCE_SERVICE,
                          REQ_TYPE_SERVICE_GET_WIFI_SIGNAL_LEVEL,
                          true, r);
  }

  bool disconnectWifi(services::wifi_disconnect *status) {
    serv_req_wifi_disconnect *r = new serv_req_wifi_disconnect;
    r->status = status;

    return instance->post(REQ_SOURCE_SERVICE,
                          REQ_TYPE_SERVICE_DISCONNECT_WIFI,
                          true, r);
  }

  int postPluginReq(void *req, int sync) {
    plugin_req *rin = (plugin_req *) req;

    plugin_req *r = new plugin_req;
    r->pid = rin->pid;
    r->sid = rin->sid;
    r->fid = rin->fid;
    r->buf = rin->buf;

    DEBUG_PRINT(TAG, "Posting plugin request pid=%d, sid=%d, fid=%d, buf=%lx.",
                rin->pid, r->sid, rin->fid, (unsigned long) rin->buf);

    return instance->post(REQ_SOURCE_PLUGIN,
                          REQ_TYPE_PLUGIN_REQ,
                          (bool) sync, r) ? 0 : -1;
  }

  void *runFlow(void *arg) {
    DEBUG_PRINT(TAG, "Begin flow processing...");

    ((Flow *) arg)->run();

    DEBUG_PRINT(TAG, "Flow processing complete.");

    return NULL;
  }

  void *runService(void *arg) {
    DEBUG_PRINT(TAG, "Service starting...");

    services::Service *s = (services::Service *) arg;
    s->run();

    DEBUG_PRINT(TAG, "Service stopped.");

    return NULL;
  }

  void *runPlugin(void *arg) {
    DEBUG_PRINT(TAG, "Plugin running...");

    Plugin *p = (Plugin *) arg;
    p->run();

    DEBUG_PRINT(TAG, "Plugin stopped.");

    return NULL;
  }

  // interface to managed code lib

  void Router::init() {
    instance = this;
    m_stopping = false;
  }

  void Router::stop() {
    m_stopping = true;
  }




    std::string ConvertJString(JNIEnv* env, jstring str)
    {
        const jsize len = env->GetStringUTFLength(str);
        const char* strChars = env->GetStringUTFChars(str, (jboolean *)0);

        std::string Result(strChars, len);

        env->ReleaseStringUTFChars(str, strChars);

        return Result;
    }

  void Router::run(JNIEnv *jenv, jobject jrouter, int tunfd) {
    m_stopping = false;



    // init JNI interface
    if (!initJni(jenv, jrouter)) {
      ERROR_PRINT(TAG, "Failed to init JNI interface.");
      m_stopping = true;
    }

    // init tun IP address
    if (!initTunAddr()) {
      ERROR_PRINT(TAG, "Failed to determine address of tun interface.");
      m_stopping = true;
    }

    // init request pipe
    if (!initPipe()) {
      ERROR_PRINT(TAG, "Failed to create pipe.");
      m_stopping = true;
    }

    // init fd sets
    fd_set fds, readfds;
    int nfds;
    FD_ZERO(&fds);
    FD_SET(tunfd, &fds);
    FD_SET(m_rreqfd, &fds);
    FD_ZERO(&readfds);
    nfds = std::max(tunfd + 1, m_rreqfd + 1);

    // read from tun, flows
    while (!m_stopping) {
      readfds = fds;
      timeval timeout;
      timeout.tv_sec = TIMEOUT_SECS;
      timeout.tv_usec = 0;

      int rc = select(nfds, &readfds, NULL, NULL, &timeout);

      if (rc > 0) {
        for (int i = 0; i < nfds; i++) {
          if (FD_ISSET(i, &readfds)) {
            if (i == tunfd) {
              processTun(tunfd);
            } else if (i == m_rreqfd) {
              processReq(m_rreqfd);
            }
          }
        }
      } else if (rc == 0) {
//        DEBUG_PRINT(TAG, "Select timed out.");
//        m_flowTable.print();
      } else {
        common::printSyscallError(TAG, "Select error: %s");
        break;
      }
    }

    // cleanup flows
    m_flowTable.abortAll();
    m_flowTable.joinAll();
    m_flowTable.removeAll();

    // cleanup plugins
    m_pluginTable.stopAll();
    m_pluginTable.joinAll();
    m_pluginTable.removeAll();

    // cleanup services
    m_serviceTable.stopAll();
    m_serviceTable.joinAll();
    m_serviceTable.removeAll();

    // cleanup fds
    if (m_rreqfd != -1) {
      close(m_rreqfd);
    }
    if (m_wreqfd != -1) {
      close(m_wreqfd);
    }
    close(tunfd);
  }







  void Router::startPlugin(int pid, int pipeline, int *services, int n, const char *libpath) {
    ui_req_start *r = new ui_req_start;
    r->pid = pid;
    r->pipeline = pipeline;
    r->nservices = n;
    for (int i = 0; i < n; i++) {
      r->services[i] = services[i];
    }
    strncpy(r->libpath, libpath, MAX_PATH);

    post(REQ_SOURCE_UI, REQ_TYPE_UI_START_PLUGIN, false, r);
  }

  void Router::stopPlugin(int pid) {
    ui_req_stop *r = new ui_req_stop;
    r->pid = pid;

    post(REQ_SOURCE_UI, REQ_TYPE_UI_STOP_PLUGIN, false, r);
  }

  void Router::updatePlugin(int pid, char *buf, int n) {
    ui_req_update *r = new ui_req_update;
    r->pid = pid;
    r->n = n;
    memcpy(r->buf, buf, (size_t) n);

    post(REQ_SOURCE_UI, REQ_TYPE_UI_UPDATE_PLUGIN, false, r);
  }

  bool Router::post(int source, int type, bool sync, void *data) {
    req r;
    r.source = source;
    r.type = type;
    r.sync = sync;
    r.lock = NULL;
    r.cv = NULL;
    r.status = NULL;
    r.data = data;

    if (sync) {
      r.lock = new pthread_mutex_t;
      r.cv = new pthread_cond_t;
      r.status = new int;

      *r.lock = PTHREAD_MUTEX_INITIALIZER;
      *r.cv = PTHREAD_COND_INITIALIZER;
      *r.status = REQ_STATUS_PENDING;
    }

    if (!common::writeAllFd(m_wreqfd, (uint8_t *) &r, sizeof(r))) {
      ERROR_PRINT(TAG, "Unable to post flow request.");
      return false;
    }

    bool ok = true;
    if (r.sync) {
      pthread_mutex_lock(r.lock);
      while (*r.status == REQ_STATUS_PENDING) {
        pthread_cond_wait(r.cv, r.lock);
      }

      ok = (*r.status == REQ_STATUS_OK);

      pthread_mutex_destroy(r.lock);
      pthread_cond_destroy(r.cv);

      delete r.lock;
      delete r.cv;
      delete r.status;
    }

    return ok;
  }

  // private functions

  bool Router::initJni(JNIEnv *jenv, jobject jrouter) {
    return m_jni.cacheRefs(jenv, jrouter);
  }

  bool Router::initTunAddr() {
    ifreq ifr;
    ifr.ifr_addr.sa_family = AF_INET;
    strcpy(ifr.ifr_name, "tun0");

    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
      common::printSyscallError(TAG, "Unable to create socket: %s");
      return false;
    }

    if (ioctl(fd, SIOCGIFADDR, &ifr) < 0) {
      common::printSyscallError(TAG, "Unable to get tun0 address: %s");
      return false;
    }

    sockaddr_in *addr = (sockaddr_in *) &ifr.ifr_addr;
    m_tunaddripv4 = addr->sin_addr.s_addr;
    DEBUG_PRINT(TAG,
                "Detected tun0 address: %s",
                inet_ntoa(addr->sin_addr));

    close(fd);

    return true;
  }

  bool Router::initPipe() {
    m_rreqfd = -1;
    m_wreqfd = -1;

    int fds[2];
    if (pipe(fds) < 0) {
      common::printSyscallError(TAG, "Error creating request pipe: %s");
      return false;
    }

    m_rreqfd = fds[0];
    m_wreqfd = fds[1];
    return true;
  }

  bool Router::processTun(int tunfd) {
    DEBUG_PRINT(TAG, "Processing packet from tun...");

    char buf[MTU];

    int n = (int) read(tunfd, buf, MTU);
    if (n > 0) {
      DEBUG_PRINT(TAG, "Read %d bytes from tun.", n);

      timeval tv;
      gettimeofday(&tv, NULL);
      OutPacket *outpkt = new OutPacket(buf, (size_t) n, &tv);
      outpkt->print();

      if (!outpkt->isDeliverable()) {
        ERROR_PRINT(TAG, "Undeliverable packet from tun.");
        delete outpkt;
        return false;
      } else if (!outpkt->matchesTun(m_tunaddripv4)) {
        DEBUG_PRINT(TAG, "Packet from tun does not match tun address. Dropped.");
        delete outpkt;
        return true;
      } else {
        // create flow, if needed
        if (!m_flowTable.hasFlow(outpkt->getFlowId())) {
          if (!processTunNew(tunfd, outpkt)) {
            delete outpkt;
            return false;
          }
        }

        // send packet to flow
        if (!processTunPacket(outpkt)) {
          ERROR_PRINT(TAG, "Error passing packet to flow.");
          delete outpkt;
          return false;
        }//else{
//          try {
//            __android_log_print(ANDROID_LOG_DEBUG, "JTN-good", "%s, %s, %d",
//                                outpkt->getIpv4Layer()->getSrcIpAddress().toString().c_str(),
//                                outpkt->getIpv4Layer()->getDstIpAddress().toString().c_str(),
//                                outpkt->getDstPort());
//          }catch(const std::exception&){
//            __android_log_print(ANDROID_LOG_DEBUG, "JTN-bad", "error");
//          }
        //}

      }

    } else if (n == 0) {
      ERROR_PRINT(TAG, "tun fd closed.");
      return false;
    } else if (errno == EAGAIN) {
      ERROR_PRINT(TAG, "No data available from tun.");
      return false;
    } else {
      common::printSyscallError(TAG, "Error reading from tun: %s");
      return false;
    }

    DEBUG_PRINT(TAG, "Packet from tun processed.");

      return true;
  }

  bool Router::processTunNew(int tunfd, const OutPacket *outpkt) {
    DEBUG_PRINT(TAG, "Processing new flow...");

    // get time that this flow was first seen
    timeval tv;
    gettimeofday(&tv, NULL);

    // check for valid transport
    if (!outpkt->isTcp() && !outpkt->isUdp()) {
      ERROR_PRINT(TAG, "Unknown transport in tun packet.");
      return false;
    }

    // check for SYN, if TCP
    if (outpkt->isTcp() && !outpkt->isSyn()) {
      processTunReset(tunfd, outpkt);
      return false;
    }

    // get socket type
    int socktype;
    if (outpkt->isTcp()) {
      socktype = SOCK_STREAM;
    } else {
      socktype = SOCK_DGRAM;
    }

    // create socket
    int sock;
    if (outpkt->isIpv4()) {
      sock = socket(AF_INET, socktype, 0);
    } else {
      sock = socket(AF_INET6, socktype, 0);
    }
    if (sock < 0) {
      common::printSyscallError(TAG, "Unable to create socket: %s");
      return false;
    }

    // protect socket from VPN interface
    if (!m_jni.protectSocket(sock)) {
      ERROR_PRINT(TAG, "Unable to protect socket.");
      close(sock);
      return false;
    }

    // get plugins that need to be pipelined
    std::vector<Plugin*> plugins;
    m_pluginTable.getPipelinePlugins(&plugins);

    // init flow
    pthread_t *t = new pthread_t;
    Flow *f = Flow::createFlow(outpkt->getFlowId(),
                               sock, tunfd,
                               &tv, t,
                               &plugins,
                               flowDone);
    if (f == NULL) {
      ERROR_PRINT(TAG, "Unable to create flow.");
      close(sock);
      delete t;
      return false;
    }

    // start thread
    int rc = pthread_create(t, NULL, runFlow, (void *) f);
    if (rc < 0) {
      ERROR_PRINT(TAG, "Error creating thread for new flow: %d", rc);
      delete f;
      return false;
    }

    // add entry to flow table
    m_flowTable.addFlow(outpkt->getFlowId(), f);

    DEBUG_PRINT(TAG, "New flow initialized.");

    return true;
  }

  bool Router::processTunReset(int tunfd, const OutPacket *outpkt) {
    DEBUG_PRINT(TAG, "Resetting existing TCP connection...");

    // init TCP parameters
    uint32_t isn;
    uint32_t acknum;
    uint16_t win;
    srand((unsigned) time(NULL));
    isn = (unsigned) rand();
    acknum = ntohl(outpkt->getSeqNum()) + outpkt->getPayloadLen();
    win = 65535;

    // create RST packet
    InPacket *inpkt;
    if (outpkt->isIpv4()) {
      inpkt = new InPacket(outpkt->getDstAddrIpv4(),
                           outpkt->getDstPort(),
                           outpkt->getSrcAddrIpv4(),
                           outpkt->getSrcPort(),
                           outpkt->isTcp(),
                           NULL, 0);

      inpkt->createRst(htonl(isn), htonl(acknum), htons(win));
    } else {
      // TODO IPv6
      ERROR_PRINT(TAG, "TCP/IPv6 unimplemented.");
      return false;
    }
    inpkt->print();

    // inject RST packet into tun interface
    const uint8_t *raw = inpkt->getRawData();
    size_t len = inpkt->getRawDataLen();
    if (!common::writeAllFd(tunfd, raw, len)) {
      ERROR_PRINT(TAG, "Error writing RST to tun.");
      delete inpkt;
      return false;
    }
    delete inpkt;

    DEBUG_PRINT(TAG, "Existing TCP connection reset.");

    return true;
  }

  bool Router::processTunPacket(const OutPacket *outpkt) {
    DEBUG_PRINT(TAG, "Sending tun packet to flow...");

    Flow *f = m_flowTable.getFlow(outpkt->getFlowId());
    if (!f->processPacket(outpkt)) {
      ERROR_PRINT(TAG, "Unable to send packet to flow.");
      return false;
    }

    DEBUG_PRINT(TAG, "Tun packet sent to flow.");

    return true;
  }

  bool Router::processReq(int reqfd) {
    DEBUG_PRINT(TAG, "Processing request...");

    req r;
    if (read(reqfd, &r, sizeof(r)) < 0) {
      common::printSyscallError(TAG, "Error reading request: %s");
      return false;
    }

    bool ok;
    bool pending = false;
    switch (r.source) {
      case REQ_SOURCE_FLOW:
        ok = processReqFlow(r.type, r.data);
        break;
      case REQ_SOURCE_UI:
        ok = processReqUi(r.type, r.data);
        break;
      case REQ_SOURCE_SERVICE:
        ok = processReqService(r.type, r.data);
        break;
      case REQ_SOURCE_PLUGIN:
        ok = processReqPlugin(r.type, r.data, r.lock, r.cv, r.status);
        pending = ok;
        break;
      default:
        ERROR_PRINT(TAG, "Unknown request source.");
        ok = false;
        break;
    }

    if (r.sync && !pending) {
      pthread_mutex_lock(r.lock);
      *r.status = ok ? REQ_STATUS_OK : REQ_STATUS_ERR;
      pthread_cond_signal(r.cv);
      pthread_mutex_unlock(r.lock);
    }

    DEBUG_PRINT(TAG, "Request processed.");

    return ok;
  }

  bool Router::processReqFlow(int type, void *data) {
    DEBUG_PRINT(TAG, "Processing flow request...");

    flow_req_done *r;
    bool ok;

    switch (type) {
      case REQ_TYPE_FLOW_DONE:
        r = (flow_req_done *) data;
        ok = processReqFlowDone(r->fid);
        delete r;
        break;
      default:
        ERROR_PRINT(TAG, "Unknown flow request.");
        ok = false;
        break;
    }

    DEBUG_PRINT(TAG, "Flow request processed.");

    return ok;
  }

  bool Router::processReqFlowDone(const FlowId &fid) {
    DEBUG_PRINT(TAG, "Processing flow done request...");

    m_flowTable.removeFlow(fid);

    DEBUG_PRINT(TAG, "Flow done request processed.");

    return true;
  }

  bool Router::processReqUi(int type, void *data) {
    DEBUG_PRINT(TAG, "Processing UI request...");

    ui_req_start *rstart;
    ui_req_stop *rstop;
    ui_req_update *rupdate;
    bool ok;

    switch (type) {
      case REQ_TYPE_UI_START_PLUGIN:
        rstart = (ui_req_start *) data;
        ok = processReqUiStartPlugin(rstart->pid, rstart->pipeline,
                                     rstart->services, rstart->nservices,
                                     rstart->libpath);
        delete rstart;
        break;
      case REQ_TYPE_UI_STOP_PLUGIN:
        rstop = (ui_req_stop *) data;
        ok = processReqUiStopPlugin(rstop->pid);
        delete rstop;
        break;
      case REQ_TYPE_UI_UPDATE_PLUGIN:
        rupdate = (ui_req_update *) data;
        ok = processReqUiUpdatePlugin(rupdate->pid,
                                      rupdate->buf,
                                      rupdate->n);
        delete rupdate;
        break;
      default:
        ERROR_PRINT(TAG, "Unknown UI request: %d", type);
        ok = false;
        break;
    }

    DEBUG_PRINT(TAG, "UI request processed.");

    return ok;
  }

  bool Router::processReqUiStartPlugin(int pid, int pipeline,
                                       int *services, int n, const char *libpath) {
    DEBUG_PRINT(TAG, "Processing UI start plugin request...");

    // check if plugin already running
    if (m_pluginTable.pluginRunning(pid)) {
      DEBUG_PRINT(TAG, "Plugin already started.");
      return true;
    }

    // get or allocate plugin
    Plugin *p;
    if (m_pluginTable.hasPlugin(pid)) {
      p = m_pluginTable.getPlugin(pid);
    }
    else {
      p = createPlugin(pid, pipeline, services, n, libpath);
      if (p == NULL) {
        ERROR_PRINT(TAG, "Error creating plugin %d.", pid);
        return false;
      }
      p->init();
    }

    // launch plugin
    pthread_t *t = new pthread_t;
    p->setRunState(true, t);
    int rc = pthread_create(t, NULL, runPlugin, (void *) p);
    if (rc < 0) {
      ERROR_PRINT(TAG, "Error creating thread for plugin: %d", rc);
      p->setRunState(false, NULL);
    }

    // add plugin to existing pipelines
    if (p->isPipeline()) {
      std::vector<Flow*> flows;
      m_flowTable.getAllFlows(&flows);
      for (auto it = flows.begin(); it != flows.end(); it++) {
        if (!(*it)->insertIntoPipeline(p)) {
          ERROR_PRINT(TAG, "Error adding plugin %d to flow %s.",
                      pid, (*it)->getFlowId().toString().c_str());
          return false;
        }
      }
    }

    DEBUG_PRINT(TAG, "UI start plugin request processed.");

    return true;
  }


  //JTN: Reason why the plugin needs to be passed a pid when we create it,
    // it serves as an identifier to check if it's running and stop if so
  bool Router::processReqUiStopPlugin(int pid) {
    DEBUG_PRINT(TAG, "Processing UI stop plugin request...");

    // check if plugin is already stopped
    if (!m_pluginTable.pluginRunning(pid)) {
      DEBUG_PRINT(TAG, "Plugin already stopped.");
      return true;
    }

    // remove plugin from pipelines, if needed
    Plugin *p = m_pluginTable.getPlugin(pid);
    if (p->isPipeline()) {
      std::vector<Flow*> flows;
      m_flowTable.getAllFlows(&flows);
      for (auto it = flows.begin(); it != flows.end(); it++) {
        if (!(*it)->removeFromPipeline(p)) {
          ERROR_PRINT(TAG, "Error removing plugin %d from flow %s.",
                      pid, (*it)->getFlowId().toString().c_str());
        }
      }
    }

    // stop plugin
    p->stop();
    int rc = pthread_join(*p->getThread(), NULL);
    if (rc < 0) {
      ERROR_PRINT(TAG, "Error joining plugin thread: %d", rc);
    }

    // mark not running
    p->setRunState(false, NULL);

    DEBUG_PRINT(TAG, "UI stop plugin request processed.");

    return true;
  }

  bool Router::processReqUiUpdatePlugin(int pid, char *buf, int n) {
    DEBUG_PRINT(TAG, "Processing UI update plugin request...");

    if (!m_pluginTable.pluginRunning(pid)) {
      ERROR_PRINT(TAG, "Plugin %d not running.", pid);
      return false;
    }

    Plugin *p = m_pluginTable.getPlugin(pid);
    p->update(buf, n);

    DEBUG_PRINT(TAG, "UI update plugin request processed.");

    return true;
  }

  bool Router::processReqService(int type, void *data) {
    DEBUG_PRINT(TAG, "Processing service request...");

    serv_req_flow_stats *rf;
    serv_req_ui_data *ru;
    serv_req_wifi_signal_level *rws;
    serv_req_wifi_disconnect *rwd;
    bool ok;

    switch (type) {
      case REQ_TYPE_SERVICE_GET_FLOW_STATS:
        rf = (serv_req_flow_stats *) data;
        ok = processReqServiceGetFlowStats(rf->stats);
        delete rf;
        break;
      case REQ_TYPE_SERVICE_PUSH_UI_DATA:
        ru = (serv_req_ui_data *) data;
        ok = processReqServicePushUiData(ru->uidata);
        delete ru;
        break;
      case REQ_TYPE_SERVICE_GET_WIFI_SIGNAL_LEVEL:
        rws = (serv_req_wifi_signal_level *) data;
        ok = processReqServiceGetWifiSignalLevel(&rws->level->level);
        break;
      case REQ_TYPE_SERVICE_DISCONNECT_WIFI:
        rwd = (serv_req_wifi_disconnect *) data;
        ok = processReqServiceDisconnectWifi(&rwd->status->success);
        break;
      default:
        ERROR_PRINT(TAG, "Unknown service request: %d", type);
        ok = false;
        break;
    }

    DEBUG_PRINT(TAG, "Service request processed.");

    return ok;
  }

  bool Router::processReqServiceGetFlowStats(services::flow_stats *stats) {
    DEBUG_PRINT(TAG, "Processing service flow stats request...");

    std::vector<Flow*> flows;
    m_flowTable.getAllFlows(&flows);

    int i = 0;
    for (auto it = flows.begin(); it != flows.end(); it++) {
      Flow *f = (*it);
      const FlowId &fid = f->getFlowId();

      stats->flows[i].srcip   = fid.m_srcip4;
      stats->flows[i].dstip   = fid.m_dstip4;
      stats->flows[i].srcport = fid.m_srcport;
      stats->flows[i].dstport = fid.m_dstport;
      stats->flows[i].sent    = f->getSent();
      stats->flows[i].recv    = f->getRecv();
      stats->flows[i].istcp   = fid.m_istcp;

      i++;

      if (i == SERV_PARAM_FLOW_STATS_GET_N) {
        break;
      }
    }

    stats->nflows = i;

    DEBUG_PRINT(TAG, "Service flow stats request processed.");

    return true;
  }

  bool Router::processReqServicePushUiData(services::ui_data *uidata) {
    DEBUG_PRINT(TAG, "Processing service ui data request...");

    if (!m_jni.postUiData(uidata)) {
      ERROR_PRINT(TAG, "Unable to post UI data.");
      return false;
    }

    DEBUG_PRINT(TAG, "Service ui data request processed.");

    return true;
  }

  bool Router::processReqServiceGetWifiSignalLevel(double *level) {
    DEBUG_PRINT(TAG, "Processing service wifi signal level request...");

    if (!m_jni.getWifiSignalLevel(level)) {
      ERROR_PRINT(TAG, "Unable to get wifi signal level.");
      return false;
    }

    DEBUG_PRINT(TAG, "Service wifi signal level request processed.");

    return true;
  }

  bool Router::processReqServiceDisconnectWifi(bool *success) {
    DEBUG_PRINT(TAG, "Processing service disconnect wifi request...");

    *success = m_jni.disconnectWifi();

    if (!*success) {
      ERROR_PRINT(TAG, "Unable to disconnect wifi.");
      return false;
    }

    DEBUG_PRINT(TAG, "Service disconnect wifi request processed.");

    return true;
  }

  bool Router::processReqPlugin(int type, void *data,
                                pthread_mutex_t *lock, pthread_cond_t *cv, int *status) {
    DEBUG_PRINT(TAG, "Processing plugin request...");

    plugin_req *r;
    bool ok;

    switch (type) {
      case REQ_TYPE_PLUGIN_REQ:
        r = (plugin_req *) data;
        ok = processReqPluginReq(r->pid, r->sid, r->fid, r->buf,
                                 lock, cv, status);
        delete r;
        break;
      default:
        ERROR_PRINT(TAG, "Unknown plugin request: %d", type);
        ok = false;
        break;
    }

    DEBUG_PRINT(TAG, "Plugin request processed.");

    return ok;
  }

  bool Router::processReqPluginReq(int pid, int sid, int fid, void *buf,
                                   pthread_mutex_t *lock, pthread_cond_t *cv, int *status) {
    DEBUG_PRINT(TAG, "Processing plugin request %d:%d:%d.", pid, sid, fid);

    if (!m_pluginTable.hasPlugin(pid)) {
      ERROR_PRINT(TAG, "Plugin request for dead plugin: %d", pid);
      return false;
    }

    if (!m_serviceTable.hasService(sid)) {
      ERROR_PRINT(TAG, "Plugin request requires dead service: %d", sid);
      return false;
    }

    services::Service *s = m_serviceTable.getService(sid);
    if (!s->postReq(fid, buf, lock, cv, status)) {
      ERROR_PRINT(TAG, "Plugin request %d rejected by service %d", fid, sid);
      return false;
    }

    DEBUG_PRINT(TAG, "Plugin request %d:%d:%d processed.", pid, sid, fid);

    return true;
  }

  bool checkPluginServices(int *services, int service){
      for(int i = 0; i < MAX_SERVICES; i++){
          if (services[i] == service){
              return true;
          }
      }
      return false;
  }

  Plugin *Router::createPlugin(int pid, int pipeline,
                               int *services, int n, const char *libpath) {
    void *handle;
    void (*init)(int, int (*)(void*, int));
    void (*run)(void);
    void (*stop)(void);
    void (*deinit)(void);
    void (*update)(char *, int);
    void (*procout)(uint32_t, uint16_t, int *);
    void (*procout_flex)(char *, char *, int *);
    void (*procin)(char *, char *, int *);

    DEBUG_PRINT(TAG, "Creating plugin %d, pipeline=%d, nservices=%d...",
                pid, pipeline, n);

    // start required services
    if (!startServices(services, n)) {
      ERROR_PRINT(TAG, "Unable to start services requested by plugin.");
      return NULL;
    }

    // load handle for shared library
    handle = dlopen(libpath, RTLD_NOW);
    if (handle == NULL) {
      ERROR_PRINT(TAG, "Unable to load shared library %s: %s", libpath, dlerror());
      return NULL;
    }

    // load symbols
    init = (void (*)(int, int (*)(void*, int))) dlsym(handle, PLUGIN_SYM_INIT);
    if (init == NULL) {
      ERROR_PRINT(TAG, "Unable to load init symbol: %s", dlerror());
      dlclose(handle);
      return NULL;
    }

    run = (void (*)(void)) dlsym(handle, PLUGIN_SYM_RUN);
    if (run == NULL) {
      ERROR_PRINT(TAG, "Unable to load run symbol: %s", dlerror());
      dlclose(handle);
      return NULL;
    }

    stop = (void (*)(void)) dlsym(handle, PLUGIN_SYM_STOP);
    if (stop == NULL) {
      ERROR_PRINT(TAG, "Unable to load stop symbol: %s", dlerror());
      dlclose(handle);
      return NULL;
    }

    deinit = (void (*)(void)) dlsym(handle, PLUGIN_SYM_DEINIT);
    if (deinit == NULL) {
      ERROR_PRINT(TAG, "Unable to load deinit symbol: %s", dlerror());
      dlclose(handle);
      return NULL;
    }

    update = (void (*)(char *, int)) dlsym(handle, PLUGIN_SYM_UPDATE);
    if (update == NULL) {
      ERROR_PRINT(TAG, "Unable to load update symbol: %s", dlerror());
      dlclose(handle);
      return NULL;
    }

    bool flex_procout = checkPluginServices(services, SERV_FLEX_PROC_OUT);
    if(flex_procout){
        procout_flex = (void (*)(char *, char *, int *)) dlsym(handle, PLUGIN_SYM_PROCOUT_FLEX);
        if (procout_flex == NULL) {
            ERROR_PRINT(TAG, "Unable to load procout_flex symbol: %s", dlerror());
            dlclose(handle);
            return NULL;
        }
    }else {
        procout = (void (*)(uint32_t, uint16_t, int *)) dlsym(handle, PLUGIN_SYM_PROCOUT);
        if (procout == NULL) {
            ERROR_PRINT(TAG, "Unable to load procout symbol: %s", dlerror());
            dlclose(handle);
            return NULL;
        }
    }

    procin = (void (*)(char *, char *, int *)) dlsym(handle, PLUGIN_SYM_PROCIN);
    if(procin == NULL){
      ERROR_PRINT(TAG, "Unable to load procin symbol: %s", dlerror());
      dlclose(handle);
      return NULL;
    }



    // allocate plugin
      Plugin *p;
//      if(flex_procout){
//          p = new Plugin(pid, pipeline, handle, postPluginReq, init, run, stop, deinit, update, procout_flex);
//    }else
//
//      {
//          p = new Plugin(pid, pipeline, handle, postPluginReq,
//                           init, run, stop, deinit, update, procout);
//  }
    p = new Plugin(pid, pipeline, handle, postPluginReq, init, run, stop, deinit, update, procout_flex, procin);

    // add plugin to table
    m_pluginTable.addPlugin(pid, p);

    DEBUG_PRINT(TAG, "Plugin %d created.", pid);

    return p;
  }

  bool Router::startServices(int *services, int n) {
    DEBUG_PRINT(TAG, "Starting services...");

    for (int i = 0; i < n; i++) {
      if (!startService(services[i])) {
        ERROR_PRINT(TAG, "Unable to start service: %d", services[i]);
        return false;
      }
    }

    DEBUG_PRINT(TAG, "Services started.");

    return true;
  }

  bool Router::startService(int sid) {
    DEBUG_PRINT(TAG, "Starting service %d...", sid);
    //Return true if plugin wants flex out because it doesn't require an extra service to start
    if(sid == SERV_FLEX_PROC_OUT){
        return true;
    }
    // check if service already exists
    if (m_serviceTable.hasService(sid)) {
      DEBUG_PRINT(TAG, "Service %d already started.", sid);
      return true;
    }

    // allocate service
    pthread_t *t = new pthread_t;
    services::Service *s = services::Service::createService(
      sid,
      getFlowStats, pushUiData,
      getWifiSignalLevel, disconnectWifi,
      t);
    if (s == NULL) {
      ERROR_PRINT(TAG, "Unable to create service %d.", sid);
      return false;
    }
    s->init();

    // start service
    int rc = pthread_create(t, NULL, runService, s);
    if (rc < 0) {
      ERROR_PRINT(TAG, "Error creating service thread: %d", rc);
      delete s;
      return false;
    }

    // add service to table
    m_serviceTable.addService(sid, s);

    DEBUG_PRINT(TAG, "Service %d started.", sid);

    return true;
  }
}
