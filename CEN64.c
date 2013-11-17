/* ============================================================================
 *  CEN64.c: Main application.
 *
 *  CEN64: Cycle-accurate, Efficient Nintendo 64 Simulator.
 *  Copyright (C) 2013, Tyler J. Stachecki.
 *  All rights reserved.
 *
 *  This file is subject to the terms and conditions defined in
 *  file 'LICENSE', which is part of this source code package.
 * ========================================================================= */
#include "CEN64.h"
#include "Common.h"
#include "Device.h"
#include "Event.h"

#ifdef __cplusplus
#include <csetjmp>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#else
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif

#ifdef GLFW3
#include <GLFW/glfw3.h>
#else
#include <GL/glfw.h>
#endif

#ifdef _WIN32
#include <winsock.h>
#endif

#include <signal.h>

#ifdef GLFW3
GLFWwindow *window;
#endif

/* GLFW seems to like global state. */
/* We'll jmp back into main at close. */
static jmp_buf env;
static int RequestShutdown(void);
static void SignalHandler(int);

#ifdef GLFW3
static void WindowResizeCallback(void *window, int width, int height);
#else
static void WindowResizeCallback(int width, int height);
#endif

/* ============================================================================
 *  CreateWindow: Creates an OpenGL window.
 * ========================================================================= */
static int
CreateWindow(void** window, bool fullscreen) {
#ifdef GLFW3
  GLFWwindow myWindow;
  glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);

  glfwWindowHint(GLFW_RED_BITS, 5);
  glfwWindowHint(GLFW_GREEN_BITS, 6);
  glfwWindowHint(GLFW_BLUE_BITS, 5);
  glfwWindowHint(GLFW_ALPHA_BITS, 0);
  glfwWindowHint(GLFW_DEPTH_BITS, 8);
  glfwWindowHint(GLFW_STENCIL_BITS, 0);

  if ((myWindow = glfwCreateWindow(640, 480, "CEN64", NULL, NULL)) == NULL)
    return -1;

  glfwMakeContextCurrent(window);
  glfwSetWindowCloseCallback(window, RequestShutdown);
  glfwSetWindowSizeCallback(window, WindowResizeCallback);

  memcpy(window, &myWindow, sizeof(myWindow));
  SetVIFContext(device->vif, window);

#else
  glfwOpenWindowHint(GLFW_WINDOW_NO_RESIZE, GL_FALSE);
  if (glfwOpenWindow(640, 480, 5, 6, 5, 0, 8, 0, GLFW_WINDOW) != GL_TRUE)
    return -2;

  glfwSetWindowTitle("CEN64");
  glfwSetWindowCloseCallback(RequestShutdown);
  glfwSetWindowSizeCallback(WindowResizeCallback);
#endif

  glfwPollEvents();
  return 0;
}

/* ============================================================================
 *  DestroyWindow: Tears down the OpenGL window.
 * ========================================================================= */
static void DestroyWindow(void *window) {
#ifdef GLFW3
  glfwDestroyWindow((GLFWwindow*) window);
#else
  glfwCloseWindow();
#endif
}

/* ============================================================================
 *  ParseArgs: Parses the argument list and performs actions.
 * ========================================================================= */
static int
ParseArgs(int args, const char *argv[], struct CEN64Device *device, int *port) {
  int i;

  /* TODO: getops or something sensible. */
  for (i = 0; i < args; i++) {
    const char *arg = argv[i];

    while (*arg == ' ');

    /* Accept -, --, and / */
    if (*arg == '-') {
      arg++;

      if (*arg == '-')
        arg++;
    }

    else if (*arg == '/')
      arg++;

    /* Set Controller Type. */
    if (!strcmp(arg, "controller")) {
      if (++i >= args) {
        printf("-controller: Missing argument; ignoring.\n");
        continue;
      }

      SetControlType(device->pif, argv[i]);
    }

    /* Set backing EEPROM file. */
    else if (!strcmp(arg, "eeprom")) {
      if (++i >= args) {
        printf("-eeprom: Missing argument; ignoring.\n");
        continue;
      }

      SetEEPROMFile(device->pif, argv[i]);
    }

    /* Set the communications port. */
    else if (!strcmp(arg, "port")) {
      char *endptr;

      long long int num = strtol(argv[++i], &endptr, 10);

      if (*(argv[i]) == '\0' || *endptr != '\0') {
        printf("-port: Needs a numeric argument.\n");
        continue;
      }

      else if (num < 0 || num > 65535) {
        printf("-port: Argument must be in range: 0..65535.\n");
        continue;
      }

      *port = num;
    }

    /* Set backing SRAM file. */
    else if (!strcmp(arg, "sram")) {
      if (++i >= args) {
        printf("-sram: Missing argument; ignoring.\n");
        continue;
      }

      SetSRAMFile(device->rom, argv[i]);
    }
  }

  return 0;
}

/* ============================================================================
 *  RequestShutdown: Somebody told us to stop executing; bail out...
 * ========================================================================= */
static int
RequestShutdown(void) {
  longjmp(env, 1);
  return 0;
}

/* ============================================================================
 *  RunConsole: Executes the console until we get interrupted.
 * ========================================================================= */
static void
RunConsole(struct CEN64Device *device) {
  signal(SIGINT, SignalHandler);

  if (setjmp(env) == 0) {
    while (1)
      CycleDevice(device);
  }
}

/* ============================================================================
 *  SignalHandler: Called on SIGINT traps, calls RequestShutdown.
 * ========================================================================= */
static void
SignalHandler(int sig) {
  RequestShutdown();
}

/* ============================================================================
 *  DestroyResources: Destroy any resources; call before exiting.
 * ========================================================================= */
static void
DestroyResources(struct CEN64Device *device, void *window, int sfd, int cfd) {
  if (device != NULL)
    DestroyDevice(device);

  if (window != NULL)
  DestroyWindow(window);

  if (cfd >= 0)
    CloseEventManagerHandle(cfd);

  if (sfd >= 0)
    CleanupEventManager(sfd);

#ifdef _WIN32
  WSACleanup();
#endif

  glfwTerminate();
}

/* ============================================================================
 *  WindowResizeCallback: GLFW window resized; fill window, but maintain
 *  aspect ratio.
 * ========================================================================= */
#ifdef GLFW3
static void
WindowResizeCallback(GLFWwindow *window, int width, int height) {
#else
static void
WindowResizeCallback(int width, int height) {
#endif
  float aspect = 4.0 / 3.0;

  if (height <= 0)
    height = 1;

  glViewport(0, 0, width, height);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  if((float)width / (float)height > aspect) {
    aspect = 3.0 / 4.0;
    aspect *= (float)width / (float)height;
    glOrtho(-aspect, aspect, -1, 1, -1, 1);
  }

  else {
    aspect *= (float)height / (float)width;
    glOrtho(-1, 1, -aspect, aspect, -1, 1);
  }

  glClear(GL_COLOR_BUFFER_BIT);
}

/* ============================================================================
 *  main: Parses arguments and kicks off the application.
 * ========================================================================= */
int main(int argc, const char *argv[]) {
  int sfd = -1, port = -1;
  struct CEN64Device *device;
  void *window = NULL;
  int cfd = -1;

#ifdef _WIN32
  WSADATA wsaData;
#endif

  if (argc < 3) {
    printf(
      "Usage: %s [options] <pifrom> <cart>\n\n"
      "Options:\n"
      "  -controller [keyboard,mayflash64,retrolink,wiiu,x360]\n"
      "  -eeprom <file>\n"
      "  -port 0, <1..65535>\n"
      "  -sram <file>\n\n",
      argv[0]);

    printf("RSP Build Type: %s\nRDP Build Type: %s\n",
      RSPBuildType, RDPBuildType);

    return 0;
  }

#ifdef _WIN32
  if (WSAStartup(MAKEWORD(1, 1), &wsaData) != 0) {
    printf("Failed to initialize WinSock.\n");
    DestroyResources(NULL, NULL, -1);
    return 1;
  }
#endif

  /* Kick off a window and such. */
  if (glfwInit() != GL_TRUE) {
    printf("Failed to initialize GLFW.\n");
    return 2;
  }

  if (CreateWindow(&window, false) < 0) {
    printf("Failed to open a GLFW window.\n");
    DestroyResources(NULL, NULL, -1, -1);
    return 2;
  }

  /* Parse command line arguments, build the console. */
  if ((device = CreateDevice(argv[argc - 2])) == NULL) {
    printf("Failed to create a device.\n");
    DestroyResources(NULL, window, -1, -1);
    return 3;
  }

  if (ParseArgs(argc - 3, argv + 1, device, &port)) {
    DestroyResources(device, window, -1, -1);
    return 255;
  }

  /* Establish a communication vector with frontend. */
  /* If user doesn't want one, then just ignore this. */
  if (port >= 0) {
    if ((sfd = RegisterEventManager(port)) < 0 ||
      (port = GetEventManagerPort(sfd)) < 0) {
      printf("Failed to create a socket.\n");
      DestroyResources(device, window, -1, -1);
      return 4;
    }

    /* We got a port, wait for the connect. */
    /* Limit ourselves to one client for now. */
    printf("%d\n", port);

    cfd = GetEventManagerHandle(sfd);
    CloseEventManagerHandle(sfd);
    sfd = -1;
  }

  if (LoadCartridge(device, argv[argc - 1])) {
    printf("Failed to load the ROM.\n");
    DestroyResources(device, window, sfd, cfd);
    return 5;
  }

  /* Main loop: check for work, execute. */
  debug("== Booting the Console ==");

  RunConsole(device);

  /* Print statistics, gracefully terminate. */
  debug("== Destroying the Console ==");

#ifndef NDEBUG
  RSPDumpStatistics(device->rsp);
  VR4300DumpStatistics(device->vr4300);
#endif

  DestroyResources(device, window, sfd, cfd);
  return 0;
}

