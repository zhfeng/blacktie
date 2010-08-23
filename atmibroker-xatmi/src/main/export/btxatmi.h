/*
 * JBoss, Home of Professional Open Source
 * Copyright 2008, Red Hat, Inc., and others contributors as indicated
 * by the @authors tag. All rights reserved.
 * See the copyright.txt in the distribution for a
 * full listing of individual contributors.
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the GNU Lesser General Public License, v. 2.1.
 * This program is distributed in the hope that it will be useful, but WITHOUT A
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
 * You should have received a copy of the GNU Lesser General Public License,
 * v.2.1 along with this distribution; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301, USA.
 */

#ifndef XXATMI_H
#define XXATMI_H

#include "atmiBrokerXatmiMacro.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct msg_opts {
	int priority;	/* msg priority from 0 (lowest) to 9 */
} msg_opts_t;

extern BLACKTIE_XATMI_DLL char* btalloc(msg_opts_t* ctrl, char* type, char* subtype, long size);
#ifdef __cplusplus
}
#endif
#endif // XATMI_H