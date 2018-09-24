#include "freertos/queue.h"
#include <type_traits>
#include <optional>

namespace frtos {

template< typename T >
struct Queue
{
    static_assert( std::is_trivially_copyable< T >::value, "RTOS Queue items have to be trivially copyable" );

    using TickT = portTickType;
    static constexpr TickT MAX_DELAY = portMAX_DELAY;

    explicit Queue( size_t capacity ) : _q( xQueueCreate( capacity, sizeof( T ) ) )
    { }

    ~Queue() {
        vQueueDelete( _q );
    }

    bool valid() const { return _q; }
    explicit operator bool() const { return _q; }

    bool full() const { return uxQueueSpacesAvailable( _q ) == 0; }
    bool full_isr() const { return xQueueIsQueueFullFromISR( _q ); }

    std::optional< T > receive( TickT delay = MAX_DELAY )
    {
        char buf[ sizeof( T ) ];
        if ( xQueueReceive( _q, &buf, delay ) ) {
            return std::optional< T >( *reinterpret_cast< T * >( buf ) );
        }
        return std::nullopt;
    }

    bool send( const T &val, TickT delay = MAX_DELAY )
    {
        return xQueueSendToBack( _q, reinterpret_cast< const char * >( &val ), delay );
    }

    bool send_isr( const T &val )
    {
        return xQueueSendToBackFromISR( _q, reinterpret_cast< const char * >( &val ), nullptr );
    }

  private:
    xQueueHandle _q;
};

template< typename T, size_t _capacity >
struct QueueFixed : Queue< T >
{
    QueueFixed() : Queue< T >( _capacity ) { }
};

} // namespace frtos
