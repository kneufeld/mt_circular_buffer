
#include <algorithm>

#include <boost/thread/condition.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/circular_buffer.hpp>

#if 0
#define logging std::cout
#else
#define logging while(false) std::cout
// don't use if(false) in case 'logging' is part of an if statement
// if( foo )
//   logging << "some string";
// else // this else would belong to logging's if(false)
//   return;
#endif

// Thread safe circular buffer
class mt_circular_buffer : private boost::noncopyable
{
public:

    typedef boost::shared_ptr<mt_circular_buffer> pointer;
    typedef unsigned char byte;

    mt_circular_buffer( int n = 1024 )
        : m_closed( false ), m_written( false ), m_total_read( 0 ), m_total_written( 0 )
    {
        m_buffer.set_capacity( n );
    }

    // set the capactity of the buffer in bytes
    void set_capacity( int capacity )
    {
        scoped_lock lock( m_monitor );

        bool growing = capacity > m_buffer.size();

        m_buffer.set_capacity( capacity );

        if( growing )
        {
            // let any blocked writers know that they have room to write
            m_read_event.notify_one(); 
        }
    }

    // close the buffer to future writes
    // this function is very handy if you're using the mt_circular_buffer as a stream
    // allows a read to return before it receives all of it's requested bytes
    void close()
    {
        scoped_lock lock( m_monitor );
        logging << "closing circular buffer" << std::endl;

        m_closed = true;
        m_written = true;      // unblock wait_for_write

        // wake up any reads that might be in progress so that it can return
        m_write_event.notify_all();
    }

    bool closed() const
    {
        return m_closed;
    }

    // return counter of how many bytes we've read (includes skipped bytes)
    size_t total_read() const
    {
        scoped_lock lock( m_monitor );
        return m_total_read;
    }

    // return counter of how many bytes we've written
    size_t total_written() const
    {
        scoped_lock lock( m_monitor );
        return m_total_written;
    }

    // wait for a write to happen without removing any bytes from the buffer
    // only one thread should be waiting or reading
    void wait_for_write()
    {
        scoped_lock lock( m_monitor );

        // there is no race condition because m_written can only be checked if we have the mutex
        // note this is a while loop and not an if because wait() can return spuriously. seriously.
        // for more info see: 
        //   http://stackoverflow.com/questions/8594591/why-does-pthread-cond-wait-have-spurious-wakeups
        while( ! m_written )
        {
            m_write_event.wait( lock );
        }

        // have the looging outside the above loop because m_written may be true by the time we get here
        logging << "wait_for_write signalled" << std::endl;

        // in case another thread is waiting, notify it
        m_write_event.notify_one();
    }

    // helper function so caller doesn't always have to cast
    // caller is responsible for any and all problems from such a dangerous cast
    template<typename T>
    size_t write( const T* data, size_t count )
    {
        return write( reinterpret_cast<const byte*>( data ), count );
    }

    // write into the buffer, this will block until the bytes have been written
    // this method handles all the locking
    size_t write( const byte* data, size_t count )
    {
        size_t bytes_written = 0;

        while( bytes_written < count )
        {
            scoped_lock lock( m_monitor );

            if( m_closed ) // only check once we have the mutex
            {
                throw std::runtime_error( "trying to write to a closed buffer" );
            }

            while( m_buffer.full() )
            {
                logging << "writer waiting" << std::endl;
                m_read_event.wait( lock );
                logging << "writer waking" << std::endl;
            }

            size_t to_write = ( std::min )( count - bytes_written, remaining() );
            bytes_written += _write( data + bytes_written, to_write );
        }

        //assert( bytes_written == count );
        return bytes_written;
    }

    // helper function so caller doesn't always have to cast
    // caller is responsible for any and all problems from such a dangerous cast
    template<typename T>
    size_t read( T* data, size_t count )
    {
        return read( reinterpret_cast<byte*>( data ), count );
    }

    // read from the buffer, this will block until the bytes have been read or the buffer is closed
    // this method handles all the locking
    size_t read( byte* data, size_t count )
    {
        size_t bytes_read = 0;

        while( bytes_read < count )
        {
            scoped_lock lock( m_monitor );

            // we may have closed and signalled m_write_event but we weren't waiting on it yet
            // therefore, only wait if we're empty and we're not closed
            while( m_buffer.empty() && ! m_closed )
            {
                logging << "reader waiting" << std::endl;
                m_write_event.wait( lock );
                logging << "reader waking" << std::endl;
            }

            size_t to_read = ( std::min )( count - bytes_read, m_buffer.size() );
            bytes_read += _read( data + bytes_read, to_read );

            // don't break before reading any remaining bytes
            // as the caller probably wants them
            if( m_closed ) { break; }
        }

        return bytes_read;
    }

    // throw way the first n bytes of the buffer
    size_t skip( size_t count )
    {
        std::vector<byte> scrach( count );
        return read( &scrach[0], count );
    }

    // delete contents of buffer
    void clear()
    {
        scoped_lock lock( m_monitor );
        m_buffer.clear();
    }

    // how many bytes are currently in the buffer
    size_t size() const
    {
        scoped_lock lock( m_monitor );
        return m_buffer.size();
    }

    // how many bytes could the buffer hold
    size_t capacity() const
    {
        scoped_lock lock( m_monitor );
        return m_buffer.capacity();
    }

    // is the buffer empty
    bool empty() const
    {
        scoped_lock lock( m_monitor );
        return m_buffer.empty();
    }

    // is the buffer full
    bool full() const
    {
        scoped_lock lock( m_monitor );
        return m_buffer.full();
    }


private:

    // this method does the actual writing to the internal circular buffer
    size_t _write( const byte* data, size_t count )
    {
        logging <<"_write: " << count << std::endl;

        m_written = true;
        m_total_written += count;

        m_buffer.insert( m_buffer.end(), data, data + count );
        m_write_event.notify_all(); // wake up any blocked readers

        return count;
    }

    // this method does the actual reading from the internal circular buffer
    size_t _read( byte* data, size_t count )
    {
        logging << "_read: " << count << std::endl;

        m_total_read += count;

        std::copy( m_buffer.begin(), m_buffer.begin() + count, data );
        m_buffer.erase_begin( count );

        m_read_event.notify_one(); // wake up a blocked writer

        return count;
    }

    // how many bytes could we write before blocking
    size_t remaining() const
    {
        return m_buffer.capacity() - m_buffer.size();
    }

    friend class mt_circular_buffer_tests;
    typedef boost::mutex::scoped_lock       scoped_lock;

    boost::circular_buffer<byte>            m_buffer;

    mutable boost::mutex                    m_monitor;
    boost::condition                        m_write_event;  // a write happened
    boost::condition                        m_read_event;   // a read happened
    bool                                    m_closed;
    bool                                    m_written;

    size_t                                  m_total_read;
    size_t                                  m_total_written;
};

