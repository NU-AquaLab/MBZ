#include <arpa/inet.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>

static int udpsock = -1;
static int tcpsock = -1;

void handle_sigint(int signum);
int handle_udp(int fd);
int handle_new_tcp(int fd);
int handle_tcp(int fd);
void cleanup();

int main(int argc, char **argv) {
  int udpport, tcpport;
  int n_connections;
  struct sigaction sigact;
  struct sockaddr_in udpaddr, tcpaddr;
  fd_set fds, readfds;
  int nfds;
  int i, newfd;

  // check arguments
  if (argc != 4) {
    printf("Usage: server <udp-port> <tcp-port> <n-connections>\n");
    return 1;
  }

  // init
  printf("Initializing MBZ test server...\n");

  // parse arguments
  udpport = atoi(argv[1]);
  tcpport = atoi(argv[2]);
  n_connections = atoi(argv[3]);

  // install SIGINT handler
  memset(&sigact, 0, sizeof(sigact));
  sigact.sa_handler = handle_sigint;
  if (sigaction(SIGINT, &sigact, NULL) < 0) {
    perror("Unable to install SIGINT handler");
    return 2;
  }

  // create UDP socket
  if ((udpsock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("Unable to create UDP socket");
    cleanup();
    return 2;
  }

  memset(&udpaddr, 0, sizeof(udpaddr));
  udpaddr.sin_family = AF_INET;
  udpaddr.sin_port = htons(udpport);
  udpaddr.sin_addr.s_addr = INADDR_ANY;

  if (bind(udpsock, (struct sockaddr *) &udpaddr, sizeof(udpaddr)) < 0) {
    perror("Unable to bind UDP socket");
    cleanup();
    return 2;
  }

  printf("UDP socket initialized.\n");

  // create TCP socket
  if ((tcpsock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("Unable to create TCP socket");
    cleanup();
    return 2;
  }

  memset(&tcpaddr, 0, sizeof(tcpaddr));
  tcpaddr.sin_family = AF_INET;
  tcpaddr.sin_port = htons(tcpport);
  tcpaddr.sin_addr.s_addr = INADDR_ANY;

  if (bind(tcpsock, (struct sockaddr *) &tcpaddr, sizeof(tcpaddr)) < 0) {
    perror("Unable to bind TCP socket");
    cleanup();
    return 2;
  }

  if (listen(tcpsock, n_connections) < 0) {
    perror("Unable to listen on TCP socket");
    cleanup();
    return 2;
  }

  printf("TCP socket initialized.\n");

  // init fd sets
  FD_ZERO(&fds);
  FD_ZERO(&readfds);
  FD_SET(udpsock, &fds);
  FD_SET(tcpsock, &fds);
  nfds = udpsock > tcpsock ? udpsock + 1 : tcpsock + 1;

  // respond to requests
  printf("Waiting for requests...\n");
  while (1) {
    readfds = fds;

    if (select(nfds, &readfds, NULL, NULL, NULL) < 0) {
      perror("Select");
      break;
    }

    for (i = 0; i < nfds; i++) {
      if (FD_ISSET(i, &readfds)) {
        if (i == udpsock) {
          if (handle_udp(udpsock) < 0) {
            FD_CLR(udpsock, &fds);
          }
        }
        else if (i == tcpsock) {
          if ((newfd = handle_new_tcp(tcpsock)) < 0) {
            printf("Unable to accept TCP connection.\n");
            FD_CLR(tcpsock, &fds);
          }
          else {
            FD_SET(newfd, &fds);
            nfds = nfds > newfd ? nfds : newfd + 1;
          }
        }
        else {
          if (handle_tcp(i) <= 0) {
            close(i);
            FD_CLR(i, &fds);
          }
        }
      }
    }
  }

  // cleanup
  cleanup();
  return 0;
}

void handle_sigint(int signum) {
  printf("Received signal %d.\n", signum);
  cleanup();
  exit(0);
}

int handle_udp(int fd) {
  char buf[1024];
  struct sockaddr_in srcaddr;
  socklen_t addrlen;
  int nr, ns;

  // get data
  memset(&srcaddr, 0, sizeof(srcaddr));
  addrlen = sizeof(srcaddr);
  if ((nr = recvfrom(fd, buf, sizeof(buf), 0, 
                     (struct sockaddr *) &srcaddr, &addrlen)) < 0) {
    perror("Unable to receive UDP data");
    return nr;
  }
  printf("UDP data from %s:%u\n", 
         inet_ntoa(srcaddr.sin_addr), 
         ntohs(srcaddr.sin_port));

  // echo data
  if ((ns = sendto(fd, buf, nr, 0, 
                   (struct sockaddr *) &srcaddr, addrlen)) < 0) {
    perror("Uanble to send UDP data");
    return ns;
  }
  if (ns != nr) {
    printf("Failed to send all UDP data.\n");
    return -1;
  }

  return nr;
}

int handle_new_tcp(int fd) {
  int sock;
  struct sockaddr_in srcaddr;
  socklen_t addrlen;

  // accept connection
  memset(&srcaddr, 0, sizeof(srcaddr));
  addrlen = sizeof(srcaddr);
  if ((sock = accept(fd, (struct sockaddr *) &srcaddr, &addrlen)) < 0) {
    perror("accept");
    return sock;
  }
  printf("TCP conn from %s:%u\n", 
         inet_ntoa(srcaddr.sin_addr), 
         ntohs(srcaddr.sin_port));

  return sock;
}

int handle_tcp(int fd) {
  char buf[1024];
  int nr, ns;

  // get data
  if ((nr = read(fd, buf, sizeof(buf))) < 0) {
    perror("Unable to read TCP data");
    return nr;
  }

  // echo data
  if (nr > 0) {
    if ((ns = write(fd, buf, nr)) < 0) {
      perror("Unable to write TCP data");
      return ns;
    }
    if (ns != nr) {
      printf("Failed to write all TCP data.\n");
      return -1;
    }
  }

  return nr;
}

void cleanup() {
  if (udpsock > 0) {
    close(udpsock);
    udpsock = -1;
  }
  if (tcpsock > 0) {
    close(tcpsock);
    tcpsock = -1;
  }

  printf("MBZ test server de-init.\n");
}
