#pragma once

#include <string>

#include "cppunit/extensions/HelperMacros.h"

#include "circular_buffer.h"

class circular_tests : public CPPUNIT_NS::TestFixture
{
	CPPUNIT_TEST_SUITE( circular_tests );
	CPPUNIT_TEST( Creation );
	CPPUNIT_TEST( Writing );
	CPPUNIT_TEST( Reading );
	CPPUNIT_TEST_SUITE_END();

public:

	circular_tests();
	~circular_tests();

	void setUp();		// Setup initial condition before each test run
	void tearDown();	// Cleanup data after each test run

private:

	void Creation();
	void Writing();
	void Reading();

    mt_circular_buffer::pointer cb;
};
