#include <boost/thread/condition.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/asio.hpp>

// Thread safe circular buffer
class mt_circular_buffer : private boost::noncopyable
{
public:

    typedef boost::shared_ptr<mt_circular_buffer> pointer;

    mt_circular_buffer()            { buffer.set_capacity( 1024 ); }
    mt_circular_buffer( int n )     { buffer.set_capacity( n ); }

    void write( const char* data, size_t count )
    {
        {
            // this redundant looking scope is not redundant
            // if count >= remaining() then we'll never release the lock. ever.
            scoped_lock lock( monitor );

            if( count < remaining() )
            {
                _write( data, count );
                return;
            }
        }

        size_t written = 0;

        while( written < count )
        {
            scoped_lock lock( monitor );
            size_t to_write = remaining();

            _write( data + written, to_write ) ;
            written += to_write;
        }
    }

    void read( char* data, size_t count )
    {
        {
            scoped_lock lock( monitor );

            if( count < buffer.size() )
            {
                _read( data, count );
                return;
            }
        }

        size_t bytes_read = 0;

        while( bytes_read < count )
        {
            scoped_lock lock( monitor );

            while( buffer.empty() )
            {
                buffer_not_empty.wait( lock );
            }

            size_t to_read = ( min )( count - bytes_read, buffer.size() );

            _read( data + bytes_read, to_read ) ;
            bytes_read += to_read;
        }
    }

    void clear()
    {
        scoped_lock lock( monitor );
        buffer.clear();
    }

    // how many bytes are currently in buffer
    int size()
    {
        scoped_lock lock( monitor );
        return buffer.size();
    }

    // how many bytes could the buffer hold
    int capacity()
    {
        scoped_lock lock( monitor );
        return buffer.capacity();
    }

    bool empty()
    {
        scoped_lock lock( monitor );
        return buffer.empty();
    }

    bool full()
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

    typedef boost::mutex::scoped_lock       scoped_lock;
    typedef boost::asio::const_buffers_1       cbuffer;
    typedef boost::asio::mutable_buffers_1     mbuffer;

    void _write( const char* data, size_t count )
    {
        for( int i = 0; i < count; i++ )
            buffer.push_back( data[i] );

        buffer_not_empty.notify_one();
    }

    void _read( char* data, size_t count )
    {
        for( int i = 0; i < count; i++ )
        {
            data[i] = buffer.front();
            buffer.pop_front();
        }
    }

    size_t remaining() const
    {
        return buffer.capacity() - buffer.size();
    }

    boost::condition                buffer_not_empty;
    boost::mutex                    monitor;
    boost::circular_buffer<char>    buffer;
};

