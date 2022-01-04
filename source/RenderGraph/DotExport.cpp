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
#include <fstream>
#include <sstream>
#include <string>
#include <type_traits>

namespace crg::dot
{
	namespace
	{
		template < typename char_type, char_type fill_char = ' ', typename traits = std::char_traits< char_type > >
		struct BasicIndentBuffer
			: public std::basic_streambuf< char_type, traits >
		{
		public:
			typedef typename traits::int_type int_type;
			typedef typename traits::pos_type pos_type;
			typedef typename traits::off_type off_type;

		private:
			BasicIndentBuffer( const BasicIndentBuffer< char_type, fill_char, traits > & ) = delete;
			BasicIndentBuffer< char_type, fill_char, traits > & operator =( const BasicIndentBuffer< char_type, fill_char, traits > & ) = delete;

		public:
			explicit BasicIndentBuffer( std::basic_streambuf< char_type, traits > * sbuf )
				: m_sbuf( sbuf )
				, m_count( 0 )
				, m_set( true )
			{
			}

			~BasicIndentBuffer()override = default;

			int indent() const
			{
				return m_count;
			}

			void indent( int i )
			{
				m_count += i;
			}

			std::streambuf * sbuf() const
			{
				return m_sbuf;
			}

		private:
			virtual int_type overflow( int_type c = traits::eof() )
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
			int m_count;
			bool m_set;
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
			~BasicIndentBufferManager()
			{
				--sm_instances;
				lock_type lock{ m_mutex };

				for ( iterator it = m_list.begin(); it != m_list.end(); ++it )
				{
					delete it->second;
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

				if ( cb_iter == m_list.end() )
				{
					return nullptr;
				}

				return cb_iter->second;
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

		struct Indent
		{
			explicit Indent( int i )
				: m_indent( i )
			{
			}

			int m_indent;
		};

		inline long & indentValue( std::ios_base & ios )
		{
			static int indentIndex = std::ios_base::xalloc();
			return ios.iword( indentIndex );
		}

		inline long getIndent( std::ios_base & ios )
		{
			return indentValue( ios );
		}

		inline void addIndent( std::ios_base & ios, int val )
		{
			indentValue( ios ) += long( val );
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
		inline void callback( std::ios_base::event ev, std::ios_base & ios, int x )
		{
			if ( BasicIndentBufferManager< CharType >::instances() )
			{
				if ( ev == std::ios_base::erase_event )
				{
					BasicIndentBufferManager< CharType >::instance()->erase( ios );
				}
				else if ( ev == std::ios_base::copyfmt_event )
				{
#if __GNUC__ && ( __GNUC__ < 3 || (__GNUC__ == 3 && __GNUC_MINOR__ < 3 ) )
#	error Your compiler is too buggy; it is known to miscompile.
#	error Known good compilers: 3.3
#else

					if ( std::basic_ostream< CharType > & o_s = dynamic_cast< std::basic_ostream< CharType > & >( ios ) )
					{
						o_s << Indent( getIndent( ios ) );
					}

#endif
				}
			}
		}

		template< typename CharType >
		inline std::basic_ostream< CharType > & operator <<( std::basic_ostream< CharType > & stream, Indent const & ind )
		{
			auto sbuf = dynamic_cast< BasicIndentBuffer< CharType > * >( stream.rdbuf() );

			if ( !sbuf )
			{
				sbuf = installIndentBuffer( stream );
				stream.register_callback( callback< CharType >, 0 );
			}

			addIndent( stream, ind.m_indent );
			sbuf->indent( ind.m_indent );
			return stream;
		}

		static std::string_view constexpr imgColour{ "#8b008b" };
		static std::string_view constexpr bufColour{ "#458b00" };
		static std::string_view constexpr extColour{ "#ff7f00" };
		static std::string_view constexpr passColour{ "#00007f" };

		struct FramePassGroupStreams
		{
			using FramePassGroupStreamsPtr = std::unique_ptr< FramePassGroupStreams >;

			FramePassGroupStreams( Config const & config
				, FramePassGroupStreams * parent = nullptr
				, FramePassGroup const * group = nullptr )
				: m_config{ config }
				, m_parent{ parent }
				, m_group{ group }
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

				if ( m_group )
				{
					auto id = uint32_t( getOutermost().size() );

					if ( m_config.splitGroups )
					{
						m_stream << "digraph \"" << m_group->name << "\" {\n";
					}
					else
					{
						m_stream << "subgraph cluster_" << id << " {\n";
						m_stream << "  label=\"" << m_group->name << "\";\n";

						if ( m_config.withColours )
						{
							m_stream << "  style=filled;\n";
							m_stream << "  fillcolor=\"" << colours[std::min( size_t( getLevel() ), colours.size() - 1u )] << "\";\n";
							m_stream << "  color=black;\n";
						}
					}
				}
			}

			FramePassGroupStreams & getOutermost()
			{
				auto group = m_group;
				auto result = this;

				while ( group )
				{
					group = group->parent;
					result = result->m_parent;
				}

				return *result;
			}

			std::pair< FramePassGroupStreams *, bool > emplace( FramePassGroup const * group )
			{
				auto streams = find( group );

				if ( streams )
				{
					return { streams, false };
				}

				if ( group->parent == m_group )
				{
					m_children.emplace_back( std::make_unique< FramePassGroupStreams >( m_config, this, group ) );
					streams = m_children.back().get();
				}
				else
				{
					auto ires = emplace( group->parent );
					streams = ires.first;
					streams = streams->emplace( group ).first;
				}

				return { streams, true };
			}

			size_t size()const noexcept
			{
				return std::accumulate( m_children.begin()
					, m_children.end()
					, size_t{}
					, []( size_t val, FramePassGroupStreamsPtr const & lookup )
					{
						return val + 1u + lookup->size();
					} );
			}

			FramePassGroupStreams * find( FramePassGroup const * group )
			{
				if ( m_group == group )
				{
					return this;
				}

				FramePassGroupStreams * result{};
				auto it = std::find_if( m_children.begin()
					, m_children.end()
					, [group, &result]( FramePassGroupStreamsPtr const & lookup )
					{
						auto ret = lookup->find( group );

						if ( ret )
						{
							result = ret;
						}

						return ret != nullptr;
					} );

				return it == m_children.end()
					? nullptr
					: result;
			}

			void write( DisplayResult & streams )
			{
				auto baseStreamIt = streams.emplace( ( m_config.splitGroups && m_group ) ? m_group->name : std::string{}, std::stringstream{} ).first;

				if ( m_group )
				{
					baseStreamIt->second << m_stream.str();
				}

				if ( m_config.splitGroups )
				{
					if ( m_group )
					{
						baseStreamIt->second << "}\n";
					}

					for ( auto & group : m_children )
					{
						group->write( streams );
					}
				}
				else
				{
					baseStreamIt->second << Indent{ 2 };

					for ( auto & group : m_children )
					{
						group->write( streams );
						baseStreamIt->second << "\n";
					}

					baseStreamIt->second << Indent{ -2 };

					if ( m_group )
					{
						baseStreamIt->second << "}";
					}
				}
			}

			std::stringstream & stream()
			{
				return m_stream;
			}

			FramePassGroupStreams * getParent()const
			{
				return m_parent;
			}

			uint32_t getLevel()const
			{
				return ( m_parent
					? ( m_parent->m_parent
						? 1u + m_parent->getLevel()
						: m_parent->getLevel() )
					: 0u );
			}

		private:
			Config const & m_config;
			FramePassGroupStreams * m_parent;
			std::vector< FramePassGroupStreamsPtr > m_children;
			FramePassGroup const * m_group;
			std::stringstream m_stream;
			uint32_t m_count{};
		};

		template< typename ValueT >
		FramePassGroupStreams & operator<<( FramePassGroupStreams & stream, ValueT const & value )
		{
			stream.stream() << value;
			return stream;
		}

		class DotOutVisitor
			: public GraphVisitor
		{
		public:
			static void submit( DisplayResult & streams
				, std::set< std::string > & nodes
				, FramePassGroupStreams & groups
				, ConstGraphAdjacentNode node
				, Config const & config
				, std::set< ConstGraphAdjacentNode > & visited )
			{
				DotOutVisitor vis{ streams, nodes, groups, config, visited };
				node->accept( &vis );
			}

			static void submit( DisplayResult & streams
				, std::set< std::string > & nodes
				, FramePassGroupStreams & groups
				, ConstGraphAdjacentNode node
				, Config const & config )
			{
				std::set< ConstGraphAdjacentNode > visited;
				submit( streams, nodes, groups, node, config, visited );
			}

			static void displayNode( std::ostream & stream
				, std::string const & name
				, std::string const & shape
				, std::string_view const & colour
				, std::set< std::string > & nodes
				, Config const & config )
			{
				auto ires = nodes.insert( name );

				if ( ires.second )
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

			static FramePassGroupStreams & displayGroupNode( FramePassGroupStreams & stream
				, FramePassGroup const * group
				, FramePassGroupStreams & groups
				, Config const & config )
			{
				if ( config.withGroups )
				{
					auto name = group ? group->name : "External";
					auto ires = groups.emplace( group );
					auto & grstream = *ires.first;
					return grstream;
				}

				return stream;
			}

			static FramePassGroupStreams & displayPassNode( FramePassGroupStreams & pstream
				, uint32_t id
				, std::string const & name
				, FramePassGroup const * group
				, std::string_view const & colour
				, std::set< std::string > & nodes
				, FramePassGroupStreams & groups
				, Config const & config )
			{
				auto & stream = displayGroupNode( pstream, group, groups, config );
				auto ires = nodes.insert( name );

				if ( ires.second )
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

				return stream;
			}

			static void displayEdge( std::ostream & stream
				, std::string const & from
				, std::string const & to
				, std::string const & label
				, std::string_view const & colour
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

			template< typename AttachT >
			static void displayAttachPass( FramePassGroupStreams & stream
				, AttachT const & attach
				, std::set< std::string > & nodes
				, FramePassGroupStreams & groups
				, Config const & config )
			{
				uint32_t nodeId = 0;
				std::string node = "ExternalSource";
				FramePassGroup const * group = nullptr;
				std::string_view colour = extColour;

				if ( attach.pass )
				{
					nodeId = attach.pass->id;
					node = attach.pass->name;
					group = &attach.pass->group;
					colour = passColour;
				}

				displayPassNode( stream, nodeId, node, group, colour, nodes, groups, config );
			}

			static std::string const & getAttachName( crg::Buffer const & data )
			{
				return data.name;
			}

			static std::string const & getAttachName( crg::ImageViewId const & data )
			{
				return data.data->name;
			}

			static bool isIn( FramePassGroupStreams * inner
				, FramePassGroupStreams * outer )
			{
				return outer == inner
					|| ( outer && isIn( inner, outer->getParent() ) );
			}

			static FramePassGroupStreams * getCommonGroup( FramePassGroupStreams * lhs
				, FramePassGroupStreams * rhs )
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
				, std::string_view const & colour
				, TransitionT const & transition
				, std::set< std::string > & nodes
				, FramePassGroupStreams & groups
				, Config const & config )
			{
				std::string name{ transition.outputAttach.name + "\\ntransition to\\n" + transition.inputAttach.name };
				std::string srcNode = "ExternalSource";
				std::string dstNode = "ExternalDestination";
				auto srcStream = &groups;
				auto dstStream = &groups;

				if ( transition.outputAttach.pass )
				{
					srcNode = transition.outputAttach.pass->name;

					if ( config.withGroups )
					{
						srcStream = groups.find( &transition.outputAttach.pass->group );
					}
				}

				if ( transition.inputAttach.pass )
				{
					dstNode = transition.inputAttach.pass->name;

					if ( config.withGroups )
					{
						dstStream = groups.find( &transition.inputAttach.pass->group );
					}
				}

				auto curstream = &stream;

				if ( srcStream != &groups
					&& dstStream != &groups )
				{
					if ( srcStream == dstStream )
					{
						curstream = &srcStream->stream();
					}
					else if ( auto streams = getCommonGroup( srcStream, dstStream ) )
					{
						curstream = &streams->stream();
					}
				}

				displayNode( *curstream, name, "box", colour, nodes, config );
				displayEdge( *curstream, srcNode, name, getAttachName( transition.data ), colour, config );
				displayEdge( *curstream, name, dstNode, getAttachName( transition.data ), colour, config );
			}

			static void submit( DisplayResult & streams
				, AttachmentTransitions const & transitions
				, Config const & config )
			{
				std::set< std::string > nodes;
				FramePassGroupStreams groups{ config };

				for ( auto & transition : transitions.viewTransitions )
				{
					displayAttachPass( groups, transition.outputAttach, nodes, groups, config );
					displayAttachPass( groups, transition.inputAttach, nodes, groups, config );
				}

				for ( auto & transition : transitions.bufferTransitions )
				{
					displayAttachPass( groups, transition.outputAttach, nodes, groups, config );
					displayAttachPass( groups, transition.inputAttach, nodes, groups, config );
				}

				std::stringstream trstream;

				for ( auto & transition : transitions.viewTransitions )
				{
					displayTransitionEdge( trstream, imgColour, transition, nodes, groups, config );
				}

				for ( auto & transition : transitions.bufferTransitions )
				{
					displayTransitionEdge( trstream, bufColour, transition, nodes, groups, config );
				}

				if ( !config.splitGroups )
				{
					auto & stream = streams.emplace( std::string{}, std::stringstream{} ).first->second;
					stream << "digraph \"Transitions\" {\n";
				}

				if ( config.withGroups )
				{
					groups.write( streams );
				}

				if ( !config.splitGroups )
				{
					auto & stream = streams.find( std::string{} )->second;
					stream << trstream.str();
					stream << "}\n";
				}
			}

		private:
			DotOutVisitor( DisplayResult & streams
				, std::set< std::string > & nodes
				, FramePassGroupStreams & groups
				, Config const & config
				, std::set< ConstGraphAdjacentNode > & visited )
				: m_streams{ streams }
				, m_nodes{ nodes }
				, m_groups{ groups }
				, m_config{ config }
				, m_visited{ visited }
			{
			}

			void printEdge( ConstGraphAdjacentNode lhs
				, ConstGraphAdjacentNode rhs
				, std::stringstream & stream )
			{
				std::string sep;
				auto transitions = rhs->getInputAttaches( lhs );
				std::sort( transitions.viewTransitions.begin()
					, transitions.viewTransitions.end()
					, []( ViewTransition const & ilhs, ViewTransition const & irhs )
					{
						return ilhs.outputAttach.name < irhs.outputAttach.name;
					} );
				std::sort( transitions.bufferTransitions.begin()
					, transitions.bufferTransitions.end()
					, []( BufferTransition const & ilhs, BufferTransition const & irhs )
					{
						return ilhs.outputAttach.name < irhs.outputAttach.name;
					} );

				auto lhsStream = &displayPassNode( m_groups, lhs->getId(), lhs->getName(), &lhs->getGroup(), passColour, m_nodes, m_groups, m_config );
				auto rhsStream = &displayPassNode( m_groups, rhs->getId(), rhs->getName(), &rhs->getGroup(), passColour, m_nodes, m_groups, m_config );
				auto curstream = &stream;

				if ( lhsStream != &m_groups
					&& rhsStream != &m_groups )
				{
					if ( lhsStream == rhsStream )
					{
						curstream = &lhsStream->stream();
					}
					else if ( auto streams = getCommonGroup( lhsStream, rhsStream ) )
					{
						curstream = &streams->stream();
					}
				}

				for ( auto & transition : transitions.viewTransitions )
				{
					std::string name{ "Transition to\\n" + transition.inputAttach.name };
					displayNode( *curstream, name, "box", imgColour, m_nodes, m_config );
					displayEdge( *curstream, lhs->getName(), name, transition.data.data->name, imgColour, m_config );
					displayEdge( *curstream, name, rhs->getName(), transition.data.data->name, imgColour, m_config );
				}

				for ( auto & transition : transitions.bufferTransitions )
				{
					std::string name{ "Transition to\\n" + transition.inputAttach.name };
					displayNode( *curstream, name, "box", bufColour, m_nodes, m_config );
					displayEdge( *curstream, lhs->getName(), name, transition.data.name, bufColour, m_config );
					displayEdge( *curstream, name, rhs->getName(), transition.data.name, bufColour, m_config );
				}
			}

			void submit( ConstGraphAdjacentNode node )
			{
				submit( m_streams
					, m_nodes
					, m_groups
					, node
					, m_config
					, m_visited );
			}

			void visitRootNode( RootNode const * node )override
			{
				m_streams.emplace( std::string{}, std::stringstream{} );

				if ( !m_config.splitGroups )
				{
					auto & stream = m_streams.find( std::string{} )->second;
					stream << "digraph \"" << node->getName() << "\" {\n";
				}

				for ( auto & next : node->getNext() )
				{
					submit( m_streams
						, m_nodes
						, m_groups
						, next
						, m_config
						, m_visited );
				}

				if ( m_config.withGroups )
				{
					m_groups.write( m_streams );
				}

				if ( !m_config.splitGroups )
				{
					auto & stream = m_streams.find( std::string{} )->second;
					stream << "}\n";
				}
			}

			void visitFramePassNode( FramePassNode const * node )override
			{
				m_visited.insert( node );
				auto nexts = node->getNext();

				for ( auto & next : nexts )
				{
					printEdge( node, next, m_streams.find( std::string{} )->second );

					if ( m_visited.end() == m_visited.find( next ) )
					{
						submit( next );
					}
				}
			}

		private:
			DisplayResult & m_streams;
			std::set< std::string > & m_nodes;
			FramePassGroupStreams & m_groups;
			Config const & m_config;
			std::set< GraphNode const * > & m_visited;
		};
	}

	DisplayResult displayPasses( RunnableGraph const & value
		, Config const & config )
	{
		std::set< std::string > nodes;
		FramePassGroupStreams groups{ config };
		DisplayResult result;
		DotOutVisitor::submit( result, nodes, groups, value.getGraph(), config );
		return result;
	}

	DisplayResult displayTransitions( RunnableGraph const & value
		, Config const & config )
	{
		DisplayResult result;
		DotOutVisitor::submit( result, value.getTransitions(), config );
		return result;
	}

	void displayPasses( std::ostream & stream
		, RunnableGraph const & value
		, Config const & config )
	{
		auto result = displayPasses( value, config );

		for ( auto & it : result )
		{
			stream << it.second.str();
		}
	}

	void displayTransitions( std::ostream & stream
		, RunnableGraph const & value
		, Config const & config )
	{
		auto result = displayTransitions( value, config );

		for ( auto & it : result )
		{
			stream << it.second.str();
		}
	}
}
