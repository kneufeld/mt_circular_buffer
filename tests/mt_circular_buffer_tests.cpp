#include <future>
#include <iostream>

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestRunner.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/BriefTestProgressListener.h>
#include <cppunit/CompilerOutputter.h>

using namespace std;

#include "mt_circular_buffer.h"

class mt_circular_buffer_tests : public CPPUNIT_NS::TestFixture
{
public:

    mt_circular_buffer::pointer cb;

    void setUp()
    {
        cb.reset( new mt_circular_buffer( 4 ) );
        CPPUNIT_ASSERT( cb );
    }

    void tearDown()
    {
        cb.reset();
    }

    void test_size()
    {
        CPPUNIT_ASSERT_EQUAL( 4, cb->capacity() );
        CPPUNIT_ASSERT_EQUAL( 0, cb->size() );

        CPPUNIT_ASSERT_EQUAL( true, cb->empty() );
        CPPUNIT_ASSERT_EQUAL( false, cb->full() );
    }

    void test_writing()
    {
        char b = 0;

        cb->write( &b, 1 );
        CPPUNIT_ASSERT_EQUAL( 1, cb->size() );

        cb->write( &b, 1 );
        CPPUNIT_ASSERT_EQUAL( 2, cb->size() );

        cb->write( &b, 1 );
        CPPUNIT_ASSERT_EQUAL( 3, cb->size() );

        cb->write( &b, 1 );
        CPPUNIT_ASSERT_EQUAL( 4, cb->size() );

        CPPUNIT_ASSERT_EQUAL( true, cb->full() );

        CPPUNIT_ASSERT_EQUAL( 4, cb->capacity() );
    }

    void test_writing2()
    {
        short b = 0;

        cb->write( ( char* )&b, sizeof( b ) );
        CPPUNIT_ASSERT_EQUAL( 2, cb->size() );

        cb->write( ( char* )&b, sizeof( b ) );
        CPPUNIT_ASSERT_EQUAL( 4, cb->size() );

        CPPUNIT_ASSERT_EQUAL( true, cb->full() );
    }

    void test_writing3()
    {
        short b = 0;

        cb->write( ( char* )&b, sizeof( b ) );
        CPPUNIT_ASSERT_EQUAL( 2, cb->size() );

        b = 1;
        cb->write( ( char* )&b, sizeof( b ) );
        CPPUNIT_ASSERT_EQUAL( 4, cb->size() );

        auto async_reader = [this]()
        {
            short s = 10;
            cb->read( ( char* )&s, sizeof( s ) );
            CPPUNIT_ASSERT_EQUAL( short( 0 ), s );
        };

        std::future<void> reader = std::async( std::launch::async, async_reader );

        b = 2;
        cb->write( ( char* )&b, sizeof( b ) );
        CPPUNIT_ASSERT_EQUAL( 4, cb->size() );

        cb->read( ( char* )&b, sizeof( b ) );
        CPPUNIT_ASSERT_EQUAL( short( 1 ), b );

        cb->read( ( char* )&b, sizeof( b ) );
        CPPUNIT_ASSERT_EQUAL( short( 2 ), b );

        CPPUNIT_ASSERT_EQUAL( true, cb->empty() );
    }

    void test_writing4()
    {
        std::string input("this is a really long string");
        char output[256];

        auto async_reader = [&]()
        {
            cb->read( output, input.size() );
        };

        std::future<void> reader = std::async( std::launch::async, async_reader );

        cb->write( input.data(), input.size() );

        CPPUNIT_ASSERT( input == output );
    }

    void test_reading()
    {
        char b = 1;
        cb->write( &b, 1 );

        b = 0;
        cb->read( &b, 1 );

        CPPUNIT_ASSERT_EQUAL( ( char )1, b );
        CPPUNIT_ASSERT_EQUAL( 0, cb->size() );
        CPPUNIT_ASSERT_EQUAL( true, cb->empty() );
    }

    void test_reading2()
    {
        auto async_writer = [this]()
        {
            short s = 'x' << 8 | 'y';
            cb->write( ( char* )&s, sizeof( s ) );
        };

        std::future<void> writer = std::async( std::launch::async, async_writer );

        char b = 'z';

        cb->read( &b, 1 );
        CPPUNIT_ASSERT_EQUAL( 'y', b );

        cb->read( &b, 1 );
        CPPUNIT_ASSERT_EQUAL( 'x', b );

        CPPUNIT_ASSERT_EQUAL( 0, cb->size() );
        CPPUNIT_ASSERT_EQUAL( true, cb->empty() );
    }

    void test_reading3()
    {
        std::string input("this is a really long string");
        char output[256];
        output[ input.size() ] = 0; // we can either null terminate this string or read/write +1

        auto async_writer = [&]()
        {
            cb->write( input.data(), input.size() );
        };

        std::future<void> writer = std::async( std::launch::async, async_writer );

        cb->read( output, input.size() );

        //cout << std::endl << input << std::endl << output << std::endl;
        CPPUNIT_ASSERT( input == output );
    }

    CPPUNIT_TEST_SUITE( mt_circular_buffer_tests );
    CPPUNIT_TEST( test_size );
    CPPUNIT_TEST( test_writing );
    CPPUNIT_TEST( test_writing2 );
    CPPUNIT_TEST( test_writing3 );
    CPPUNIT_TEST( test_writing4 );
    CPPUNIT_TEST( test_reading );
    CPPUNIT_TEST( test_reading2 );
    CPPUNIT_TEST( test_reading3 );
    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION( mt_circular_buffer_tests );

int main( int argc, char** argv )
{
    // informs test-listener about testresults
    CppUnit::TestResult controller;

    // register listener for collecting the test-results
    CppUnit::TestResultCollector result;
    controller.addListener( &result );

    // register listener for per-test progress output
    CppUnit::BriefTestProgressListener progress;
    controller.addListener( &progress );

    // insert test-suite at test-runner by registry
    CppUnit::TestRunner testrunner;
    testrunner.addTest( CppUnit::TestFactoryRegistry::getRegistry().makeTest() );
    testrunner.run( controller );

    // output results in compiler-format
    CppUnit::CompilerOutputter compileroutputter( &result, std::cerr );
    compileroutputter.write();
}


