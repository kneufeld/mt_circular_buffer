
#include <algorithm>

#include <boost/thread/condition.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/circular_buffer.hpp>

// Thread safe circular buffer
class mt_circular_buffer : private boost::noncopyable
{
public:

    typedef boost::shared_ptr<mt_circular_buffer> pointer;
    typedef unsigned char byte;

    mt_circular_buffer( int n = 1024 )
    : m_closed(false), m_total_read(0), m_total_written(0)
    {
        m_buffer.set_capacity( n );
    }

    // set the capactity of the buffer in bytes
    void set_capacity( int capacity )
    {
        scoped_lock lock( m_monitor );
        m_buffer.set_capacity( capacity );
    }

    // close the buffer to future writes
    // this function is very handy if you're using the mt_circular_buffer as a stream
    // allows a read to return before it receives all of it's requested bytes
    void close()
    {
        // std::cout << "buffer closing" << std::endl;
        scoped_lock lock( m_monitor );
        m_closed = true;

        // wake up any read that might be in progress so that it can return
        m_buffer_not_empty.notify_all();
    }

    // return counter of how many bytes we've read (includes skipped bytes)
    size_t total_read() const
    {
        scoped_lock lock( m_monitor );
        return m_total_read;
    }

    // return counter of how many bytes we've written
    size_t total_written()  const
    {
        scoped_lock lock( m_monitor );
        return m_total_written;
    }

    // helper function so caller doesn't always have to cast
    // caller is responsible for any and all problems from such a dangerous cast
    template<typename T>
    size_t write( const T* data, size_t count )
    {
        return write( reinterpret_cast<const byte*>(data), count );
    }

    // write into the buffer, this will block until the bytes have been written
    // this method handles all the locking
    size_t write( const byte* data, size_t count )
    {
        size_t bytes_written = 0;
        scoped_lock lock( m_monitor );

        if( m_closed )
        {
            throw std::runtime_error( "trying to write to a closed buffer" );
        }

        while( bytes_written < count )
        {
            if( m_buffer.full() )
            {
                m_buffer_not_full.wait( lock );
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
        return read( reinterpret_cast<byte*>(data), count );
    }

    // read from the buffer, this will block until the bytes have been read or the buffer is closed
    // this method handles all the locking
    size_t read( byte* data, size_t count )
    {
        size_t bytes_read = 0;

        while( bytes_read < count )
        {
            scoped_lock lock( m_monitor );

            if( m_buffer.empty() )
            {
                m_buffer_not_empty.wait( lock );
            }

            size_t to_read = (std::min)( count - bytes_read, m_buffer.size() );
            bytes_read += _read( data + bytes_read, to_read );

            // check after we read and not before to get the remaining
            // bytes, the caller probably wants them
            if( m_closed ) { break; }
        }

        return bytes_read;
    }

    // throw way the first n bytes of the buffer
    size_t skip( size_t count )
    {
        std::vector<byte> scrach(count);
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
        // std::cout <<  std::endl <<"_write: " << count << std::endl;
        m_total_written += count;
        m_buffer.insert( m_buffer.end(), data, data + count );
        m_buffer_not_empty.notify_all(); // wake up a blocked reader
        return count;
    }

    // this method does the actual reading from the internal circular buffer
    size_t _read( byte* data, size_t count )
    {
        // std::cout << std::endl << "_read: " << count << std::endl;
        m_total_read += count;
        std::copy( m_buffer.begin(), m_buffer.begin() + count, data );
        m_buffer.erase_begin( count );
        m_buffer_not_full.notify_all(); // wake up a blocked writer
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

    boost::condition                        m_buffer_not_empty;
    boost::condition                        m_buffer_not_full;
    mutable boost::mutex                    m_monitor;
    bool                                    m_closed;

    size_t                                  m_total_read;
    size_t                                  m_total_written;
};

