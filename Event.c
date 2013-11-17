/* ============================================================================
 *  Event.c: Event manager.
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

#include "Common.h"
#include "Event.h"

#ifdef __cplusplus
#include <cstdio>
#include <cstring>
#else
#include <stdio.h>
#include <string.h>
#endif

#ifndef _WIN32
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#endif

/* ============================================================================
 *  CleanupEventManager: Releases any resources used for event communication.
 * ========================================================================= */
void CleanupEventManager(int sfd) {
#ifdef _WIN32
  closesocket(sfd);
#else
  close(sfd);
#endif
}

/* ============================================================================
 *  CloseEventManagerHandle: Shuts down a connected handle and releases it.
 * ========================================================================= */
void CloseEventManagerHandle(int cfd) {
#ifdef _WIN32
  closesocket(cfd);
#else
  shutdown(cfd, SHUT_RDWR);
  close(cfd);
#endif
}

/* ============================================================================
 *  GetEventManagerHandle: Waits for a client to connect, returns a handle.
 * ========================================================================= */
int GetEventManagerHandle(int sfd) {
  struct sockaddr addr;
  socklen_t addrlen;
  int cfd;

  do {
    cfd = accept(sfd, &addr, &addrlen);
  } while (cfd < 0);

  return cfd;
}

/* ============================================================================
 *  GetEventManagerPort: Returns the port used for event communication.
 * ========================================================================= */
int GetEventManagerPort(int sfd) {
  struct sockaddr_in sin;
  socklen_t len = sizeof(sin);

  if (getsockname(sfd, (struct sockaddr *) &sin, &len) == -1)
    return -1;

  return ntohs(sin.sin_port);
}

/* ============================================================================
 *  RegisterEventManager: Creates a new socket for event communication.
 * ========================================================================= */
int RegisterEventManager(int port) {
  char buf[8], *bufptr = NULL;
  struct addrinfo *cur, *res;
  struct addrinfo hints;
  int sfd = -1;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  sprintf(buf, "%5d", port);
  bufptr = buf;

  /* Try to enumerate a list of matching interfaces. */
  if (getaddrinfo(NULL, bufptr, &hints, &res) != 0)
    return -1;

  /* Try to crawl over the result list; */
  /* create a socket, and try to bind it. */
  for (cur = res; cur != NULL; cur = cur->ai_next) {
    if ((sfd = socket(cur->ai_family, cur->ai_socktype, cur->ai_protocol)) < 0)
      continue;

    if (bind(sfd, cur->ai_addr, cur->ai_addrlen) < 0) {
#ifdef _WIN32
      closesocket(sfd);
#else
      close(sfd);
#endif
      continue;
    }

    if (listen(sfd, 1) == -1) {
#ifdef _WIN32
      closesocket(sfd);
#else
      close(sfd);
#endif

      continue;
    }

    /* If we got this far, we have bound socket. */
    freeaddrinfo(res);
    return sfd;
  }

  freeaddrinfo(res);
  return -1;
}

