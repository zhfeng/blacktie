package org.jboss.blacktie.jatmibroker.tx;

import org.jboss.blacktie.jatmibroker.jab.JABException;
import org.jboss.blacktie.jatmibroker.jab.JABSession;
import org.jboss.blacktie.jatmibroker.jab.JABSessionAttributes;
import org.jboss.blacktie.jatmibroker.jab.JABTransaction;
import org.omg.CosNaming.NamingContextPackage.CannotProceed;
import org.omg.CosNaming.NamingContextPackage.InvalidName;
import org.omg.CosNaming.NamingContextPackage.NotFound;
import org.omg.CosTransactions.Status;
import org.omg.CosTransactions.Unavailable;
import org.omg.PortableServer.POAManagerPackage.AdapterInactive;

public class TX {

	public static final int TX_OK = -1;//0;
	public static final int TX_ROLLBACK = -1;//-2;

	public static final int TX_ROLLBACK_ONLY = -1;//= 2;

	public static int tx_begin() {
		return TX_OK;
/*
		try {
			JABSessionAttributes aJabSessionAttributes = new JABSessionAttributes(
					null);
			JABSession aJabSession = new JABSession(aJabSessionAttributes);
			new JABTransaction(aJabSession, 5000);
			return TX_OK;
		} catch (Throwable t) {
			return -1;
		}
*/
	}

	public static int tx_open() {
		return TX_OK;
	}

	public static int tx_info(TXINFO txinfo) {
		txinfo.transaction_state = TX_ROLLBACK_ONLY;
		return TX_OK;
/*
		try {
			Status status = JABTransaction.current().getStatus();
			if (status.value() == Status._StatusMarkedRollback) {
				txinfo.transaction_state = TX_ROLLBACK_ONLY;
			} else {
				txinfo.transaction_state = -1;
			}
			return TX_OK;
		} catch (Throwable t) {
			txinfo.transaction_state = -1;
			return -1;
		}
*/
	}

	public static int tx_commit() {
		return TX_ROLLBACK;
/*
		try {
			JABTransaction.current().commit();
			return TX_OK;
		} catch (JABException e) {
			return TX_ROLLBACK;
		}
*/
	}
}