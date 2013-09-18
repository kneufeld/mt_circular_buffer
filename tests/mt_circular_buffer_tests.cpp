#include <future>
#include <iostream>

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestAssert.h>
#include <cppunit/TestRunner.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/BriefTestProgressListener.h>
#include <cppunit/CompilerOutputter.h>


#include "mt_circular_buffer.h"

using namespace std;

class mt_circular_buffer_tests : public CPPUNIT_NS::TestFixture
{
public:

    mt_circular_buffer::pointer cb;
    typedef mt_circular_buffer::byte byte;

    void setUp()
    {
        cb.reset( new mt_circular_buffer( 4 ) );
        CPPUNIT_ASSERT( cb );
        logging << endl;
    }

    void tearDown()
    {
        cb.reset();
    }

    void test_size()
    {
        CPPUNIT_ASSERT_EQUAL( ( size_t )4, cb->capacity() );
        CPPUNIT_ASSERT_EQUAL( ( size_t )0, cb->size() );

        CPPUNIT_ASSERT_EQUAL( size_t( cb->m_buffer.end() - cb->m_buffer.begin() ), cb->size() );
        CPPUNIT_ASSERT_EQUAL( true, cb->empty() );
        CPPUNIT_ASSERT_EQUAL( false, cb->full() );
    }

    void test_clear()
    {
        byte b = 0;
        cb->write( &b, 1 );
        CPPUNIT_ASSERT_EQUAL( ( size_t )1, cb->size() );

        cb->clear();

        CPPUNIT_ASSERT_EQUAL( true, cb->empty() );
        CPPUNIT_ASSERT_EQUAL( ( size_t )0, cb->size() );
    }

    void test_capacity1()
    {
        CPPUNIT_ASSERT_EQUAL( ( size_t )4, cb->capacity() );

        cb.reset( new mt_circular_buffer( 10 ) );
        CPPUNIT_ASSERT_EQUAL( ( size_t )10, cb->capacity() );
        
        cb->set_capacity( 5 );
        CPPUNIT_ASSERT_EQUAL( ( size_t )5, cb->capacity() );
    }

    void test_capacity2()
    {
        CPPUNIT_ASSERT_EQUAL( ( size_t )0, cb->size() );

        std::string input( "1234" );
        cb->write( input.data(), input.size() );

        CPPUNIT_ASSERT_EQUAL( ( size_t )4, cb->size() );

        cb->set_capacity(1); // moves end() of buffer

        char b;
        cb->read(&b,1);

        CPPUNIT_ASSERT_EQUAL( '1', b ); // note: not 4
        CPPUNIT_ASSERT_EQUAL( ( size_t )0, cb->size() );
        CPPUNIT_ASSERT_EQUAL( ( size_t )1, cb->capacity() );
    }

    void test_capacity3()
    {
        std::string input( "123456" );

        auto async_writer = [&]()
        {
            cb->write( input.data(), input.size() );
        };

        std::future<void> writer = std::async( std::launch::async, async_writer );

        cb->wait_for_write(); // don't set the new capacity until the writer is blocked
        cb->set_capacity( input.size() );

        writer.get();
        CPPUNIT_ASSERT( "we didn't deadlock" );
    }

    void test_close1()
    {
        // during development, read() would deadlock if close() tried to get the lock
        // this test would show it

        std::string input( "12345" );
        int n = input.size();
        char output[256];
        output[ n ] = 0; // we can either null terminate this string or read/write +1

        auto async_reader = [&]()
        {
            size_t inc = 3;
            cb->read( output, inc );
            CPPUNIT_ASSERT_EQUAL( (size_t)(n-inc), cb->read( output+inc, 10 ) );
        };

        std::future<void> reader = std::async( std::launch::async, async_reader );

        cb->write( input.data(), n );
        cb->close();
        reader.get();

        logging << endl << input << endl << output << endl;
        CPPUNIT_ASSERT( input == output );
    }

    void test_close2()
    {
        // during development, read() would deadlock if close() tried to get the lock
        // this test always passed because write() never blocked

        cb.reset( new mt_circular_buffer( 10 ) );
        test_close1();
    }

    void test_close3()
    {
        cb->close();

        byte b = 0;
        CPPUNIT_ASSERT_EQUAL( ( size_t )0, cb->read(&b,1) );

        CPPUNIT_ASSERT_THROW( cb->write(&b,1), std::runtime_error );
    }

    void test_wait1()
    {
        auto async_writer = [&]()
        {
            char b = 0;
            cb->write( &b, 1 );
        };

        std::future<void> writer = std::async( std::launch::async, async_writer );

        cb->wait_for_write();
        CPPUNIT_ASSERT( "we didn't deadlock" ); // if we made it this far then we haven't deadlocked
    }

    void test_wait2()
    {
        auto async_writer = [&]()
        {
            char b = 1;
            cb->write( &b, 1 );
        };

        auto async_reader = [&]()
        {
            char b = 0;
            cb->read( ( byte* )&b, sizeof( b ) );
            CPPUNIT_ASSERT_EQUAL( char( 1 ), b );
        };

        std::future<void> writer = std::async( std::launch::async, async_writer );
        std::future<void> reader = std::async( std::launch::async, async_reader );

        cb->wait_for_write();

        CPPUNIT_ASSERT( "we didn't deadlock" ); // if we made it this far then we haven't deadlocked
    }

    void test_wait3()
    {
        cb->close();    
        cb->wait_for_write();
        CPPUNIT_ASSERT( "we didn't deadlock" );
    }

    void test_writing1()
    {
        byte b = 0;

        cb->write( &b, 1 );
        CPPUNIT_ASSERT_EQUAL( ( size_t )1, cb->size() );
        CPPUNIT_ASSERT_EQUAL( size_t( cb->m_buffer.end() - cb->m_buffer.begin() ), cb->size() );

        cb->write( &b, 1 );
        CPPUNIT_ASSERT_EQUAL( ( size_t )2, cb->size() );

        cb->write( &b, 1 );
        CPPUNIT_ASSERT_EQUAL( ( size_t )3, cb->size() );

        cb->write( &b, 1 );
        CPPUNIT_ASSERT_EQUAL( ( size_t )4, cb->size() );

        CPPUNIT_ASSERT_EQUAL( true, cb->full() );
        CPPUNIT_ASSERT_EQUAL( ( size_t )4, cb->capacity() );
    }

    void test_writing2()
    {
        short b = 0;

        cb->write( ( byte* )&b, sizeof( b ) );
        CPPUNIT_ASSERT_EQUAL( ( size_t )2, cb->size() );

        cb->write( ( byte* )&b, sizeof( b ) );
        CPPUNIT_ASSERT_EQUAL( ( size_t )4, cb->size() );

        CPPUNIT_ASSERT_EQUAL( true, cb->full() );
    }

    void test_writing3()
    {
        short b = 0;

        cb->write( ( byte* )&b, sizeof( b ) );
        CPPUNIT_ASSERT_EQUAL( ( size_t )2, cb->size() );

        b = 1;
        cb->write( ( byte* )&b, sizeof( b ) );
        CPPUNIT_ASSERT_EQUAL( ( size_t )4, cb->size() );

        auto async_reader = [this]()
        {
            short s = 10;
            cb->read( ( byte* )&s, sizeof( s ) );
            CPPUNIT_ASSERT_EQUAL( short( 0 ), s );
        };

        std::future<void> reader = std::async( std::launch::async, async_reader );

        b = 2;
        cb->write( ( byte* )&b, sizeof( b ) );
        CPPUNIT_ASSERT_EQUAL( ( size_t )4, cb->size() );

        cb->read( ( byte* )&b, sizeof( b ) );
        CPPUNIT_ASSERT_EQUAL( short( 1 ), b );

        cb->read( ( byte* )&b, sizeof( b ) );
        CPPUNIT_ASSERT_EQUAL( short( 2 ), b );

        CPPUNIT_ASSERT_EQUAL( true, cb->empty() );
    }

    void test_writing4()
    {
        // the following 1 byte buffer also passes this test
        // cb.reset( new mt_circular_buffer( 1 ) );

        std::string input( "this is a really long string" );
        char output[256];
        output[ input.size() ] = 0; // we can either null terminate this string or read/write +1

        auto async_reader = [&]()
        {
            cb->read( output, input.size() );
        };

        std::future<void> reader = std::async( std::launch::async, async_reader );

        cb->write( input.data(), input.size() );
        reader.get();

        logging << endl << input << endl << output << endl;
        CPPUNIT_ASSERT( input == output );
    }

    void test_writing5()
    {
        // the following 1 byte buffer also passes this test
        cb.reset( new mt_circular_buffer( 500 ) );

        std::string input( "12345678" );
        char output[256];
        output[ 8 ] = 0; // we can either null terminate this string or read/write +1

        auto async_reader = [&]()
        {
            cb->read( output, 4 );
            cb->read( output + 4, 4 );
        };

        cb->write( input.data(), 6 );

        std::future<void> reader = std::async( std::launch::async, async_reader );

        cb->write( input.data() + 6, 2 );

        reader.get();
        cb->write( input.data(), 8 );

        logging << endl << input << endl << output << endl;
        CPPUNIT_ASSERT( input == output );
    }

    void test_reading1()
    {
        byte b = 1;
        cb->write( &b, 1 );

        b = 0;
        cb->read( &b, 1 );

        CPPUNIT_ASSERT_EQUAL( ( byte )1, b );
        CPPUNIT_ASSERT_EQUAL( ( size_t )0, cb->size() );
        CPPUNIT_ASSERT_EQUAL( true, cb->empty() );
    }

    void test_reading2()
    {
        auto async_writer = [this]()
        {
            short s = 'x' << 8 | 'y';
            cb->write( ( byte* )&s, sizeof( s ) );
        };

        std::future<void> writer = std::async( std::launch::async, async_writer );

        char b = 'z';

        cb->read( &b, 1 );
        CPPUNIT_ASSERT_EQUAL( 'y', b );

        cb->read( &b, 1 );
        CPPUNIT_ASSERT_EQUAL( 'x', b );

        CPPUNIT_ASSERT_EQUAL( ( size_t )0, cb->size() );
        CPPUNIT_ASSERT_EQUAL( true, cb->empty() );
    }

    void test_reading3()
    {
        // the following 1 byte buffer also passes this test
        // cb.reset( new mt_circular_buffer( 1 ) );

        std::string input( "this is a really long string" );
        char output[256];
        output[ input.size() ] = 0; // we can either null terminate this string or read/write +1

        auto async_writer = [&]()
        {
            cb->write( input.data(), input.size() );
        };

        std::future<void> writer = std::async( std::launch::async, async_writer );

        cb->read( output, input.size() );
        writer.get();

        logging << std::endl << input << std::endl << output << std::endl;
        CPPUNIT_ASSERT( input == output );
    }

    void test_skip1()
    {
        std::string input( "12" );
        cb->write( input.data(), input.size() );
        
        cb->skip(1);

        char b;
        cb->read( &b, 1 );

        CPPUNIT_ASSERT_EQUAL( '2', b );
    }

    void test_skip2()
    {
        std::string input( "123456" );

        auto async_writer = [&]()
        {
            cb->write( input.data(), input.size() );
        };

        std::future<void> writer = std::async( std::launch::async, async_writer );
        
        cb->skip(5);

        char b;
        cb->read( &b, 1 );

        CPPUNIT_ASSERT_EQUAL( '6', b );
    }

    CPPUNIT_TEST_SUITE( mt_circular_buffer_tests );
    CPPUNIT_TEST( test_size );
    CPPUNIT_TEST( test_clear );
    CPPUNIT_TEST( test_capacity1 );
    CPPUNIT_TEST( test_capacity2 );
    CPPUNIT_TEST( test_capacity3 );
    CPPUNIT_TEST( test_close1 );
    CPPUNIT_TEST( test_close2 );
    CPPUNIT_TEST( test_close3 );
    CPPUNIT_TEST( test_wait1 );
    CPPUNIT_TEST( test_wait2 );
    CPPUNIT_TEST( test_wait3 );
    CPPUNIT_TEST( test_writing1 );
    CPPUNIT_TEST( test_writing2 );
    CPPUNIT_TEST( test_writing3 );
    CPPUNIT_TEST( test_writing4 );
    CPPUNIT_TEST( test_writing5 );
    CPPUNIT_TEST( test_reading1 );
    CPPUNIT_TEST( test_reading2 );
    CPPUNIT_TEST( test_reading3 );
    CPPUNIT_TEST( test_skip1 );
    CPPUNIT_TEST( test_skip2 );
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

    if( argc > 1 )
    {
        testrunner.run( controller, argv[1] );
    }
    else
    {
        testrunner.run( controller, "" );
    }

    // output results in compiler-format
    CppUnit::CompilerOutputter compileroutputter( &result, std::cerr );
    compileroutputter.write();
}


