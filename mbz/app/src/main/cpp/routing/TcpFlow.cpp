#include <android/log.h>

#include <sstream>
#include <stdlib.h>
#include <string.h>

#include "TcpFlow.h"
#include "common/utils.h"

#define TAG (m_fid.toString().c_str())
#define TIMEOUT_SECS 600

namespace routing {
  TcpFlow::TcpFlow(const FlowId &fid,
                   int sockfd, int tunfd,
                   timeval *tv, pthread_t *t,
                   std::vector<Plugin *> *plugins,
                   bool (*done)(const FlowId &),
                   int rreqfd, int wreqfd) :
    Flow(fid, sockfd, tunfd,
         tv, t, plugins, done,
         rreqfd, wreqfd, TIMEOUT_SECS),
    m_state(CLOSED),
    m_seqnum_out(0), m_acknum_out(0),
    m_seqnum_in(0), m_acknum_in(0), m_win_in(0) {}

  bool TcpFlow::prepareSock(const OutPacket *outpkt) {
    DEBUG_PRINT(TAG, "Preparing TCP socket...");

    // setup remote address
    sockaddr *addr;
    sockaddr_in ipv4addr;
    if (outpkt->isIpv4()) {
      memset(&ipv4addr, 0, sizeof(ipv4addr));
      ipv4addr.sin_family = AF_INET;
      ipv4addr.sin_addr.s_addr = outpkt->getDstAddrIpv4();
      ipv4addr.sin_port = outpkt->getDstPort();

      addr = (sockaddr *) &ipv4addr;
    } else {
      // TODO IPv6
      ERROR_PRINT(TAG, "TCP/IPv6 unimplemented.");
      return false;
    }

    // connect to remote host
    if (connect(m_sockfd, addr, sizeof(sockaddr)) < 0) {
      common::printSyscallError(TAG, "Error connecting to TCP remote host: %s");
      return false;
    }

    DEBUG_PRINT(TAG, "TCP socket prepared.");

    return true;
  }

  bool TcpFlow::processTunPacket(const OutPacket *outpkt, bool *done) {
    DEBUG_PRINT(TAG, "Processing outbound TCP packet...");

    // open a socket, if needed
    if (!m_sock_ready) {
      if (!prepareSock(outpkt)) {
        return false;
      }
      m_sock_ready = true;
    }

    // process TCP RST
    if (outpkt->isRst()) {
      processTunRst(done);
      return false;
    }

    // process TCP SYN
    if (outpkt->isSyn()) {
      if (!processTunSyn(outpkt)) {
        return false;
      }
    }

    // process TCP data
    if (outpkt->hasData()) {
      if (!processTunData(outpkt)) {
        return false;
      }
    }

    // process TCP ACK
    if (outpkt->isAck()) {
      if (!processTunAck(outpkt, done)) {
        return false;
      }
    }

    // process TCP FIN
    if (outpkt->isFin()) {
      if (!processTunFin(outpkt, done)) {
        return false;
      }
    }

    DEBUG_PRINT(TAG, "Outbound TCP packet processed.");

    return true;
  }

  bool TcpFlow::processSockReset(bool *done) {
    DEBUG_PRINT(TAG, "Processing remote TCP reset...");

    m_remote_reset = true;

    // init TCP parameters
    uint32_t seqnum = m_seqnum_in;
    uint32_t acknum = m_acknum_in;
    uint16_t win = m_win_in;

    // create RST packet
    InPacket *inpkt = generatePacket(NULL, 0);
    if (inpkt == NULL) {
      ERROR_PRINT(TAG, "Error creating inbound RST packet.");
      return false;
    }
    inpkt->createRst(seqnum, acknum, win);
    inpkt->print();

    // write packet to tun
    if (!writeTun(inpkt)) {
      ERROR_PRINT(TAG, "Error writing inbound RST packet.");
      return false;
    }

    // signal done
    *done = true;

    DEBUG_PRINT(TAG, "Remote TCP reset processed.");

    return true;
  }

  bool TcpFlow::processSockClose(bool *) {
    DEBUG_PRINT(TAG, "Processing remote TCP close...");

    m_remote_closed = true;

    // already closed?
    if (m_state == TIME_WAIT ||
        m_state == CLOSE_WAIT ||
        m_state == LAST_ACK) {
      DEBUG_PRINT(TAG, "Remote TCP close dropped.");
      return true;
    }

    // init TCP parameters
    uint32_t seqnum = m_seqnum_in;
    uint32_t acknum = m_seqnum_out;
    uint16_t win = m_win_in;

    // create FIN ACK packet
    InPacket *inpkt = generatePacket(NULL, 0);
    if (inpkt == NULL) {
      ERROR_PRINT(TAG, "Error creating inbound FIN ACK packet.");
      return false;
    }
    inpkt->createFinAck(htonl(seqnum), htonl(acknum), htons(win));
    inpkt->print();

    // write packet to tun
    if (!writeTun(inpkt)) {
      ERROR_PRINT(TAG, "Error writing inbound FIN ACK packet.");
      return false;
    }

    // update flow info
    finAckRecv(seqnum, acknum);

    DEBUG_PRINT(TAG, "Remote TCP close processed.");

    return true;
  }

  bool TcpFlow::processSockData(uint8_t *buf, size_t len) {
    DEBUG_PRINT(TAG, "Processing inbound TCP data...");

    // init TCP parameters
    uint32_t seqnum = m_seqnum_in;
    uint32_t acknum = m_acknum_in;
    uint16_t win = m_win_in;

    // create TCP data packet
    InPacket *inpkt = generatePacket(buf, len);
    if (inpkt == NULL) {
      ERROR_PRINT(TAG, "Error creating inbound data packet.");
      return false;
    }
    inpkt->createTcpData(htonl(seqnum), htonl(acknum), htons(win));
    inpkt->print();

    // write packet to tun
    if (!writeTun(inpkt)) {
      ERROR_PRINT(TAG, "Error writing inbound data packet.");
      return false;
    }

    // update flow info
    dataRecv(seqnum, len, acknum);

    DEBUG_PRINT(TAG, "Inbound TCP data processed.");

    return true;
  }

  bool TcpFlow::processTunRst(bool *done) {
    DEBUG_PRINT(TAG, "Processing TCP RST...");

    // close socket
    closeSock();

    // tell router to abandon flow
    *done = true;

    DEBUG_PRINT(TAG, "TCP RST processed.");

    return true;
  }

  bool TcpFlow::processTunSyn(const OutPacket *outpkt) {
    DEBUG_PRINT(TAG, "Processing TCP SYN...");

    // update flow info
    synSent(ntohl(outpkt->getSeqNum()));

    // init TCP parameters
    srand((unsigned) time(NULL));
    uint32_t isn = (unsigned) rand();
    uint32_t acknum = ntohl(outpkt->getSeqNum()) + 1;
    uint16_t win = 65535;

    // create packet
    InPacket *inpkt = generatePacket(NULL, 0);
    if (inpkt == NULL) {
      ERROR_PRINT(TAG, "Error creating inbound SYN ACK packet.");
      return false;
    }
    inpkt->createSynAck(htonl(isn), htonl(acknum), htons(win));
    inpkt->print();

    // write packet to tun
    if (!writeTun(inpkt)) {
      ERROR_PRINT(TAG, "Error writing inbound SYN ACK packet.");
      return false;
    }

    // update flow info
    synAckRecv(isn, acknum, win);

    DEBUG_PRINT(TAG, "TCP SYN processed.");

    return true;
  }

  bool TcpFlow::processTunAck(const OutPacket *outpkt, bool *done) {
    DEBUG_PRINT(TAG, "Processing TCP ACK...");

    // update state machine, if needed
    ackSent(ntohl(outpkt->getAckNum()));

    // if we had an ACK for an incoming FIN, clear TCP entry
    if (m_state == TIME_WAIT) {
      DEBUG_PRINT(TAG, "TCP flow done (active close).");
      closeSock();
      *done = true;
    }

    DEBUG_PRINT(TAG, "TCP ACK processed.");

    return true;
  }

  bool TcpFlow::processTunFin(const OutPacket *outpkt, bool *done) {
    DEBUG_PRINT(TAG, "Processing TCP FIN...");

    // retransmission/out of order?
    uint32_t seqnum_out = ntohl(outpkt->getSeqNum()) + outpkt->getPayloadLen();
    if (seqnum_out != m_seqnum_out) {
      DEBUG_PRINT(TAG, "TCP out of order dropped.");
      return true;
    }

    // update flow info
    finSent(seqnum_out);

    // switch on type of close (active, passive)
    if (m_state == FIN_WAIT_1) {
      if (!processTunFinActive()) {
        return false;
      }
    } else if (m_state == LAST_ACK) {
      if (!processTunFinPassive(outpkt, done)) {
        return false;
      }
    } else {
      DEBUG_PRINT(TAG, "TCP FIN dropped.");
    }

    DEBUG_PRINT(TAG, "TCP FIN processed.");

    return true;
  }

  bool TcpFlow::processTunFinActive() {
    DEBUG_PRINT(TAG, "Processing FIN for active close...");

    // perform active half-close of socket
    if (shutdown(m_sockfd, SHUT_WR) < 0) {
      common::printSyscallError(TAG, "Error performing active half-close: %s");
      return false;
    }

    DEBUG_PRINT(TAG, "TCP FIN active close processed.");

    return true;
  }

  bool TcpFlow::processTunFinPassive(const OutPacket *outpkt, bool *done) {
    DEBUG_PRINT(TAG, "Processing FIN for passive close...");

    // close socket
    if (!closeSock()) {
      ERROR_PRINT(TAG,
                  "Error closing TCP socket for passive close.");
      return false;
    }

    // init TCP parameters
    uint32_t seqnum = m_seqnum_in;
    uint32_t acknum = ntohl(outpkt->getSeqNum()) + outpkt->getPayloadLen() + 1;
    uint16_t win = m_win_in;

    // create ACK packet
    InPacket *inpkt = generatePacket(NULL, 0);
    if (inpkt == NULL) {
      ERROR_PRINT(TAG, "Error creating inbound ACK packet.");
      return false;
    }
    inpkt->createAck(htonl(seqnum), htonl(acknum), htons(win));
    inpkt->print();

    // write packet to tun
    if (!writeTun(inpkt)) {
      ERROR_PRINT(TAG, "Error writing inbound ACK packet.");
      return false;
    }

    // notify router that processing is complete
    *done = true;

    DEBUG_PRINT(TAG, "TCP FIN passive close processed.");

    return true;
  }

  bool TcpFlow::processTunData(const OutPacket *outpkt) {
    DEBUG_PRINT(TAG, "Processing TCP data...");

    // invalid TCP state?
    if (!sockOpen()) {
      ERROR_PRINT(TAG, "TCP sent data while sock closed.");
      return false;
    }

    // retransmission/out of order?
    uint32_t seqnum_out = ntohl(outpkt->getSeqNum());
    if (seqnum_out != m_seqnum_out) {
      DEBUG_PRINT(TAG, "TCP out of order dropped.");
      return true;
    }

    // pass data to socket
    if (!writeSock(outpkt)) {
      ERROR_PRINT(TAG, "Error writing outbound TCP to socket.");
      return false;
    }

    // update flow info
    dataSent(ntohl(outpkt->getSeqNum()), outpkt->getPayloadLen());

    // init TCP parameters
    uint32_t seqnum = m_seqnum_in;
    uint32_t acknum = ntohl(outpkt->getSeqNum()) + outpkt->getPayloadLen();
    uint16_t win = m_win_in;

    // create ACK packet
    InPacket *inpkt = generatePacket(NULL, 0);
    if (inpkt == NULL) {
      ERROR_PRINT(TAG, "Error creating inbound ACK packet.");
      return false;
    }
    inpkt->createAck(htonl(seqnum), htonl(acknum), htons(win));
    inpkt->print();

    // write packet to tun
    if (!writeTun(inpkt)) {
      ERROR_PRINT(TAG, "Error writing inbound ACK packet.");
      return false;
    }

    // update flow info
    ackRecv(acknum);

    DEBUG_PRINT(TAG, "TCP data processed.");

    return true;
  }

  void TcpFlow::synSent(uint32_t seqnum_out) {
    m_state = SYN_SENT;

    m_seqnum_out = seqnum_out + 1;

    gettimeofday(&m_tfirstbyte, NULL);
    updateTime();
  }

  void TcpFlow::synAckRecv(uint32_t seqnum_in, uint32_t acknum_in, uint16_t win_in) {
    m_state = ESTABLISHED;

    m_seqnum_in = seqnum_in + 1;
    m_acknum_in = acknum_in;
    m_win_in = win_in;

    updateTime();
  }

  void TcpFlow::ackSent(uint32_t acknum_out) {
    m_acknum_out = acknum_out;

    updateTime();
  }

  void TcpFlow::finSent(uint32_t seqnum_out) {
    if (m_state == ESTABLISHED) {
      m_state = FIN_WAIT_1;
    } else if (m_state == CLOSE_WAIT) {
      m_state = LAST_ACK;
    }

    m_seqnum_out = seqnum_out + 1;

    updateTime();
  }

  void TcpFlow::finAckRecv(uint32_t seqnum_in, uint32_t acknum_in) {
    if (m_state == ESTABLISHED) {
      m_state = CLOSE_WAIT;
    } else if (m_state == FIN_WAIT_1) {
      m_state = TIME_WAIT;
    }

    m_seqnum_in = seqnum_in + 1;
    m_acknum_in = acknum_in;

    updateTime();
  }

  void TcpFlow::ackRecv(uint32_t acknum_in) {
    m_acknum_in = acknum_in;

    updateTime();
  }

  void TcpFlow::dataSent(uint32_t seqnum_out, size_t len) {
    m_sent += len;

    m_seqnum_out = seqnum_out + (uint32_t) len;

    updateTime();
  }

  void TcpFlow::dataRecv(uint32_t seqnum_in, size_t len, uint32_t acknum_in) {
    m_recv += len;

    m_seqnum_in = seqnum_in + (uint32_t) len;
    m_acknum_in = acknum_in;

    updateTime();
  }

  std::string TcpFlow::toString() const {
    std::stringstream ss;

    ss << Flow::toString() << " ";

    ss << "state=" << describeState(m_state);
    ss << " seqnum_out=" << m_seqnum_out;
    ss << " acknum_out=" << m_acknum_out;
    ss << " seqnum_in=" << m_seqnum_in;
    ss << " acknum_in=" << m_acknum_in;
    ss << " win_in=" << m_win_in;

    return ss.str();
  }

  const char *TcpFlow::describeState(TcpState state) const {
    switch (state) {
      case CLOSED:
        return "CLOSED";
      case SYN_SENT:
        return "SYN_SENT";
      case ESTABLISHED:
        return "ESTABLISHED";
      case FIN_WAIT_1:
        return "FIN_WAIT_1";
      case TIME_WAIT:
        return "TIME_WAIT";
      case CLOSE_WAIT:
        return "CLOSE_WAIT";
      case LAST_ACK:
        return "LAST_ACK";
      default:
        return "<unknown>";
    }
  }
}
