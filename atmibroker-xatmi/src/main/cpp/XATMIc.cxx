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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <iostream>

#include "log4cxx/logger.h"

#include "ThreadLocalStorage.h"
#include "txClient.h"
#include "xatmi.h"
#include "Session.h"
#include "AtmiBrokerClientControl.h"
#include "AtmiBrokerServerControl.h"
#include "AtmiBrokerClient.h"
#include "AtmiBrokerServer.h"
#include "AtmiBrokerMem.h"

//#include "AtmiBrokerCodes.h"
char* dTPERESET = (char*) "0";
char* dTPEBADDESC = (char*) "2";
char* dTPEBLOCK = (char*) "3";
char* dTPEINVAL = (char*) "4";
char* dTPELIMIT = (char*) "5";
char* dTPENOENT = (char*) "6";
char* dTPEOS = (char*) "7";
char* dTPEPROTO = (char*) "9";
char* dTPESVCERR = (char*) "10";
char* dTPESVCFAIL = (char*) "11";
char* dTPESYSTEM = (char*) "12";
char* dTPETIME = (char*) "13";
char* dTPETRAN = (char*) "14";
char* dTPGOTSIG = (char*) "15";
char* dTPEITYPE = (char*) "17";
char* dTPEOTYPE = (char*) "18";
char* dTPEEVENT = (char*) "22";
char* dTPEMATCH = (char*) "23";

// Global state
int _tperrno = 0;
long _tpurcode = -1;

// Logger for XATMIc
log4cxx::LoggerPtr loggerXATMI(log4cxx::Logger::getLogger("loggerXATMI"));

int bufferSize(char* data, int suggestedSize) {
	int data_size = ::tptypes(data, NULL, NULL);
	if (data_size >= 0) {
		if (suggestedSize <= 0 || suggestedSize > data_size) {
			return data_size;
		} else {
			return suggestedSize;
		}
	} else {
		LOG4CXX_DEBUG(loggerXATMI,
				(char*) "A NON-BUFFER WAS ATTEMPTED TO BE SENT");
		setSpecific(TPE_KEY, dTPEINVAL);
		return -1;
	}

}
int send(Session* session, const char* replyTo, char* idata, long ilen,
		int correlationId, long flags, long rval, long rcode) {
	LOG4CXX_DEBUG(loggerXATMI, (char*) "send - idata: %s ilen: %d flags: %d"
			<< idata << " " << ilen << " " << flags);
	if (flags & TPSIGRSTRT) {
		LOG4CXX_ERROR(loggerXATMI, (char*) "TPSIGRSTRT NOT SUPPORTED");
	}
	int toReturn = -1;
	if (session->getCanSend()) {
		try {
			void* control = 0;

			if (~TPNOTRAN & flags) {
				// don't run the call in a transaction
				control = disassociate_tx();
			}

			char* data_togo = (char *) malloc(ilen);
			memcpy(data_togo, idata, ilen);

			MESSAGE message;
			message.replyto = replyTo;
			message.data = data_togo;
			message.len = ilen;
			message.correlationId = correlationId;
			message.flags = flags;
			message.rcode = rcode;
			message.rval = rval;
			session->send(message);
			toReturn = 0;

			if (control) {
				associate_tx(control);
			}
		} catch (...) {
			LOG4CXX_ERROR(loggerXATMI,
					(char*) "aCorbaService->start_conversation(): call failed");
			setSpecific(TPE_KEY, dTPESYSTEM);
		}
	} else {
		setSpecific(TPE_KEY, dTPEPROTO);
	}

	return toReturn;
}

int receive(Session* session, char ** odata, long *olen, long flags,
		long* event) {
	int toReturn = -1;
	int len = ::bufferSize(*odata, *olen);
	if (len != -1) {
		LOG4CXX_DEBUG(loggerXATMI,
				(char*) "tprecv - odata: %s olen: %p flags: %d" << *odata
						<< " " << olen << " " << flags);
		if (flags & TPSIGRSTRT) {
			LOG4CXX_ERROR(loggerXATMI, (char*) "TPSIGRSTRT NOT SUPPORTED");
		}
		if (session->getCanRecv()) {
			// TODO Make configurable Default wait time is blocking (x1000 in SynchronizableObject)
			long time = 0;
			if (TPNOBLOCK & flags) {
				time = 1;
			} else if (TPNOTIME & flags) {
				time = 0;
			}
			MESSAGE message = session->receive(time);
			if (message.data != NULL) {
				// TODO Handle TPNOCHANGE
				if (len < message.len) {
					*odata = ::tprealloc(*odata, message.len);
				}
				*olen = message.len;
				memcpy(*odata, (char*) message.data, *olen);
				if (message.rcode == TPESVCFAIL) {
					*event = TPESVCFAIL;
				} else if (message.rcode == TPESVCERR) {
					*event = TPESVCERR;
				}
				try {
					if (message.replyto != NULL && strcmp(message.replyto, "")
							!= 0) {
						session->setSendTo(message.replyto);
					} else {
						session->setSendTo(NULL);
					}
					if (message.flags & TPRECVONLY) {
						session->setCanSend(true);
						session->setCanRecv(false);
					}
				} catch (...) {
					LOG4CXX_ERROR(
							loggerXATMI,
							(char*) "Could not set the send to destination to: "
									<< message.replyto);
				}
				LOG4CXX_DEBUG(loggerXATMI, (char*) "returning - %s" << *odata);
				toReturn = 0;
			} else {
				setSpecific(TPE_KEY, dTPETIME);
			}
		} else {
			setSpecific(TPE_KEY, dTPEPROTO);
		}
	} else {
		setSpecific(TPE_KEY, dTPEOTYPE);
	}
	return toReturn;
}

int _get_tperrno(void) {
	LOG4CXX_DEBUG(loggerXATMI, (char*) "_get_tperrno");
	//return &_tperrno;
	char* err = (char*) getSpecific(TPE_KEY);
	int toReturn = 0;
	if (err != NULL) {
		toReturn = atoi(err);
	}
	return toReturn;
}

long * _get_tpurcode(void) {
	LOG4CXX_ERROR(loggerXATMI, (char*) "_get_tpurcode - Not implemented");
	return &_tpurcode;
}

int tpadvertise(char * svcname, void(*func)(TPSVCINFO *)) {
	LOG4CXX_TRACE(loggerXATMI, (char*) "tpadvertise: " << svcname);
	setSpecific(TPE_KEY, dTPERESET);
	int toReturn = -1;
	if (ptrServer != NULL) {
		if (ptrServer->advertiseService(svcname, func)) {
			toReturn = 0;
		}
	} else {
		LOG4CXX_ERROR(loggerXATMI, (char*) "server not initialized");
		setSpecific(TPE_KEY, dTPESYSTEM);
	}
	LOG4CXX_TRACE(loggerXATMI, (char*) "tpadvertise return: " << toReturn
			<< " tperrno: " << tperrno);
	return toReturn;
}

int tpunadvertise(char * svcname) {
	LOG4CXX_TRACE(loggerXATMI, (char*) "tpunadvertise: " << svcname);
	setSpecific(TPE_KEY, dTPERESET);
	int toReturn = -1;
	if (ptrServer != NULL) {
		if (svcname && strcmp(svcname, "") != 0) {
			if (ptrServer->isAdvertised(svcname)) {
				ptrServer->unadvertiseService(svcname);
				toReturn = 0;
			} else {
				setSpecific(TPE_KEY, dTPENOENT);
			}
		} else {
			setSpecific(TPE_KEY, dTPEINVAL);
		}
	} else {
		LOG4CXX_ERROR(loggerXATMI, (char*) "server not initialized");
		setSpecific(TPE_KEY, dTPESYSTEM);
	}
	LOG4CXX_TRACE(loggerXATMI, (char*) "tpunadvertise return: " << toReturn
			<< " tperrno: " << tperrno);
	return toReturn;
}

char* tpalloc(char* type, char* subtype, long size) {
	LOG4CXX_TRACE(loggerXATMI, (char*) "tpalloc: " << type << " " << subtype
			<< " " << size);
	setSpecific(TPE_KEY, dTPERESET);
	char* toReturn =
			AtmiBrokerMem::get_instance()->tpalloc(type, subtype, size);
	LOG4CXX_TRACE(loggerXATMI, (char*) "tpalloc returning" << " tperrno: "
			<< tperrno);
	return toReturn;
}

char* tprealloc(char * addr, long size) {
	LOG4CXX_TRACE(loggerXATMI, (char*) "tprealloc: " << size);
	setSpecific(TPE_KEY, dTPERESET);
	char* toReturn = AtmiBrokerMem::get_instance()->tprealloc(addr, size);
	LOG4CXX_TRACE(loggerXATMI, (char*) "tprealloc returning" << " tperrno: "
			<< tperrno);
	return toReturn;
}

void tpfree(char* ptr) {
	LOG4CXX_TRACE(loggerXATMI, (char*) "tpfree");
	setSpecific(TPE_KEY, dTPERESET);
	AtmiBrokerMem::get_instance()->tpfree(ptr);
	LOG4CXX_TRACE(loggerXATMI, (char*) "tpfree returning" << " tperrno: "
			<< tperrno);
}

long tptypes(char* ptr, char* type, char* subtype) {
	LOG4CXX_TRACE(loggerXATMI, (char*) "tptypes: " << type << " " << subtype);
	setSpecific(TPE_KEY, dTPERESET);
	long toReturn = AtmiBrokerMem::get_instance()->tptypes(ptr, type, subtype);
	LOG4CXX_TRACE(loggerXATMI, (char*) "tptypes return: " << toReturn
			<< " tperrno: " << tperrno);
	return toReturn;
}

int tpcall(char * svc, char* idata, long ilen, char ** odata, long *olen,
		long flags) {
	LOG4CXX_TRACE(loggerXATMI, (char*) "tpcall " << svc);
	setSpecific(TPE_KEY, dTPERESET);
	int toReturn = -1;
	if (clientinit() != -1) {
		int cd = tpacall(svc, idata, ilen, flags);
		if (cd != -1) {
			toReturn = tpgetrply(&cd, odata, olen, flags);
		}
	}
	LOG4CXX_TRACE(loggerXATMI, (char*) "tpcall return: " << toReturn
			<< " tperrno: " << tperrno);
	return toReturn;
}

int tpacall(char * svc, char* idata, long ilen, long flags) {
	LOG4CXX_TRACE(loggerXATMI, (char*) "tpacall " << svc);
	setSpecific(TPE_KEY, dTPERESET);
	int len = ::bufferSize(idata, ilen);
	int toReturn = -1;
	if (len != -1) {
		if (clientinit() != -1) {
			Session* session = NULL;
			try {
				int cd = -1;
				session = ptrAtmiBrokerClient->createSession(cd, svc);
				if (cd != -1) {
					::send(session, session->getReplyTo(), idata, len, cd,
							flags, 0, 0);
					if (strcmp((const char*) getSpecific(TPE_KEY), dTPERESET) == 0) {
						if (TPNOREPLY & flags) {
							toReturn = 0;
							ptrAtmiBrokerClient->closeSession(cd);
						} else {
							toReturn = cd;
						}
					}
				} else {
					setSpecific(TPE_KEY, dTPELIMIT);
				}
			} catch (...) {
				LOG4CXX_ERROR(loggerXATMI,
						(char*) "tpconnect failed to connect to service queue");
				setSpecific(TPE_KEY, dTPENOENT);
			}
		}
	}
	LOG4CXX_TRACE(loggerXATMI, (char*) "tpacall return: " << toReturn
			<< " tperrno: " << tperrno);
	return toReturn;
}

int tpconnect(char * svc, char* idata, long ilen, long flags) {
	LOG4CXX_TRACE(loggerXATMI, (char*) "tpconnect " << svc);
	setSpecific(TPE_KEY, dTPERESET);
	int toReturn = -1;
	if (flags & TPSENDONLY || flags & TPRECVONLY) {
		int len = 0;
		if (idata != NULL) {
			len = ::bufferSize(idata, ilen);
		}
		if (len != -1) {
			if (clientinit() != -1) {
				int cd = -1;
				Session* session = NULL;
				try {
					session = ptrAtmiBrokerClient->createSession(cd, svc);
					if (cd != -1) {
						::send(session, session->getReplyTo(), idata, len, cd,
								flags, 0, 0);
						if (flags & TPRECVONLY) {
							session->setCanSend(false);
						} else {
							session->setCanRecv(false);
						}
						toReturn = cd;
					} else {
						setSpecific(TPE_KEY, dTPELIMIT);
					}
				} catch (...) {
					LOG4CXX_ERROR(
							loggerXATMI,
							(char*) "tpconnect failed to connect to service queue");
					setSpecific(TPE_KEY, dTPENOENT);
				}
			}
		}
	} else {
		setSpecific(TPE_KEY, dTPEINVAL);
	}
	LOG4CXX_TRACE(loggerXATMI, (char*) "tpconnect return: " << toReturn
			<< " tperrno: " << tperrno);
	return toReturn;
}

int tpgetrply(int *id, char ** odata, long *olen, long flags) {
	LOG4CXX_TRACE(loggerXATMI, (char*) "tpgetrply " << id);
	setSpecific(TPE_KEY, dTPERESET);
	int toReturn = -1;
	if (clientinit() != -1) {
		if (id && olen) {
			Session* session = ptrAtmiBrokerClient->getSession(*id);
			if (session == NULL) {
				setSpecific(TPE_KEY, dTPEBADDESC);
			} else {
				long event;
				toReturn = ::receive(session, odata, olen, flags, &event);
				ptrAtmiBrokerClient->closeSession(*id);
				if (event == TPESVCERR) {
					setSpecific(TPE_KEY, dTPESVCERR);
					toReturn = -1;
				} else if (event == TPESVCFAIL) {
					setSpecific(TPE_KEY, dTPESVCFAIL);
					toReturn = -1;
				}
				LOG4CXX_TRACE(loggerXATMI, (char*) "tpgetrply session closed");
			}
		} else {
			setSpecific(TPE_KEY, dTPEINVAL);
		}
	}
	LOG4CXX_TRACE(loggerXATMI, (char*) "tpgetrply return: " << toReturn
			<< " tperrno: " << tperrno);
	return toReturn;
}

int tpcancel(int id) {
	LOG4CXX_TRACE(loggerXATMI, (char*) "tpcancel " << id);
	setSpecific(TPE_KEY, dTPERESET);
	int toReturn = -1;
	if (clientinit() != -1) {
		void* currentImpl = getSpecific(TSS_KEY);
		if (currentImpl) {
			setSpecific(TPE_KEY, dTPETRAN);
		}
		if (ptrAtmiBrokerClient->getSession(id) != NULL) {
			ptrAtmiBrokerClient->closeSession(id);
			LOG4CXX_TRACE(loggerXATMI, (char*) "tpcancel session closed");
			toReturn = 0;
		} else {
			setSpecific(TPE_KEY, dTPEBADDESC);
		}
	}
	LOG4CXX_TRACE(loggerXATMI, (char*) "tpcancel return: " << toReturn
			<< " tperrno: " << tperrno);
	return toReturn;
}

int tpsend(int id, char* idata, long ilen, long flags, long *revent) {
	LOG4CXX_TRACE(loggerXATMI, (char*) "tpsend " << id);
	setSpecific(TPE_KEY, dTPERESET);
	int toReturn = -1;
	Session* session = (Session*) getSpecific(SVC_SES);
	int len = -1;
	if (session != NULL) {
		if (session->getId() != id) {
			session = NULL;
		} else {
			len = ilen;
		}
	}
	if (session == NULL) {
		if (clientinit() != -1) {
			session = ptrAtmiBrokerClient->getSession(id);
			if (session == NULL) {
				setSpecific(TPE_KEY, dTPEBADDESC);
			} else {
				if (!session->getCanSend()) {
					setSpecific(TPE_KEY, dTPEPROTO);
				} else {
					len = ::bufferSize(idata, ilen);
				}
			}
		}
	}
	if (len != -1) {
		toReturn = ::send(session, session->getReplyTo(), idata, len, id,
				flags, 0, 0);
		if (flags & TPRECVONLY) {
			session->setCanSend(false);
			session->setCanRecv(true);
		}
	}
	LOG4CXX_TRACE(loggerXATMI, (char*) "tpsend return: " << toReturn
			<< " tperrno: " << tperrno);
	return toReturn;
}

int tprecv(int id, char ** odata, long *olen, long flags, long* event) {
	LOG4CXX_TRACE(loggerXATMI, (char*) "tprecv " << id);
	setSpecific(TPE_KEY, dTPERESET);
	int toReturn = -1;
	Session* session = (Session*) getSpecific(SVC_SES);
	if (session != NULL && session->getId() != id) {
		session = NULL;
	}
	if (session == NULL) {
		if (clientinit() != -1) {
			session = ptrAtmiBrokerClient->getSession(id);
		}
	}
	if (session == NULL) {
		setSpecific(TPE_KEY, dTPEBADDESC);
	} else {
		toReturn = ::receive(session, odata, olen, flags, event);
		if (*event == TPESVCERR) {
			setSpecific(TPE_KEY, dTPEEVENT);
			toReturn = -1;
		} else if (*event == TPESVCFAIL) {
			setSpecific(TPE_KEY, dTPEEVENT);
			toReturn = -1;
		}
	}
	LOG4CXX_TRACE(loggerXATMI, (char*) "tprecv return: " << toReturn
			<< " tperrno: " << tperrno);
	return toReturn;
}

void tpreturn(int rval, long rcode, char* data, long len, long flags) {
	LOG4CXX_TRACE(loggerXATMI, (char*) "tpreturn " << rval);
	setSpecific(TPE_KEY, dTPERESET);
	Session* session = (Session*) getSpecific(SVC_SES);
	if (session != NULL) {
		if (!session->getCanSend()) {
			setSpecific(TPE_KEY, dTPEPROTO);
		} else {
			session->setCanRecv(false);
			if (bufferSize(data, len) == -1) {
				::send(session, "", data, 0, 0, flags, TPFAIL, TPESVCERR);
			} else {
				::send(session, "", data, len, 0, flags, rval, rcode);
			}
			::tpfree(data);
			session->setSendTo(NULL);
			session->setCanSend(false);
		}
	} else {
		setSpecific(TPE_KEY, dTPEPROTO);
	}
	LOG4CXX_TRACE(loggerXATMI, (char*) "tpreturn returning" << " tperrno: "
			<< tperrno);
}

int tpdiscon(int id) {
	LOG4CXX_TRACE(loggerXATMI, (char*) "tpdiscon " << id);
	setSpecific(TPE_KEY, dTPERESET);
	int toReturn = -1;
	if (clientinit() != -1) {
		LOG4CXX_DEBUG(loggerXATMI, (char*) "end - id: %d" << id);
		Session* session = ptrAtmiBrokerClient->getSession(id);
		if (session == NULL) {
			setSpecific(TPE_KEY, dTPEBADDESC);
		} else {
			try {
				void* currentImpl = getSpecific(TSS_KEY);
				if (currentImpl) {
					toReturn = tx_rollback();
				}
				ptrAtmiBrokerClient->closeSession(id);
				LOG4CXX_TRACE(loggerXATMI, (char*) "tpdiscon session closed");
			} catch (...) {
				LOG4CXX_ERROR(
						loggerXATMI,
						(char*) "aCorbaService->start_conversation(): call failed");
				setSpecific(TPE_KEY, dTPESYSTEM);
			}
		}
	}
	LOG4CXX_TRACE(loggerXATMI, (char*) "tpdiscon return: " << toReturn
			<< " tperrno: " << tperrno);
	return toReturn;
}
