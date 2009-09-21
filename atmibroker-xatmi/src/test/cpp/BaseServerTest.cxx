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
#include <cppunit/extensions/HelperMacros.h>
#include "BaseServerTest.h"
#include "BaseTest.h"

extern "C" {
#include "AtmiBrokerServerControl.h"
}

#include "xatmi.h"

void BaseServerTest::setUp() {
	// Perform initial start up
	BaseTest::setUp();

	int result;

#ifdef WIN32
		char* argv1[] = {(char*)"server", (char*)"-c", (char*)"win32"};
#else
		char* argv1[] = {(char*)"server", (char*)"-c", (char*)"linux"};
#endif
	int argc1 = sizeof(argv1)/sizeof(char*);

	result = serverinit(argc1, argv1);
	// Check that there is no error on server setup
	CPPUNIT_ASSERT(result != -1);
	CPPUNIT_ASSERT(tperrno == 0);
}

void BaseServerTest::tearDown() {
	// Stop the server
	serverdone();

	// Perform additional clean up
	BaseTest::tearDown();
}
