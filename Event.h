/* ============================================================================
 *  Event.h: Event manager.
 *
 *  CEN64: Cycle-accurate, Efficient Nintendo 64 Simulator.
 *  Copyright (C) 2013, Tyler J. Stachecki.
 *  All rights reserved.
 *
 *  This file is subject to the terms and conditions defined in
 *  file 'LICENSE', which is part of this source code package.
 * ========================================================================= */
#ifndef __CEN64__EVENT_H__
#define __CEN64__EVENT_H__
#include "Common.h"

void CloseEventManagerHandle(int cfd);
int GetEventManagerHandle(int sfd);

void CleanupEventManager(int sfd);
int GetEventManagerPort(int sfd);
int RegisterEventManager(int port);

#endif

