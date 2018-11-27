#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>

typedef enum {
  PROTO_UDP = 0,
  PROTO_TCP = 1,
} proto_t;

typedef struct {
  char    *name;
  proto_t  proto;
  char    *host;
  int      port;
} test_t;

void run_test(test_t *test, int *passed, int *total);
int run_udp_test(char *host, int port);
int run_tcp_test(char *host, int port);
void gen_data(char *buf, int n);
int writen(int fd, char *buf, int n);
int readn(int fd, char *buf, int n);
void print_banner();

int main(int argc, char **argv) {
  int passed, total;
  char *host;
  int udpport, tcpport;
  test_t test;

  // check args
  if (argc != 4) {
    printf("Usage: client <host> <udp-port> <tcp-port>\n");
    return 1;
  }

  // init
  printf("Running MBZ test suite...\n\n");
  passed = 0;
  total = 0;
  srand(time(NULL));

  // parse args
  host = argv[1];
  udpport = atoi(argv[2]);
  tcpport = atoi(argv[3]);

  // run tests
  test.name  = "udp-echo";
  test.proto = PROTO_UDP;
  test.host  = host;
  test.port  = udpport;
  run_test(&test, &passed, &total);

  test.name  = "tcp-echo";
  test.proto = PROTO_TCP;
  test.host  = host;
  test.port  = tcpport;
  run_test(&test, &passed, &total);

  // done
  printf("\nDone.\n");
  printf("Results: %d/%d passed.\n", passed, total);
  return 0;
}

void run_test(test_t *test, int *passed, int *total) {
  int ok;

  printf("Running test '%s'...", test->name);
  fflush(stdout);

  if (test->proto == PROTO_UDP) {
    ok = run_udp_test(test->host, test->port);
  }
  else if (test->proto == PROTO_TCP) {
    ok = run_tcp_test(test->host, test->port);
  }
  else {
    printf("unknown protocol.\n");
    return;
  }

  if (ok) {
    *passed += 1;
    printf("passed.\n");
  }
  else {
    printf("failed.\n");
  }
  *total += 1;
}

int run_udp_test(char *host, int port) {
  char out[1024];
  char in[1024];
  int sock;
  struct sockaddr_in addr;
  int i;
  int ok;

  // generate random data
  gen_data(out, sizeof(out));

  // create socket
  if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("Unable to create UDP socket");
    return 0;
  }

  // associate socket with remote
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  if (inet_aton(host, &addr.sin_addr) == 0) {
    printf("Unable to parse host address.\n");
    close(sock);
    return 0;
  }

  if (connect(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
    perror("Unable to associate UDP socket with remote");
    close(sock);
    return 0;
  }

  // send data
  if (writen(sock, out, sizeof(out)) < 0) {
    printf("Unable to write UDP socket.\n");
    close(sock);
    return 0;
  }

  // read echo
  if (readn(sock, in, sizeof(in)) < 0) {
    printf("Unable to read UDP socket.\n");
    close(sock);
    return 0;
  }

  // check echo
  for (i = 0; i < sizeof(out); i++) {
    if (out[i] != in[i]) {
      ok = 0;
      break;
    }
  }
  ok = 1;

  // done
  close(sock);
  return ok;
}

int run_tcp_test(char *host, int port) {
  char out[1024];
  char in[1024];
  int sock;
  struct sockaddr_in addr;
  int i;
  int ok;

  // generate random data
  gen_data(out, sizeof(out));

  // create socket
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("Unable to create TCP socket");
    return 0;
  }

  // associate socket with remote
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  if (inet_aton(host, &addr.sin_addr) == 0) {
    printf("Unable to parse host address.\n");
    close(sock);
    return 0;
  }

  if (connect(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
    perror("Unable to connect to TCP remote host");
    close(sock);
    return 0;
  }

  // send data
  if (writen(sock, out, sizeof(out)) < 0) {
    printf("Unable to write TCP socket.\n");
    close(sock);
    return 0;
  }

  // read echo
  if (readn(sock, in, sizeof(in)) < 0) {
    printf("Unable to read TCP socket.\n");
    close(sock);
    return 0;
  }

  // check echo
  for (i = 0; i < sizeof(out); i++) {
    if (out[i] != in[i]) {
      ok = 0;
      break;
    }
  }
  ok = 1;

  // done
  close(sock);
  return ok;
}

int writen(int fd, char *buf, int n) {
  int nw, rc;

  nw = 0;
  while (nw < n) {
    rc = write(fd, buf + nw, n - nw);

    if (rc < 0) {
      perror("writen");
      return rc;
    }

    nw += rc;
  }

  return nw;
}

int readn(int fd, char *buf, int n) {
  int nr, rc;

  nr = 0;
  while (nr < n) {
    rc = read(fd, buf + nr, n - nr);

    if (rc < 0) {
      perror("readn");
      return rc;
    }

    nr += rc;
  }

  return nr;
}

void gen_data(char *buf, int n) {
  int *p;

  for (p = (int*) buf; p < (int*) (buf+n); p++) {
    *p = rand();
  }
}

void print_banner() {
  printf("\n=============================================================\n");
}
