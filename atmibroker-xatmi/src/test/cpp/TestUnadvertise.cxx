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
#include "TestUnadvertise.h"

void TestUnadvertise::setUp() {
	userlogc((char*) "TestUnadvertise::setUp");
	BaseAdminTest::setUp();
}

void TestUnadvertise::tearDown() {
	userlogc((char*) "TestUnadvertise::tearDown");
	BaseAdminTest::tearDown();
}

void TestUnadvertise::testAdminService() {
	int cd;

	// should not unadvertise ADMIN service by itself
	cd = callADMIN((char*)"unadvertise,default_ADMIN_1", '0', 0, NULL);
	CPPUNIT_ASSERT(cd == 0);
}

void TestUnadvertise::testUnknowService() {
	int   cd;

	cd = callADMIN((char*)"unadvertise,UNKNOW,", '0', 0, NULL);
	CPPUNIT_ASSERT(cd == 0);
}

void TestUnadvertise::testUnadvertise() {
	int   cd;

	userlogc((char*) "tpcall BAR before unadvertise");
	cd = callBAR(0);
	CPPUNIT_ASSERT(cd == 0);
	
	cd = callADMIN((char*)"unadvertise,BAR,", '1', 0, NULL);
	CPPUNIT_ASSERT(cd == 0);

	userlogc((char*) "tpcall BAR after unadvertise");
	cd = callBAR(TPENOENT);
	CPPUNIT_ASSERT(cd != 0);
}
