#include <android/log.h>

#include <string.h>

#include "UdpFlow.h"
#include "common/utils.h"

#define TAG (m_fid.toString().c_str())
#define TIMEOUT_SECS 20

namespace routing {
  UdpFlow::UdpFlow(const FlowId &fid,
                   int sockfd, int tunfd,
                   timeval *tv, pthread_t *t,
                   std::vector<Plugin *> *plugins,
                   bool (*done)(const FlowId &),
                   int rreqfd, int wreqfd):
    Flow(fid, sockfd, tunfd,
         tv, t, plugins, done,
         rreqfd, wreqfd, TIMEOUT_SECS) {}

  bool UdpFlow::prepareSock(const OutPacket *outpkt) {
    DEBUG_PRINT(TAG, "Preparing UDP socket...");

    // setup remote address
    sockaddr *addr;
    sockaddr_in ipv4addr;
    if (outpkt->isIpv4()) {
      memset(&ipv4addr, 0, sizeof(ipv4addr));
      ipv4addr.sin_family = AF_INET;
      ipv4addr.sin_addr.s_addr = outpkt->getDstAddrIpv4();
      ipv4addr.sin_port = outpkt->getDstPort();

      addr = (sockaddr*) &ipv4addr;
    }
    else {
      ERROR_PRINT(TAG, "UDP/IPv6 unimplemented.");
      return false;
    }

    // associate with remote host
    if (connect(m_sockfd, addr, sizeof(sockaddr)) < 0) {
      common::printSyscallError(TAG, "Unable to associate UDP socket with remote: %s");
      return false;
    }

    DEBUG_PRINT(TAG, "UDP socket prepared.");

    return true;
  }

  bool UdpFlow::processTunPacket(const OutPacket *outpkt, bool *) {
    DEBUG_PRINT(TAG, "Processing outbound UDP packet...");

    // open a socket, if needed
    if (!m_sock_ready) {
      if (!prepareSock(outpkt)) {
        return false;
      }
      m_sock_ready = true;
    }

    // pass payload to socket and update flow info
    if (!writeSock(outpkt)) {
      ERROR_PRINT(TAG, "Error writing outbound UDP to socket.");
      return false;
    }
    dataSent(outpkt->getPayloadLen());

    DEBUG_PRINT(TAG, "Outbound UDP packet processed.");

    return true;
  }

  bool UdpFlow::processSockReset(bool *done) {
    *done = true;

    DEBUG_PRINT(TAG, "UDP socket reset processed.");

    return true;
  }

  bool UdpFlow::processSockClose(bool *done) {
    *done = true;

    DEBUG_PRINT(TAG, "UDP socket close processed.");

    return true;
  }

  bool UdpFlow::processSockData(uint8_t *buf, size_t len) {
    DEBUG_PRINT(TAG, "Processing inbound UDP packet...");

    // create incoming packet
    InPacket *inpkt = generatePacket(buf, len);
    if (inpkt == NULL) {
      ERROR_PRINT(TAG, "Error creating inbound UDP packet.");
      return false;
    }
    inpkt->print();

    // prepare message back to router
    writeTun(inpkt);

    // update flow info
    dataRecv(len);

    DEBUG_PRINT(TAG, "Inbound UDP packet processed.");

    return true;
  }

  void UdpFlow::dataSent(size_t n) {
    if (m_sent == 0) {
      gettimeofday(&m_tfirstbyte, NULL);
    }
    m_sent += n;
    updateTime();
  }

  void UdpFlow::dataRecv(size_t n) {
    m_recv += n;
    updateTime();
  }
}
