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
#ifndef SessionImpl_H_
#define SessionImpl_H_

#include "atmiBrokerCorbaMacro.h"

#include "log4cxx/logger.h"
#include "Session.h"
#include "ConnectionImpl.h"
#include "EndpointQueue.h"

class ConnectionImpl;

class ATMIBROKER_CORBA_DLL SessionImpl: public virtual Session {
public:
	SessionImpl(ConnectionImpl* connection, int id);

	SessionImpl(ConnectionImpl* connection, int id, const char* service);

	virtual ~SessionImpl();

	void setSendTo(char* replyTo);

	char* getSendTo();

	const char* getReplyTo();

	MESSAGE receive(long time);

	void send(MESSAGE message);

	virtual Destination* getDestination();

	void setCanSend(bool canSend);

	void setCanRecv(bool canRecv);

	bool getCanSend();

	bool getCanRecv();

	int getId();
private:
	static log4cxx::LoggerPtr logger;
	int id;
	ConnectionImpl* connection;
	EndpointQueue* temporaryQueue;
	AtmiBroker::EndpointQueue_var remoteEndpoint;

	const char* replyTo;
	char* sendTo;
	bool canSend;
	bool canRecv;
};

#endif
