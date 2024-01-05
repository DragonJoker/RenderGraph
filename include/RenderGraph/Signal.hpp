/*
See LICENSE file in root folder
*/
#pragma once

#include <cassert>
#include <functional>
#include <set>
#include <map>
#pragma warning( push )
#pragma warning( disable: 4365 )
#pragma warning( disable: 5262 )
#include <mutex>
#pragma warning( pop )

namespace crg
{
	/**
	*\brief
	*	A connection to a signal.
	*/
	template< typename SignalT >
	class SignalConnection
	{
	private:
		SignalConnection( SignalConnection< SignalT > const & ) = delete;
		SignalConnection & operator=( SignalConnection< SignalT > const & ) = delete;

		SignalConnection( uint32_t connection
			, SignalT * signal )
			: m_connection{ connection }
			, m_signal{ signal }
		{
		}

	public:
		/**
		*\name
		*	Construction/Destruction.
		*/
		/**\{*/
		SignalConnection()
			: SignalConnection{ 0u, nullptr }
		{
		}

		SignalConnection( SignalConnection< SignalT > && rhs )noexcept
			: SignalConnection{ 0u, nullptr }
		{
			swap( *this, rhs );
		}

		SignalConnection( uint32_t connection, SignalT & signal )
			: SignalConnection{ connection, &signal }
		{
			signal.addConnection( *this );
		}

		SignalConnection & operator=( SignalConnection< SignalT > && rhs )noexcept
		{
			SignalConnection tmp{ std::move( rhs ) };
			swap( *this, tmp );
			return *this;
		}

		~SignalConnection()noexcept
		{
			disconnect();
		}
		/**\}*/
		/**
		*\brief
		*	Disconnects from the signal.
		*/
		void disconnect()noexcept
		{
			if ( m_signal && m_connection )
			{
				m_signal->disconnect( m_connection );
				m_signal->remConnection( *this );
				m_signal = nullptr;
				m_connection = 0u;
			}
		}

		operator bool()const
		{
			return m_signal && m_connection;
		}

	private:
		void swap( SignalConnection & lhs, SignalConnection & rhs )const noexcept
		{
			if ( lhs.m_signal )
			{
				lhs.m_signal->remConnection( lhs );
			}

			if ( rhs.m_signal )
			{
				rhs.m_signal->remConnection( rhs );
			}

			std::swap( lhs.m_signal, rhs.m_signal );
			std::swap( lhs.m_connection, rhs.m_connection );

			if ( lhs.m_signal )
			{
				lhs.m_signal->addConnection( lhs );
			}

			if ( rhs.m_signal )
			{
				rhs.m_signal->addConnection( rhs );
			}
		}

	private:
		uint32_t m_connection;
		SignalT * m_signal;
	};
	/**
	*\brief
	*	Signal implementation.
	*/
	template< typename Function >
	class Signal
	{
		friend class SignalConnection< Signal< Function > >;
		using my_connection = SignalConnection< Signal< Function > >;
		using my_connection_ptr = my_connection *;
		using lock_type = std::unique_lock< std::recursive_mutex >;

	private:
		Signal( Signal const & )noexcept = delete;
		Signal & operator=( Signal const & )noexcept = delete;
		Signal( Signal && )noexcept = delete;
		Signal & operator=( Signal && )noexcept = delete;

	public:
		Signal()noexcept = default;
		/**
		*\brief
		*	Destructor.
		*\remarks
		*	Disconnects all remaining connections.
		*/
		~Signal()noexcept
		{
			// SignalConnection::disconnect appelle Signal::remConnection, qui
			// supprime la connection de m_connections, invalidant ainsi
			// l'itérateur, donc on ne peut pas utiliser un for_each, ni
			// un range for loop.
			lock_type lock( m_mutex );
			auto it = m_connections.begin();

			while ( it != m_connections.end() )
			{
				( *it )->disconnect();
				it = m_connections.begin();
			}
		}
		/**
		*\brief
		*	Connects a new function, called when the signal is emitted.
		*\param[in] function
		*	The function.
		*\return
		*	The connection.
		*/
		my_connection connect( Function function )
		{
			uint32_t index = uint32_t( m_slots.size() ) + 1u;
			m_slots.emplace( index, function );
			return my_connection{ index, *this };
		}
		/**
		*\brief
		*	Emits the signal, calling all connected functions.
		*/
		void operator()()const
		{
			auto it = m_slots.begin();
			size_t size = m_slots.size();
			size_t index = 0u;

			while ( it != m_slots.end() )
			{
				it->second();
				adjustSlots( size, index, it );
			}
		}
		/**
		*\brief
		*	Emits the signal, calling all connected functions.
		*\param[in] params
		*	The functions parameters.
		*/
		template< typename ... Params >
		void operator()( Params && ... params )const
		{
			auto it = m_slots.begin();
			size_t size = m_slots.size();
			size_t index = 0u;

			while ( it != m_slots.end() )
			{
				it->second( std::forward< Params >( params )... );
				adjustSlots( size, index, it );
			}
		}

	private:
		/**
		*\brief
		*	Disconnects a function.
		*\param[in] index
		*	The function index.
		*/
		void disconnect( uint32_t index )noexcept
		{
			auto it = m_slots.find( index );

			if ( it != m_slots.end() )
			{
				m_slots.erase( it );
			}
		}
		/**
		*\brief
		*	Adds a connection to the list.
		*\param[in] connection
		*	The connection to add.
		*/
		void addConnection( my_connection & connection )noexcept
		{
			try
			{
				lock_type lock( m_mutex );
				m_connections.insert( &connection );
			}
			catch ( ... )
			{
				// Nothing to do here
			}
		}
		/**
		*\brief
		*	Removes a connection from the list.
		*\param[in] connection
		*	The connection to remove.
		*/
		void remConnection( my_connection & connection )noexcept
		{
			try
			{
				lock_type lock( m_mutex );
				assert( m_connections.find( &connection ) != m_connections.end() );
				m_connections.erase( &connection );
			}
			catch ( ... )
			{
				// Nothing to do here
			}
		}
		/**
		*\brief
		*	Adjusts returned slot iterator.
		*\param[in,out] size
		*	The expected size, receives the real size.
		*\param[in,out] index
		*	The current iteration index.
		*/
		template< typename IterT >
		void adjustSlots( size_t & size
			, size_t & index
			, IterT & it )const noexcept
		{
			if ( size != m_slots.size() )
			{
				// Slots changed by the slot itself.
				size = m_slots.size();
				it = m_slots.begin();
				std::advance( it, index );
			}
			else
			{
				++index;
				++it;
			}
		}

	private:
		std::map< uint32_t, Function > m_slots;
		std::set< my_connection_ptr > m_connections;
		std::recursive_mutex m_mutex;
	};
}
