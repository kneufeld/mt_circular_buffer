
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
    : m_total_read(0), m_total_written(0)
    {
        buffer.set_capacity( n );
    }

    size_t total_read()     const { scoped_lock lock( monitor ); return m_total_read; }
    size_t total_written()  const { scoped_lock lock( monitor ); return m_total_written; }

    size_t write( const char* data, size_t count )
    {
        return write( reinterpret_cast<const byte*>(data), count );
    }

    size_t write( const byte* data, size_t count )
    {
        size_t bytes_written = 0;

        while( bytes_written < count )
        {
            scoped_lock lock( monitor );

            while( buffer.full() )
            {
                buffer_not_full.wait( lock );
            }

            size_t to_write = ( std::min )( count - bytes_written, remaining() );

            _write( data + bytes_written, to_write ) ;
            bytes_written += to_write;
        }

        //assert( bytes_written == count );
        return bytes_written;
    }

    size_t read( char* data, size_t count )
    {
        return read( reinterpret_cast<byte*>(data), count );
    }

    size_t read( byte* data, size_t count )
    {
        size_t bytes_read = 0;

        while( bytes_read < count )
        {
            scoped_lock lock( monitor );

            while( buffer.empty() )
            {
                buffer_not_empty.wait( lock );
            }

            size_t to_read = ( std::min )( count - bytes_read, buffer.size() );

            _read( data + bytes_read, to_read ) ;
            bytes_read += to_read;
        }

        //assert( bytes_read == count );
        return bytes_read;
    }

    size_t skip( size_t count )
    {
        std::vector<byte> scrach(count);
        size_t bytes_read = read( &scrach[0], count );
        //assert( bytes_read == count );
        return bytes_read;
    }

    void clear()
    {
        scoped_lock lock( monitor );
        buffer.clear();
    }

    // how many bytes are currently in buffer
    size_t size() const
    {
        scoped_lock lock( monitor );
        return buffer.size();
    }

    // how many bytes could the buffer hold
    size_t capacity() const
    {
        scoped_lock lock( monitor );
        return buffer.capacity();
    }

    bool empty() const
    {
        scoped_lock lock( monitor );
        return buffer.empty();
    }

    bool full() const
    {
        scoped_lock lock( monitor );
        return buffer.full();
    }

    void set_capacity( int capacity )
    {
        scoped_lock lock( monitor );
        buffer.set_capacity( capacity );
    }

private:

    friend class mt_circular_buffer_tests;

    size_t _write( const byte* data, size_t count )
    {
        m_total_written += count;
        buffer.insert( buffer.end(), data, data + count );
        buffer_not_empty.notify_one();
        return count;
    }

    size_t _read( byte* data, size_t count )
    {
        m_total_read += count;
        std::copy( buffer.begin(), buffer.begin() + count, data );
        buffer.erase_begin( count );
        buffer_not_full.notify_one();
        return count;
    }

    size_t remaining() const
    {
        return buffer.capacity() - buffer.size();
    }

    typedef boost::mutex::scoped_lock       scoped_lock;
    boost::condition                buffer_not_empty;
    boost::condition                buffer_not_full;
    mutable boost::mutex            monitor;
    boost::circular_buffer<byte>    buffer;

    size_t m_total_read;
    size_t m_total_written;
};

