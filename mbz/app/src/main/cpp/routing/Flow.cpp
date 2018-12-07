#include <errno.h>
#include <unistd.h>

#include <android/log.h>

#include <sstream>
//#include <opencl-c.h>

#include "Flow.h"
#include "TcpFlow.h"
#include "UdpFlow.h"
#include "common/utils.h"

#define MTU 1500
#define TAG (m_fid.toString().c_str())

namespace routing {
  enum req_type {
    REQ_TYPE_PACKET,
    REQ_TYPE_ADD_PLUGIN,
    REQ_TYPE_REMOVE_PLUGIN,
    REQ_TYPE_ABORT,
  };

  struct req {
    int type;
    void *data;
  };

  Flow *Flow::createFlow(const FlowId &fid,
                         int sockfd, int tunfd,
                         timeval *tv, pthread_t *t,
                         std::vector<Plugin *> *plugins,
                         bool (*done)(const FlowId &)) {
    int fds[2];
    if (pipe(fds) < 0) {
      common::printSyscallError("flows", "Unable to create pipe: %s");
      return NULL;
    }

    Flow *f;

    if (fid.m_istcp) {
      f = new TcpFlow(fid, sockfd, tunfd,
                      tv, t, plugins, done,
                      fds[0], fds[1]);
    } else {
      f = new UdpFlow(fid, sockfd, tunfd,
                      tv, t, plugins, done,
                      fds[0], fds[1]);
    }

    return f;
  }

  Flow::~Flow() {
    closeSock();
    close(m_rreqfd);
    close(m_wreqfd);
    delete m_thread;
  }

  void Flow::run() {
    // init fds
    fd_set fds;
    int nfds;

    // process input until done
    bool done = false;
    while (!done) {
      FD_ZERO(&fds);
      FD_SET(m_rreqfd, &fds);
      nfds = m_rreqfd + 1;

      if (sockReadable()) {
        FD_SET(m_sockfd, &fds);
        nfds = std::max(nfds, m_sockfd + 1);
      }

      int rc = select(nfds, &fds, NULL, NULL, getTimeout());

      if (rc > 0) {
        if (FD_ISSET(m_rreqfd, &fds)) {
          processRouterReq(&done);
        } else if (FD_ISSET(m_sockfd, &fds)) {
          processSockData(&done);
        } else {
          ERROR_PRINT(TAG, "Unknown fd selected during flow processing.");
          done = true;
        }
      } else if (rc == 0) {
        DEBUG_PRINT(TAG, "Flow select timed out. Terminating flow.");
        done = true;
      } else {
        common::printSyscallError(TAG, "Flow select error: %s");
        done = true;
      }
    }

    // notify router that we're done
    if (!m_done(m_fid)) {
      ERROR_PRINT(TAG, "Error sending done message to router.");
    }
  }

  bool Flow::processPacket(const OutPacket *outpkt) {
    return postReq(REQ_TYPE_PACKET, (void *) outpkt);
  }

  bool Flow::insertIntoPipeline(Plugin *p) {
    return postReq(REQ_TYPE_ADD_PLUGIN, (void *) p);
  }

  bool Flow::removeFromPipeline(Plugin *p) {
    return postReq(REQ_TYPE_REMOVE_PLUGIN, (void *) p);
  }

  bool Flow::abort() {
    return postReq(REQ_TYPE_ABORT, NULL);
  }

  const FlowId &Flow::getFlowId() const {
    return m_fid;
  }

  pthread_t *Flow::getThread() const {
    return m_thread;
  }

  size_t Flow::getSent() const {
    return m_sent;
  }

  size_t Flow::getRecv() const {
    return m_recv;
  }

  std::string Flow::toString() const {
    std::stringstream ss;
    timeval tv;

    ss << "sock=" << m_sockfd;
    ss << " sent="      << m_sent;
    ss << " recv="      << m_recv;
    ss << " seen="      << m_tseen.tv_sec << ":" << m_tseen.tv_usec;
    ss << " created="   << m_tcreate.tv_sec << ":" << m_tcreate.tv_usec;
    ss << " firstbyte=" << m_tfirstbyte.tv_sec << ":" << m_tfirstbyte.tv_usec;
    ss << " last="      << m_tlast.tv_sec << ":" << m_tlast.tv_usec;

    timersub(&m_tcreate, &m_tseen, &tv);
    ss << " lat to create=" << tv.tv_sec << ":" << tv.tv_usec;

    timersub(&m_tfirstbyte, &m_tseen, &tv);
    ss << " lat to first byte=" << tv.tv_sec << ":" << tv.tv_usec;

    timersub(&m_tlast, &m_tcreate, &tv);
    double t = ((double) tv.tv_sec) + tv.tv_usec / 100000.0;
    double tput_up = m_sent / t;
    double tput_down = m_recv / t;
    ss << " tput up="   << tput_up << " bytes/s";
    ss << " tput down=" << tput_down << " bytes/s";

    return ss.str();
  }

  Flow::Flow(const FlowId &fid,
             int sockfd, int tunfd,
             timeval *tv, pthread_t *t,
             std::vector<Plugin *> *plugins,
             bool (*done)(const FlowId &),
             int rreqfd, int wreqfd,
             int timeout_secs) :
    m_fid(fid), m_sockfd(sockfd), m_tunfd(tunfd),
    m_thread(t), m_plugins(), m_done(done),
    m_timeout_secs(timeout_secs),
    m_rreqfd(rreqfd), m_wreqfd(wreqfd),
    m_sock_ready(false), m_remote_closed(false), m_remote_reset(false),
    m_sent(0), m_recv(0) {

    for (auto it = plugins->begin(); it != plugins->end(); it++) {
      m_plugins.insert(*it);
    }

    m_tseen = *tv;
    gettimeofday(&m_tcreate, NULL);
    memset(&m_tfirstbyte, 0, sizeof(timeval));
    gettimeofday(&m_tlast, NULL);
  }

  void Flow::updateTime() {
    gettimeofday(&m_tlast, NULL);
  }

  bool Flow::sockOpen() const {
    return m_sockfd != -1;
  }

  bool Flow::closeSock() {
    if (sockOpen()) {
      if (close(m_sockfd) < 0) {
        common::printSyscallError(TAG, "Error closing socket: %s");
        return false;
      }

      m_sockfd = -1;
    }

    return true;
  }

  InPacket *Flow::generatePacket(uint8_t *buf, size_t len) {
    InPacket *inpkt;
    if (m_fid.m_ipver == 4) {
      inpkt = new InPacket(m_fid.m_dstip4,
                           m_fid.m_dstport,
                           m_fid.m_srcip4,
                           m_fid.m_srcport,
                           m_fid.m_istcp != 0,
                           buf, len);
    } else {
      // TODO IPv6
      ERROR_PRINT(TAG, "IPv6 unimplemented.");
      return NULL;
    }

    return inpkt;
  }

  bool Flow::writeTun(const InPacket *inpkt) {
    // write packet to tun
    const uint8_t *raw = inpkt->getRawData();
    size_t len = inpkt->getRawDataLen();
    if (!common::writeAllFd(m_tunfd, raw, len)) {
      ERROR_PRINT(TAG, "Error writing packet to tun.");
      delete inpkt;
      return false;
    }
    delete inpkt;

    return true;
  }

  bool Flow::writeSock(const OutPacket *outpkt) {
    const uint8_t *payload = outpkt->getPayload();
    size_t len = outpkt->getPayloadLen();
    if (len > 0 && !common::writeAllFd(m_sockfd, payload, len)) {
      ERROR_PRINT(TAG,
                  "Error writing outbound packet to socket.");
      return false;
    }

    return true;
  }

  bool Flow::sockReadable() const {
    return sockOpen() && m_sock_ready && !m_remote_closed && !m_remote_reset;
  }

  timeval *Flow::getTimeout() {
    m_timeout.tv_sec = m_timeout_secs;
    m_timeout.tv_usec = 0;
    return &m_timeout;
  }

  bool Flow::processRouterReq(bool *done) {
    DEBUG_PRINT(TAG, "Processing router request...");

    req r;
    OutPacket *outpkt;
    Plugin *p;
    int action = 0;

    if (read(m_rreqfd, &r, sizeof(r)) < 0) {
      common::printSyscallError(TAG, "Error reading router message: %s");
      *done = true;
      return false;
    }

    switch (r.type) {
      case REQ_TYPE_PACKET:
        outpkt = (OutPacket *) r.data;
        applyPluginsOut(outpkt, &action);
        if (!action) {
          processTunPacket(outpkt, done);
        }
        delete outpkt;
        break;
      case REQ_TYPE_ADD_PLUGIN:
        p = (Plugin *) r.data;
        addPlugin(p);
        break;
      case REQ_TYPE_REMOVE_PLUGIN:
        p = (Plugin *) r.data;
        removePlugin(p);
        break;
      case REQ_TYPE_ABORT:
        *done = true;
        break;
      default:
        ERROR_PRINT(TAG, "Unknown router request.");
        *done = true;
        break;
    }

    DEBUG_PRINT(TAG, "Router request processed.");

    return true;
  }

  bool Flow::applyPluginsOut(OutPacket *outpkt, int *action) {
    DEBUG_PRINT(TAG, "Applying plugins to outbound packet...");
    char buf[256];
//    std::string x = outpkt->toString();
    sprintf(buf, "%s\n", outpkt->toString().c_str());
    char resp;
    for (auto it = m_plugins.begin(); it != m_plugins.end(); it++) {
      DEBUG_PRINT(TAG, "Applying plugin %d.", (*it)->getPid());
      int wp = (*it)->getWhichProcout();
      if(wp == 0) {
        (*it)->procout(outpkt->getDstAddrIpv4(), outpkt->getDstPort(), action);
      }else if(wp == 1){
        (*it)->procout_flex(buf, &resp, action);
      }

      //TODO: add additional functionality to function here
      //      - i.e. bypass outward sockets and return packet in resp
      if (*action == 1) {
        break;
      }else if(*action == 2){
        //store resp from plugin to write to tun

      }
    }

    DEBUG_PRINT(TAG, "Plugins applied to outbound packet.");

    return true;
  }

  bool Flow::processSockData(bool *done) {
    DEBUG_PRINT(TAG, "Received data from socket.");

    // read socket data
    uint8_t buf[MTU];
    int n = (int) read(m_sockfd, buf, sizeof(buf));

    // handle socket data
    if (n < 0 && errno == ECONNRESET) {
      processSockReset(done);
    } else if (n < 0) {
      common::printSyscallError(TAG, "Error reading socket data: %s");
      *done = true;
    } else if (n == 0) {
      processSockClose(done);
    } else {
      InPacket ip = *generatePacket(buf, (size_t) n);
      int action = 0;
      __android_log_print(ANDROID_LOG_DEBUG, TAG, "Calling applyPluginsIn");
      applyPluginsIn(&ip, &action);
      if (action == 0) {
        processSockData(buf, (size_t) n);
      }else{
        *done = true;
      }
    }

    return true;
  }

  bool Flow::applyPluginsIn(InPacket *inPacket, int *action){
      char buf[256];
      char resp;
      sprintf(buf, "%s", inPacket->getMPacket().toString().c_str());
      for(auto it = m_plugins.begin(); it != m_plugins.end(); it++){
        //call plugin's procin function
        (*it)->procin(buf, &resp, action);
      }
    *action = 0;
    return true;
  }

  bool Flow::addPlugin(Plugin *p) {
    m_plugins.insert(p);

    DEBUG_PRINT(TAG, "Added plugin %d.", p->getPid());

    return true;
  }

  bool Flow::removePlugin(Plugin *p) {
    auto it = m_plugins.find(p);
    if (it != m_plugins.end()) {
      m_plugins.erase(it);
    }

    DEBUG_PRINT(TAG, "Removed plugin %d.", p->getPid());

    return true;
  }

  bool Flow::postReq(int type, void *data) {
    req r;
    r.type = type;
    r.data = data;

    if (!common::writeAllFd(m_wreqfd, (uint8_t *) &r, sizeof(r))) {
      ERROR_PRINT(TAG, "Error posting to work queue.");
      return false;
    }

    return true;
  }
}
