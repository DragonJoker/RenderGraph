/*
See LICENSE file in root folder.
*/
#include "RenderGraph/DotExport.hpp"
#include "RenderGraph/FrameGraph.hpp"
#include "RenderGraph/FramePass.hpp"
#include "RenderGraph/GraphVisitor.hpp"
#include "RenderGraph/RunnableGraph.hpp"

#include <numeric>
#include <array>
#include <span>
#include <string>
#include <type_traits>

#pragma warning( push )
#pragma warning( disable: 5262 )
#include <atomic>
#include <fstream>
#include <sstream>
#pragma warning( pop )

namespace crg::dot
{
	namespace dotexp
	{
		template < typename char_type, char_type fill_char = ' ', typename traits = std::char_traits< char_type > >
		struct BasicIndentBuffer
			: public std::basic_streambuf< char_type, traits >
		{
		public:
			using int_type = typename traits::int_type;
			using pos_type = typename traits::pos_type;
			using off_type = typename traits::off_type;

		private:
			BasicIndentBuffer( const BasicIndentBuffer< char_type, fill_char, traits > & ) = delete;
			BasicIndentBuffer< char_type, fill_char, traits > & operator =( const BasicIndentBuffer< char_type, fill_char, traits > & ) = delete;

		public:
			explicit BasicIndentBuffer( std::basic_streambuf< char_type, traits > * sbuf )
				: m_sbuf{ sbuf }
				, m_set{ true }
			{
			}

			int indent() const
			{
				return m_count;
			}

			void indent( long i )
			{
				m_count += i;
				m_count = std::max( 0L, m_count );
			}

			std::streambuf * sbuf() const
			{
				return m_sbuf;
			}

		private:
			int_type overflow( int_type c = traits::eof() )override
			{
				if ( traits::eq_int_type( c, traits::eof() ) )
				{
					return m_sbuf->sputc( static_cast< char_type >( c ) );
				}

				if ( m_set )
				{
					std::fill_n( std::ostreambuf_iterator< char_type >( m_sbuf ), m_count, fill_char );
					m_set = false;
				}

				if ( traits::eq_int_type( m_sbuf->sputc( static_cast< char_type >( c ) ), traits::eof() ) )
				{
					return traits::eof();
				}

				if ( traits::eq_int_type( c, traits::to_char_type( '\n' ) ) )
				{
					m_set = true;
				}

				return traits::not_eof( c );
			}

		private:
			std::basic_streambuf< char_type, traits > * m_sbuf;
			long m_count{ 0 };
			bool m_set{ false };
		};

		template< typename char_type, typename traits = std::char_traits< char_type > >
		class BasicIndentBufferManager
		{
		private:
			using bos = std::ios_base;
			using bsb = std::basic_streambuf< char_type, traits >;
			using table_type = std::map< bos *, bsb * >;
			using value_type = typename table_type::value_type;
			using iterator = typename table_type::iterator;
			using const_iterator = typename table_type::const_iterator;
			using lock_type = std::unique_lock< std::mutex >;

			BasicIndentBufferManager()
			{
				++sm_instances;
			}

			BasicIndentBufferManager( BasicIndentBufferManager< char_type, traits > & obj ) = delete;

		public:
			~BasicIndentBufferManager()noexcept
			{
				try
				{
					--sm_instances;
					lock_type lock{ m_mutex };

					for ( iterator it = m_list.begin(); it != m_list.end(); ++it )
					{
						delete it->second;
					}
				}
				catch ( ... )
				{
					// What to do here ?
				}
			}

			bool insert( bos & o_s, bsb * b_s )
			{
				lock_type lock{ m_mutex };
				return m_list.emplace( &o_s, b_s ).second;
			}

			size_t size()
			{
				lock_type lock{ m_mutex };
				return m_list.size();
			}

			bsb * getBuffer( std::ios_base & io_s )
			{
				lock_type lock{ m_mutex };
				const_iterator cb_iter( m_list.find( &io_s ) );
				bsb * result{};

				if ( cb_iter != m_list.end() )
				{
					result = cb_iter->second;
				}

				return result;
			}

			bool erase( std::ios_base & io_s )
			{
				delete getBuffer( io_s );
				lock_type lock{ m_mutex };
				return ( m_list.erase( &io_s ) == 1u );
			}

			static size_t instances()
			{
				return size_t( sm_instances );
			}

			static BasicIndentBufferManager< char_type, traits > * instance()
			{
				static BasicIndentBufferManager< char_type, traits > ibm;
				return &ibm;
			}

		private:
			static std::atomic_int sm_instances;
			table_type m_list;
			std::mutex m_mutex;
		};

		template< typename char_type, typename traits >
		std::atomic_int BasicIndentBufferManager< char_type, traits >::sm_instances = 0;

		inline void addIndent( std::ios_base & ios, long val );

		inline long & indentValue( std::ios_base & ios )
		{
			static int indentIndex = std::ios_base::xalloc();
			return ios.iword( indentIndex );
		}

		inline long getIndent( std::ios_base & ios )
		{
			return indentValue( ios );
		}

		inline void addIndent( std::ios_base & ios, long val )
		{
			indentValue( ios ) += val;
		}

		template< typename CharType, typename BufferType = BasicIndentBuffer< CharType >, typename BufferManagerType = BasicIndentBufferManager< CharType > >
		inline BufferType * installIndentBuffer( std::basic_ostream< CharType > & stream )
		{
			BufferType * sbuf( new BufferType( stream.rdbuf() ) );
			BufferManagerType::instance()->insert( stream, sbuf );
			stream.rdbuf( sbuf );
			return sbuf;
		}

		template< typename CharType >
		inline void callback( std::ios_base::event ev, std::ios_base & ios, int )
		{
			if ( BasicIndentBufferManager< CharType >::instances()
				&& ev == std::ios_base::erase_event )
			{
				BasicIndentBufferManager< CharType >::instance()->erase( ios );
			}
		}

		struct Indent
		{
			explicit Indent( long i )
				: indent( i )
			{
			}

			long indent;

		private:
			template< typename CharType >
			friend std::basic_ostream< CharType > & operator<<( std::basic_ostream< CharType > & stream, Indent const & ind )
			{
				auto sbuf = dynamic_cast< BasicIndentBuffer< CharType > * >( stream.rdbuf() );

				if ( !sbuf )
				{
					sbuf = installIndentBuffer( stream );
					stream.register_callback( callback< CharType >, 0 );
				}

				addIndent( stream, ind.indent );
				sbuf->indent( ind.indent );
				return stream;
			}
		};

		static std::string_view constexpr imgColour{ "#8b008b" };
		static std::string_view constexpr bufColour{ "#458b00" };
		static std::string_view constexpr extColour{ "#ff7f00" };
		static std::string_view constexpr passColour{ "#00007f" };

		struct FramePassGroupStreams
		{
			using FramePassGroupStreamsPtr = std::unique_ptr< FramePassGroupStreams >;

			explicit FramePassGroupStreams( Config const & config
				, FramePassGroupStreams * parent = nullptr
				, FramePassGroup const * group = nullptr )
				: m_config{ config }
				, m_parent{ parent }
				, m_group{ group }
			{
			}

			std::pair< FramePassGroupStreams *, bool > emplace( FramePassGroup const * group )
			{
				auto streams = find( group );
				if ( streams )
					return { streams, false };

				assert( group != nullptr );

				if ( group->getParent() == m_group )
				{
					m_children.emplace_back( std::make_unique< FramePassGroupStreams >( m_config, this, group ) );
					streams = m_children.back().get();
				}
				else
				{
					streams = emplace( group->getParent() ).first;
					streams = streams->emplace( group ).first;
				}

				return { streams, true };
			}

			FramePassGroupStreams * find( FramePassGroup const * group )
			{
				if ( m_group == group )
					return this;

				FramePassGroupStreams * result{};
				auto it = std::find_if( m_children.begin(), m_children.end()
					, [group, &result]( FramePassGroupStreamsPtr const & lookup )
					{
						auto ret = lookup->find( group );
						if ( ret )
							result = ret;
						return ret != nullptr;
					} );

				return it == m_children.end()
					? nullptr
					: result;
			}

			void doWriteSplitted( DisplayResult & streams )const
			{
				if ( m_group )
				{
					auto & stream = streams.try_emplace( m_group->getName() ).first->second;
					stream << "digraph \"" << m_group->getName() << "\" {\n";
					stream << m_stream.str();
					stream << "}\n";
				}

				for ( auto & group : m_children )
					group->write( streams );
			}

			void doWriteUnsplittedWithGroups( DisplayResult & streams
				, std::stringstream const & global )const
			{
				auto & stream = streams.try_emplace( m_group ? m_group->getName() : std::string{} ).first->second;

				if ( m_group )
				{
					stream << "subgraph cluster_" << m_group->getId() << " {\n";
					stream << "  label=\"" << m_group->getName() << "\";\n";

					if ( m_config.withColours )
					{
						static std::vector< std::string_view > colours
						{
							{ "#FFFFFF" },
							{ "#EEEEEE" },
							{ "#DDDDDD" },
							{ "#CCCCCC" },
							{ "#BBBBBB" },
							{ "#AAAAAA" },
							{ "#999999" },
							{ "#888888" },
						};
						stream << "  style=filled;\n";
						stream << "  fillcolor=\"" << colours[std::min( size_t( getLevel() ), colours.size() - 1u )] << "\";\n";
						stream << "  color=black;\n";
					}
				}
				else
				{
					stream << "digraph {\n";
				}

				stream << Indent{ 2 };

				for ( auto & group : m_children )
				{
					group->write( streams );
					stream << streams.find( group->getName() )->second.str();
				}

				auto & stream2 = streams.try_emplace( m_group ? m_group->getName() : std::string{} ).first->second;
				stream2 << Indent{ -2 };
				stream2 << m_stream.str();
				if ( !m_group )
					stream2 << global.str();
				stream2 << "}\n";
			}

			void doWriteUnsplitted( DisplayResult & streams
				, std::stringstream const & global )const
			{
				auto & stream = streams.try_emplace( m_group ? m_group->getName() : std::string{} ).first->second;

				if ( !m_group )
					stream << "digraph {\n";

				for ( auto & group : m_children )
				{
					group->write( streams );
					stream << streams.find( group->getName() )->second.str();
				}

				auto & stream2 = streams.try_emplace( m_group ? m_group->getName() : std::string{} ).first->second;
				stream2 << m_stream.str();

				if ( !m_group )
				{
					stream2 << global.str();
					stream2 << "}\n";
				}
			}

			void write( DisplayResult & streams
				, std::stringstream const & global = std::stringstream{} )const
			{
				if ( m_config.splitGroups )
					doWriteSplitted( streams );
				else if ( m_config.withGroups )
					doWriteUnsplittedWithGroups( streams, global );
				else
					doWriteUnsplitted( streams, global );
			}

			std::string const & getName()const
			{
				static std::string const dummy;
				return m_group ? m_group->getName() : dummy;
			}

			std::set< std::string, std::less<> > & getNodes()
			{
				return m_nodes;
			}

			FramePassGroupStreams * getParent()const
			{
				return m_parent;
			}

			uint32_t getLevel()const
			{
				if ( m_parent )
					return ( m_parent->m_parent
						? 1u + m_parent->getLevel()
						: m_parent->getLevel() );
				return 0u;
			}

			std::stringstream & getStream()const
			{
				return m_stream;
			}

		private:
			Config const & m_config;
			mutable std::stringstream m_stream;
			FramePassGroupStreams * m_parent;
			std::vector< FramePassGroupStreamsPtr > m_children;
			FramePassGroup const * m_group;
			std::set< std::string, std::less<> > m_nodes;
		};

		static void displayNode( std::ostream & stream
			, std::string const & name
			, std::string const & shape
			, std::string_view colour
			, std::set< std::string, std::less<> > & nodes
			, Config const & config )
		{
			if ( nodes.insert( name ).second )
			{
				stream << Indent{ 2 };
				stream << "\"" << name << "\" [ shape=" << shape;

				if ( config.withColours )
				{
					stream << " style=filled";
					stream << " fillcolor=white";
					stream << " color=\"" << colour << "\"";
				}

				stream << " ];\n";
				stream << Indent{ -2 };
			}
		}

		static FramePassGroupStreams & displayGroupNode( FramePassGroup const * group
			, FramePassGroupStreams & groups
			, Config const & )
		{
			return *groups.emplace( group ).first;
		}

		static FramePassGroupStreams & displayPassNode( uint32_t id
			, std::string const & name
			, FramePassGroup const * group
			, std::string_view colour
			, FramePassGroupStreams & groups
			, Config const & config )
		{
			auto & groupStream = displayGroupNode( group, groups, config );
			auto & stream = groupStream.getStream();

			if ( groupStream.getNodes().insert( name ).second )
			{
				stream << Indent{ 2 };
				stream << "\"" << name << "\" [ shape=ellipse";

				if ( config.withIds )
				{
					stream << " id=\"" << id << "\"";
				}

				if ( config.withColours )
				{
					stream << " style=filled";
					stream << " fillcolor=white";
					stream << " color=\"" << colour << "\"";
				}

				stream << " ];\n";
				stream << Indent{ -2 };
			}

			return groupStream;
		}

		static void displayEdge( std::ostream & stream
			, std::string const & from
			, std::string const & to
			, std::string const & label
			, std::string_view colour
			, Config const & config )
		{
			stream << Indent{ 2 };
			stream << "\"" << from << "\" -> \"" << to << "\" [ label=\"" << label << "\"";

			if ( config.withColours )
			{
				stream << " color = \"" << colour << "\" fontcolor=\"" << colour << "\"";
			}

			stream << " ];\n";
			stream << Indent{ -2 };
		}

		static void displayAttachPass( Attachment const & attach
			, FramePassGroupStreams & groups
			, Config const & config )
		{
			uint32_t nodeId = 0;
			std::string node = "ExternalSource";
			FramePassGroup const * group = nullptr;
			std::string_view colour = extColour;

			if ( attach.pass )
			{
				nodeId = attach.pass->getId();
				node = attach.pass->getGroupName();
				group = &attach.pass->getGroup();
				colour = passColour;
			}

			displayPassNode( nodeId, node, group, colour, groups, config );
		}

		static std::string const & getAttachName( crg::BufferViewId const & data )
		{
			return data.data->name;
		}

		static std::string const & getAttachName( crg::ImageViewId const & data )
		{
			return data.data->name;
		}

		static bool isIn( FramePassGroupStreams const * inner
			, FramePassGroupStreams const * outer )
		{
			return outer == inner
				|| ( outer && isIn( inner, outer->getParent() ) );
		}

		static FramePassGroupStreams * getCommonGroup( FramePassGroupStreams * lhs
			, FramePassGroupStreams const * rhs )
		{
			auto current = lhs;

			while ( current && !isIn( current, rhs ) )
			{
				current = current->getParent();
			}

			return current;
		}

		template< typename TransitionT >
		static void displayTransitionEdge( std::ostream & stream
			, std::string_view colour
			, TransitionT const & transition
			, FramePassGroupStreams & groups
			, Config const & config )
		{
			auto srcName = transition.outputAttach.name;
			auto dstName = transition.inputAttach.name;
			std::string srcNode = "ExternalSource";
			std::string dstNode = "ExternalDestination";
			auto srcStream = &groups;
			auto dstStream = &groups;
			auto curStreams = &groups;

			if ( transition.outputAttach.pass )
			{
				srcNode = transition.outputAttach.pass->getGroupName();
				srcStream = groups.find( &transition.outputAttach.pass->getGroup() );
			}

			if ( transition.inputAttach.pass )
			{
				dstNode = transition.inputAttach.pass->getGroupName();
				dstStream = groups.find( &transition.inputAttach.pass->getGroup() );
			}

			std::string name{ "Transition to\\n" + dstName };
			auto curstream = &stream;

			if ( srcStream != &groups
				&& dstStream != &groups )
			{
				if ( srcStream == dstStream )
				{
					curStreams = srcStream;
					curstream = &srcStream->getStream();
				}
				else if ( auto groupStreams = getCommonGroup( srcStream, dstStream ) )
				{
					curStreams = groupStreams;
					curstream = &groupStreams->getStream();
				}
			}

			displayNode( *curstream, name, "box", colour, curStreams->getNodes(), config );
			displayEdge( *curstream, srcNode, name, getAttachName( transition.data ), colour, config );
			displayEdge( *curstream, name, dstNode, getAttachName( transition.data ), colour, config );
		}

		class DotTransitionsVisitor
			: public GraphVisitor
		{
		public:
			static void submit( DisplayResult & streams
				, ConstGraphAdjacentNode node
				, Config const & config )
			{
				std::set< ConstGraphAdjacentNode > visited;
				std::stringstream trstream;
				FramePassGroupStreams groups{ config };
				submit( streams, node, config, groups, trstream, visited );
				groups.write( streams, trstream );
			}

		private:
			static void submit( DisplayResult & streams
				, ConstGraphAdjacentNode node
				, Config const & config
				, FramePassGroupStreams & groups
				, std::stringstream & trstream
				, std::set< ConstGraphAdjacentNode > & visited )
			{
				DotTransitionsVisitor vis{ streams, groups, trstream, config, visited };
				node->accept( &vis );
			}

			DotTransitionsVisitor( DisplayResult & streams
				, FramePassGroupStreams & groups
				, std::stringstream & trstream
				, Config const & config
				, std::set< ConstGraphAdjacentNode > & visited )
				: m_streams{ streams }
				, m_groups{ groups }
				, m_trstream{ trstream }
				, m_config{ config }
				, m_visited{ visited }
			{
			}

			void submit( ConstGraphAdjacentNode node )
			{
				submit( m_streams
					, node
					, m_config
					, m_groups
					, m_trstream
					, m_visited );
			}

			void visitRootNode( RootNode const * node )override
			{
				for ( auto & pred : node->getPredecessors() )
				{
					submit( m_streams
						, pred
						, m_config
						, m_groups
						, m_trstream
						, m_visited );
				}
			}

			void visitFramePassNode( FramePassNode const * node )override
			{
				for ( auto & transition : node->getImageTransitions() )
				{
					displayAttachPass( transition.outputAttach, m_groups, m_config );
					displayAttachPass( transition.inputAttach, m_groups, m_config );
				}

				for ( auto & transition : node->getBufferTransitions() )
				{
					displayAttachPass( transition.outputAttach, m_groups, m_config );
					displayAttachPass( transition.inputAttach, m_groups, m_config );
				}

				for ( auto & transition : node->getImageTransitions() )
				{
					displayTransitionEdge( m_trstream, imgColour, transition, m_groups, m_config );
				}

				for ( auto & transition : node->getBufferTransitions() )
				{
					displayTransitionEdge( m_trstream, bufColour, transition, m_groups, m_config );
				}

				m_visited.insert( node );

				for ( auto & pred : node->getPredecessors() )
				{
					if ( m_visited.end() == m_visited.find( pred ) )
					{
						submit( pred );
					}
				}
			}

		private:
			DisplayResult & m_streams;
			FramePassGroupStreams & m_groups;
			std::stringstream & m_trstream;
			Config const & m_config;
			std::set< GraphNode const * > & m_visited;
		};

		static std::string applyRemove( std::string const & text, Config const & config )
		{
			if ( config.toRemove.empty() )
				return text;

			auto result = text;
			size_t startPos = 0u;

			while ( ( startPos = result.find( config.toRemove, startPos ) ) != std::string::npos )
				result.replace( startPos, config.toRemove.length(), "" );

			return result;
		}
	}

	DisplayResult displayTransitions( RunnableGraph const & value
		, Config const & config )
	{
		DisplayResult result;
		dotexp::DotTransitionsVisitor::submit( result, value.getNodeGraph(), config );
		return result;
	}

	void displayTransitions( std::ostream & stream
		, RunnableGraph const & value
		, Config const & config )
	{
		auto result = displayTransitions( value, config );

		if ( config.splitGroups )
		{
			for ( auto const & [name, strm] : result )
			{
				if ( !name.empty() )
				{
					stream << dotexp::applyRemove( strm.str(), config );
				}
			}
		}
		else
		{
			auto it = result.find( std::string{} );
			stream << dotexp::applyRemove( it->second.str(), config );
		}
	}
}
