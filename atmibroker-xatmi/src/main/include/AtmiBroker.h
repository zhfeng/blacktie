/*
 * JBoss, Home of Professional Open Source
 * Copyright 2008, Red Hat Middleware LLC, and others contributors as indicated
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
/*
 * BREAKTHRUIT PROPRIETARY - NOT TO BE DISCLOSED OUTSIDE BREAKTHRUIT, LLC.
 */
// copyright 2006, 2008 BreakThruIT

// AtmiBroker.h

#ifndef AtmiBroker_H
#define AtmiBroker_H

#ifdef TAO_COMP
#include <tao/ORB.h>
#include <orbsvcs/CosNamingS.h>
#include <tao/PortableServer/PortableServerC.h>
#endif

#include "Sender.h"
#include "AtmiBrokerClient.h"

extern int _tperrno;
extern long _tpurcode;
extern bool loggerInitialized;
extern Sender* get_service_queue(const char * serviceName);
extern CORBA::ORB_var client_orb;
extern PortableServer::POA_var client_root_poa;
extern PortableServer::POAManager_var client_root_poa_manager;
extern CosNaming::NamingContextExt_var client_default_context;
extern CosNaming::NamingContext_var client_name_context;
extern PortableServer::POA_var client_poa;
extern AtmiBrokerClient * ptrAtmiBrokerClient;
extern CORBA::PolicyList *policyList;

#endif //AtmiBroker_H
