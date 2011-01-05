/*
 * JBoss, Home of Professional Open Source
 * Copyright 2009, Red Hat, Inc., and others contributors as indicated
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
#include <string.h>
#include "XAResourceManager.h"
#include "ThreadLocalStorage.h"
#include "ace/OS_NS_time.h"
#include "ace/OS.h"

#include "AtmiBrokerEnv.h"

log4cxx::LoggerPtr xarmlogger(log4cxx::Logger::getLogger("TxLogXAManager"));

SynchronizableObject* XAResourceManager::lock = new SynchronizableObject();
long XAResourceManager::counter = 0l;

ostream& operator<<(ostream &os, const XID& xid)
{
	os << xid.formatID << ':' << xid.gtrid_length << ':' << xid.bqual_length << ':' << xid.data;
	return os;
}

void XAResourceManager::show_branches(const char *msg, XID * xid)
{
	FTRACE(xarmlogger, "ENTER " << *xid);

	branchLock->lock();
	for (XABranchMap::iterator i = branches_.begin(); i != branches_.end(); ++i) {
		LOG4CXX_TRACE(xarmlogger, (char *) "XID: " << (i->first)); 
	}
	branchLock->unlock();
}

static int compareXids(const XID& xid1, const XID& xid2)
{
	char *x1 = (char *) &xid1;
	char *x2 = (char *) &xid2;
	char *e = (char *) (x1 + sizeof (XID));

	while (x1 < e)
		if (*x1 < *x2)
			return -1;
		else if (*x1++ > *x2++)
			return 1;

	return 0;
}

bool xid_cmp::operator()(XID const& xid1, XID const& xid2) const {
	return (compareXids(xid1, xid2) < 0);
}

bool operator<(XID const& xid1, XID const& xid2) {
	return (compareXids(xid1, xid2) < 0);
}

XAResourceManager::XAResourceManager(
	CORBA_CONNECTION* connection,
	const char * name,
	const char * openString,
	const char * closeString,
	CORBA::Long rmid,
	long sid,
	struct xa_switch_t * xa_switch,
	XARecoveryLog& log, PortableServer::POA_ptr poa) throw (RMException) :

	poa_(poa), connection_(connection), name_(name), openString_(openString), closeString_(closeString),
	rmid_(rmid), sid_(sid), xa_switch_(xa_switch), rclog_(log) {

	FTRACE(xarmlogger, "ENTER " << (char *) "new RM name: " << name << (char *) " openinfo: " <<
		openString << (char *) " rmid: " << rmid_);

	if (name == NULL) {
		RMException ex("Invalid RM name", EINVAL);
		throw ex;
	}

	int rv = xa_switch_->xa_open_entry((char *) openString, rmid_, TMNOFLAGS);

	LOG4CXX_TRACE(xarmlogger,  (char *) "xa_open: " << rv);

	if (rv != XA_OK) {
		LOG4CXX_ERROR(xarmlogger,  (char *) "xa_open error: " << rv);

		RMException ex("xa_open", rv);
		throw ex;
	}

	branchLock = new SynchronizableObject();
	// each RM has its own POA
//	createPOA();
}

XAResourceManager::~XAResourceManager() {
	FTRACE(xarmlogger, "ENTER");
	int rv = xa_switch_->xa_close_entry((char *) closeString_, rmid_, TMNOFLAGS);

	LOG4CXX_TRACE(xarmlogger, (char *) "xa_close: " << rv);

	if (rv != XA_OK)
		LOG4CXX_WARN(xarmlogger, (char *) " close RM " << name_ << " failed: " << rv);

	delete branchLock;
}

/**
 * replay branch completion on an XID that needs recovering.
 * The rc parameter is the CORBA object reference of the
 * Recovery Coordinator for the the branch represented by the XID
 *
 * return bool true if it is OK for the caller to delete the associated recovery record
 */
bool XAResourceManager::recover(XID& bid, const char* rc)
{
	bool delRecoveryRec = true;

	FTRACE(xarmlogger, "ENTER");

	CORBA::Object_var ref = connection_->orbRef->string_to_object(rc);
	XAResourceAdaptorImpl *ra = NULL;

	if (CORBA::is_nil(ref)) {
		LOG4CXX_INFO(xarmlogger, (char *) "Invalid recovery coordinator ref: " << rc);
		return delRecoveryRec;
	} else {
		CosTransactions::RecoveryCoordinator_var rcv = CosTransactions::RecoveryCoordinator::_narrow(ref);

		if (CORBA::is_nil(rcv)) {
			LOG4CXX_INFO(xarmlogger, (char *) "Could not narrow recovery coordinator ref: " << rc);
		} else {
			int rv;

			ra = new XAResourceAdaptorImpl(this, bid, bid, rmid_, xa_switch_, rclog_);

			branchLock->lock();
			if (branches_[bid] != NULL) {
				// log an error since we forgot to clean up the previous servant
				LOG4CXX_ERROR(xarmlogger, (char *) "Recovery: branch already exists: " << bid);
			}
			branchLock->unlock();

			// activate the servant
			std::string s = (char *) (bid.data + bid.gtrid_length);
			PortableServer::ObjectId_var objId = PortableServer::string_to_ObjectId(s.c_str());
			poa_->activate_object_with_id(objId, ra);
			// get a CORBA reference to the servant so that it can be enlisted in the OTS transaction
			CORBA::Object_var ref = poa_->id_to_reference(objId.in());

			ra->_remove_ref();	// now only the POA has a reference to ra

			CosTransactions::Resource_var v = CosTransactions::Resource::_narrow(ref);

			LOG4CXX_DEBUG(xarmlogger, (char*) "Recovering resource with branch id: " << bid <<
				" and recovery IOR: " << connection_->orbRef->object_to_string(v));

			try {
				/*
				 * Replay phase 2. The spec says we should use the same resource.
				 * If we really do need to use the same one then we need to reconstruct it
				 * with the same reference by setting the PortableServer::USER_ID
				 * policy on the creating POA (and use the same name based on the branch id).
				 *
				 * Remember to deal with heuristics (see section 2-50 of the OMG OTS spec).
				 */
				Status txs = rcv->replay_completion(v);

				LOG4CXX_DEBUG(xarmlogger, (char *) "Recovery: TM reports transaction status: " << txs
					<< " (" << CosTransactions::StatusActive);

				switch (txs) {
				case CosTransactions::StatusUnknown:
					// the TM must have presumed abort and discarded the transaction (? is this always the case)
					// fallthru to next case
				case CosTransactions::StatusNoTransaction:
					// the TM must have presumed abort and discarded the transaction
					// fallthru to next case
				case CosTransactions::StatusRolledBack:
					// the TM has already rolled back the transaction

					// presumed abort - force the branch to rollback
					LOG4CXX_INFO(xarmlogger, (char *) "Recovery: rolling back branch for RM " << xa_switch_->name);
					rv = xa_switch_->xa_rollback_entry(&bid, rmid_, TMNOFLAGS);

					if (rv != XA_OK) {
						// ? under what circumstances is it possible to recover from this error
						LOG4CXX_INFO(xarmlogger, (char *) "Recovery: RM returned XA error " << rv);
					}

					break;
				case CosTransactions::StatusCommitted:
					LOG4CXX_INFO(xarmlogger, (char *) "Recovery: committing branch for RM " << xa_switch_->name);
					rv = xa_switch_->xa_commit_entry(&bid, rmid_, TMNOFLAGS);

					if (rv != XA_OK) {
						// ? under what circumstances is it possible to recover from this error
						LOG4CXX_INFO(xarmlogger, (char *) "Recovery: RM returned XA error " << rv);
					}

					break;
				/*
				 * Note that the BlackTie server should only try to recover XIDs it finds in its prepared log
				 * so the following status codes should never occur:
				 */
				case CosTransactions::StatusActive:
				case CosTransactions::StatusMarkedRollback:
					LOG4CXX_INFO(xarmlogger, (char *) "Recovery: TM returned an unexpected status");
					break;
				/*
				 * The remaining cases imply that the TM will eventually tell us how to complete the branch
				 */
				case CosTransactions::StatusPrepared:
				case CosTransactions::StatusPreparing:
				case CosTransactions::StatusRollingBack:
				case CosTransactions::StatusCommitting:
					// the XAResourceAdapterImpl corresponding to the branch will remove the recovery record
					LOG4CXX_INFO(xarmlogger,
						(char *) "Recovery: replaying transaction (TM reports prepared/preparing or completing)");
					branchLock->lock();
					branches_[bid] = ra;
					branchLock->unlock();
					delRecoveryRec = false;
					break;
				default:
					// shouldn't happend all cases have been dealt with
					break;
				}
			} catch (CosTransactions::NotPrepared& e) {
				LOG4CXX_WARN(xarmlogger, (char *) "Recovery: TM says the transaction as not prepared: " << e._name());
			} catch (const CORBA::SystemException& e) {
				LOG4CXX_WARN(xarmlogger, (char*) "Recovery: replay error: " << e._name() << " minor: " << e.minor());
			}

			if (delRecoveryRec)
				delete ra;
		}
	}

	return delRecoveryRec;
}

/**
 * check whether it is OK to recover a given XID
 */
bool XAResourceManager::isRecoverable(XID &xid)
{
	/*
	 * if the XID is one of ours it will encode the RM id and the server id
	 * in the first two longs of the XID data (see XAResourceManager::gen_xid)
	 */
	char *bdata = (char *) (xid.data + xid.gtrid_length);
	char *sp = strchr(bdata, ':');
	long rmid = ACE_OS::atol(bdata);	// the RM id
	long sid = (sp == 0 || ++sp == 0 ? 0l : ACE_OS::atol(sp));	// the server id

	/*
	 * Only recover our own XIDs - the reason we need to check the server id is to
	 * avoid the possibility of a server rolling back another servers active XID
	 *
	 * Note that this means that a recovery log can only be processed by server
	 * with the correct id.
	 *
	 * The user can override this behaviour, so that any server or client can recover any log,
	 * via an environment variable:
	 */

	if (AtmiBrokerEnv::get_instance()->getenv((char*) "BLACKTIE_RC_LOG_NAME", NULL) != NULL)
		sid = sid_;

	AtmiBrokerEnv::discard_instance();

	if (rmid == rmid_ && sid == sid_) {
		/*
		 * If this XID does not appear in the recovery log then the server must have failed
		 * after calling prepare on the RM but before writing the recovery log in which case
		 * it is OK to recover the XID
		 */
		if (rclog_.find_ior(xid) == 0)
			return true;
	}

	return false;
}

/**
 * Initiate a recovery scan on the RM looking for prepared or heuristically completed branches
 * This functionality covers the following failure scenario:
 * - server calls prepare on a RM
 * - RM prepares but the the server fails before it can write to its transaction recovery log
 * In this case the RM will have a pending transaction branch which does not appear in
 * the recovery log. Calling xa_recover on the RM will return the 'missing' XID which the
 * recovery scan can rollback.
 */
int XAResourceManager::recover()
{
	XID xids[10];
	long count = sizeof (xids) / sizeof (XID);
	long flags = TMSTARTRSCAN;	// position the cursor at the start of the list
	int i, nxids;

	do {
		// ask the RM for all XIDs that need recovering
		nxids = xa_switch_->xa_recover_entry(xids, count, rmid_, flags);

		flags = TMNOFLAGS;	// on the next call continue the scan from the current cursor position

		for (i = 0; i < nxids; i++) {
			// check whether this id needs rolling back
			if (isRecoverable(xids[i])) {
				int rv = xa_switch_->xa_rollback_entry((XID *) (xids + i), rmid_, TMNOFLAGS);

				LOG4CXX_INFO(xarmlogger, (char*) "Recovery of xid " << xids[i] << " for RM " << rmid_ <<
					 " returned XA status " << rv);
			}
		}
	} while (count == nxids);

	return 0;
}

int XAResourceManager::createServant(XID& xid)
{
	FTRACE(xarmlogger, "ENTER");
	int res = XAER_RMFAIL;
	XAResourceAdaptorImpl *ra = NULL;
	CosTransactions::Control_ptr curr = (CosTransactions::Control_ptr) txx_get_control();
	CosTransactions::Coordinator_ptr coord = NULL;

	if (CORBA::is_nil(curr))
		return XAER_NOTA;

	try {
		/*
		 * Generate an XID for the new branch. The XID should have the same global transaction id
		 * that the Transaction Manager generated but should have a different branch qualifier
		 * (since we want to have loose coupling semantics).
		 */
		XID bid = gen_xid(rmid_, sid_, xid);
		// Create a servant to represent the new branch.
		ra = new XAResourceAdaptorImpl(this, xid, bid, rmid_, xa_switch_, rclog_);
#if 0
		// and activate it
		PortableServer::ObjectId_var objId = poa_->activate_object(ra);
		// get a CORBA reference to the servant so that it can be enlisted in the OTS transaction
		CORBA::Object_var ref = poa_->servant_to_reference(ra);
#else
		std::string s = (char *) (bid.data + bid.gtrid_length);
		PortableServer::ObjectId_var objId = PortableServer::string_to_ObjectId(s.c_str());
		poa_->activate_object_with_id(objId, ra);
		// get a CORBA reference to the servant so that it can be enlisted in the OTS transaction
		CORBA::Object_var ref = poa_->id_to_reference(objId.in());
#endif

		ra->_remove_ref();	// now only the POA has a reference to ra

		CosTransactions::Resource_var v = CosTransactions::Resource::_narrow(ref);

		// enlist it with the transaction
		LOG4CXX_TRACE(xarmlogger, (char*) "enlisting resource: "); // << connection_->orbRef->object_to_string(v));
		coord = curr->get_coordinator();
		CosTransactions::RecoveryCoordinator_ptr rc = coord->register_resource(v);
		//c->register_synchronization(new XAResourceSynchronization(xid, rmid_, xa_switch_));

		if (CORBA::is_nil(rc)) {
			LOG4CXX_TRACE(xarmlogger, (char*) "createServant: nill RecoveryCoordinator ");
			res = XAER_RMFAIL;
		} else {
			CORBA::String_var rcref = connection_->orbRef->object_to_string(rc);
			ra->set_recovery_coordinator(ACE_OS::strdup(rcref));
	   		CORBA::release(rc);

			branchLock->lock();
			branches_[xid] = ra;
			branchLock->unlock();
			res = XA_OK;
			ra = NULL;
		}
	} catch (RMException& ex) {
		LOG4CXX_WARN(xarmlogger, (char*) "unable to create resource adaptor for branch: " << ex.what());
	} catch (PortableServer::POA::ServantNotActive&) {
		LOG4CXX_ERROR(xarmlogger, (char*) "createServant: poa inactive");
	} catch (CosTransactions::Inactive&) {
		res = XAER_PROTO;
		LOG4CXX_TRACE(xarmlogger, (char*) "createServant: tx inactive (too late for registration)");
	} catch (const CORBA::SystemException& e) {
		LOG4CXX_WARN(xarmlogger, (char*) "Resource registration error: " << e._name() << " minor: " << e.minor());
	}

	if (ra)
		delete ra;

	if (!CORBA::is_nil(curr))
		CORBA::release(curr);

	if (!CORBA::is_nil(coord))
		CORBA::release(coord);

	return res;
}

void XAResourceManager::notify_error(XID * xid, int xa_error, bool forget)
{
	FTRACE(xarmlogger, "ENTER: reason: " << xa_error);

	if (forget)
		set_complete(xid);
}

void XAResourceManager::set_complete(XID * xid)
{
	FTRACE(xarmlogger, "ENTER");
	XABranchMap::iterator iter;

	LOG4CXX_TRACE(xarmlogger, (char*) "removing branch: " << *xid);

	if (rclog_.del_rec(*xid) != 0) {
		// probably a readonly resource
		LOG4CXX_TRACE(xarmlogger, (char*) "branch completion notification with out a corresponding log entry: " << *xid);
	}

	branchLock->lock();
	for (XABranchMap::iterator i = branches_.begin(); i != branches_.end(); ++i)
	{
		if (compareXids(i->first, (const XID&) (*xid)) == 0) {
			XAResourceAdaptorImpl *r = i->second;
			PortableServer::ObjectId_var id(poa_->servant_to_id(r));

			// Note: deactivate will delete r hence no call to r->_remove_ref();
			poa_->deactivate_object(id.in());
			branches_.erase(i->first);

			branchLock->unlock();
			return;
		}
	}
	branchLock->unlock();

	LOG4CXX_TRACE(xarmlogger, (char*) "... unknown branch");
}

int XAResourceManager::xa_start (XID * xid, long flags)
{
	FTRACE(xarmlogger, "ENTER " << rmid_ << (char *) ": flags=" << std::hex << flags << " lookup XID: " << *xid);
	XAResourceAdaptorImpl * resource = locateBranch(xid);
	int rv;

	if (resource == NULL) {
		FTRACE(xarmlogger, "creating branch " << *xid);
		if ((rv = createServant(*xid)) != XA_OK)
			return rv;

		if ((resource = locateBranch(xid)) == NULL)	// cannot be NULL
			return XAER_RMERR;

		FTRACE(xarmlogger, "starting branch " << *xid);
		return resource->xa_start(TMNOFLAGS);
	}

	FTRACE(xarmlogger, "existing branch " << *xid);
	return resource->xa_start(TMRESUME);
}

int XAResourceManager::xa_end (XID * xid, long flags)
{
	FTRACE(xarmlogger, "ENTER end branch " << *xid << " rmid=" << rmid_ << " flags=" << std::hex << flags);
	XAResourceAdaptorImpl * resource = locateBranch(xid);

	if (resource == NULL) {
		LOG4CXX_WARN(xarmlogger, (char *) " no such branch " << *xid);
		return XAER_NOTA;
	}

	return resource->xa_end(flags);
}

XAResourceAdaptorImpl * XAResourceManager::locateBranch(XID * xid)
{
	FTRACE(xarmlogger, "ENTER");
	XABranchMap::iterator iter;

	branchLock->lock();
	for (iter = branches_.begin(); iter != branches_.end(); ++iter) {
		LOG4CXX_TRACE(xarmlogger, (char *) "compare: " << *xid << " with " << (iter->first));

		if (compareXids(iter->first, (const XID&) (*xid)) == 0) {
			branchLock->unlock();
			return (*iter).second;
		}
	}
	branchLock->unlock();

	LOG4CXX_DEBUG(xarmlogger, (char *) " branch not found");
	return NULL;
}

int XAResourceManager::xa_flags()
{
	return xa_switch_->flags;
}

/**
 * Generate a new XID. The xid should be unique within the currently
 * running process. Uniqueness is assured by including
 *
 * - the global transaction identifier (gid)
 * - the server id (sid)
 * - a counter
 * - the current time
 */
XID XAResourceManager::gen_xid(long id, long sid, XID &gid)
{
	FTRACE(xarmlogger, "ENTER");
	XID xid = {gid.formatID, gid.gtrid_length};
	int i;
	long myCounter = -1l;

	lock->lock();
	myCounter = ++counter;
	lock->unlock();

	for (i = 0; i < gid.gtrid_length; i++)
		xid.data[i] = gid.data[i];

	ACE_Time_Value now = ACE_OS::gettimeofday();
	/*
	 * The first two longs in the XID data should contain the RM id and server id respectively.
	 */
	(void) sprintf(xid.data + i, "%ld:%ld:%ld:%ld:%ld", id, sid, myCounter, (long) now.sec(), (long) now.usec());
	xid.bqual_length = strlen(xid.data + i);

	FTRACE(xarmlogger, "Leaving with XID: " << xid);
	return xid;
}
