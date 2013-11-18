/* ============================================================================
 *  Examples/Socket.c: Socket API demonstration.
 *
 *  CEN64: Cycle-accurate, Efficient Nintendo 64 Simulator.
 *  Copyright (C) 2013, Tyler J. Stachecki.
 *  All rights reserved.
 *
 *  This file is subject to the terms and conditions defined in
 *  file 'LICENSE', which is part of this source code package.
 * ========================================================================= */
#ifdef _WIN32
#include <winsock2.h>
#include <WS2tcpip.h>
#include <winsock.h>
#endif

#ifdef __cplusplus
#include <cstdio>
#include <cstdlib>
#include <cstring>
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif

#ifndef _WIN32
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

/* ============================================================================
 *  loop: Controls the CEN64 process.
 * ========================================================================= */
static void
loop(int sfd) {
  return;
}

/* ============================================================================
 *  usage: Prints program usage.
 * ========================================================================= */
static void
usage(const char *argv0) {
  printf("Usage: %s [host] <port>\n", argv0);
}

/* ============================================================================
 *  main: Attempts to handleshake with CEN64 and sits in a loop.
 * ========================================================================= */
int
main(int argc, const char *argv[]) {
  int sfd = -1, success = 0;
  struct addrinfo *cur, *res;
  struct addrinfo hints;
  char *endptr;

  long long int port;
  const char *host;
  char buf[8];

#ifdef _WIN32
  WSADATA wsaData;
#endif

  if (argc != 2 && argc != 3) {
    usage(argv[0]);
    return 0;
  }

  host = (argc == 3)
    ? argv[1]
    : "localhost";

  port = strtol(argv[argc - 1], &endptr, 10);
  if (*(argv[argc - 1]) == '\0' || *endptr != '\0') {
    printf("Specified port is not valid.\n");
    usage(argv[0]);
    return 1;
  }

  if (port < 1 || port > 65535) {
    printf("Port must in the range 1..65535.\n");
    usage(argv[0]);
    return 1;
  }

  /* Try to initialize WinSock. */
#ifdef _WIN32
  if (WSAStartup(MAKEWORD(1, 1), &wsaData) != 0) 
    printf("Failed to initialize WinSock.\n");
#endif

  /* Try to get a list of usable interfaces. */
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  sprintf(buf, "%5lld", port);

  if (getaddrinfo(host, buf, &hints, &res) != 0) {
    printf("Failed to get a list of interfaces.\n");
    return 2;
  }

  /* Got a list of interfaces; try to connect()... */
  for (cur = res; cur != NULL; cur = cur->ai_next) {
    if ((sfd = socket(cur->ai_family, cur->ai_socktype, cur->ai_protocol)) < 0)
      continue;

    if (connect(sfd, cur->ai_addr, cur->ai_addrlen) < 0) {
#ifdef _WIN32
      closesocket(sfd);
#else
      close(sfd);
#endif
      continue;
    }

    success = 1;
    break;
  }

  freeaddrinfo(res);

  /* Check if we were able to connect()... */
  if (!success) {
    printf("Failed to connect to the server.\n");
    return 3;
  }

  /* The connection has been established! */
  printf("Connection established.\n");
  loop(sfd);

#ifdef _WIN32
  closesocket(sfd);
  WSACleanup();
#else
  shutdown(sfd, SHUT_RDWR);
  close(sfd);
#endif

  return 0;
}

