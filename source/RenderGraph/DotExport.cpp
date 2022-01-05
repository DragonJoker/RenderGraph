/*
See LICENSE file in root folder.
*/
#include "RenderGraph/DotExport.hpp"
#include "RenderGraph/FrameGraph.hpp"
#include "RenderGraph/FramePass.hpp"
#include "RenderGraph/GraphVisitor.hpp"
#include "RenderGraph/RunnableGraph.hpp"

#include <atomic>
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

			void indent( long i )
			{
				m_count += i;
				m_count = std::max( 0l, m_count );
			}

			std::streambuf * sbuf() const
			{
				return m_sbuf;
			}

		private:
			virtual int_type overflow( int_type c = traits::eof() )override
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
			long m_count;
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
			explicit Indent( long i )
				: indent( i )
			{
			}

			long indent;
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
						o_s << Indent{ getIndent( ios ) };
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

			addIndent( stream, ind.indent );
			sbuf->indent( ind.indent );
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
				, DisplayResult & streams
				, FramePassGroupStreams * parent = nullptr
				, FramePassGroup const * group = nullptr )
				: m_config{ config }
				, m_streams{ streams }
				, m_parent{ parent }
				, m_group{ group }
				, m_id{ uint32_t( getOutermost().size() ) }
			{
				streams.emplace( m_group ? m_group->name : std::string{}, std::stringstream{} );
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
					m_children.emplace_back( std::make_unique< FramePassGroupStreams >( m_config, m_streams, this, group ) );
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

			void write( DisplayResult & streams
				, std::stringstream const & global = {} )
			{
				if ( m_config.splitGroups )
				{
					if ( m_group )
					{
						auto streamIt = streams.find( m_group->name );
						std::stringstream stream;
						stream << "digraph \"" << m_group->name << "\" {\n";
						stream << streamIt->second.str();
						stream << "}\n";
						streamIt->second = std::move( stream );
					}

					for ( auto & group : m_children )
					{
						group->write( streams );
					}
				}
				else
				{
					auto streamIt = streams.find( m_group ? m_group->name : std::string{} );
					std::stringstream stream;
					stream << Indent{ getIndent( streamIt->second ) };

					if ( m_group )
					{
						if ( m_config.withGroups )
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
							streamIt->second << Indent{ 2 };
							stream << "subgraph cluster_" << m_id << " {\n";
							stream << "  label=\"" << m_group->name << "\";\n";

							if ( m_config.withColours )
							{
								stream << "  style=filled;\n";
								stream << "  fillcolor=\"" << colours[std::min( size_t( getLevel() ), colours.size() - 1u )] << "\";\n";
								stream << "  color=black;\n";
							}
						}
					}
					else
					{
						stream << "digraph {\n";
					}

					if ( m_config.withGroups )
					{
						stream << Indent{ 2 };
					}

					for ( auto & group : m_children )
					{
						group->write( streams );
						streamIt = streams.find( group->getName() );
						stream << streamIt->second.str();
					}

					if ( m_config.withGroups )
					{
						stream << Indent{ -2 };
					}

					streamIt = streams.find( m_group ? m_group->name : std::string{} );
					stream << streamIt->second.str();

					if ( !m_group )
					{
						stream << global.str();
						stream << "}\n";
					}
					else
					{
						if ( m_config.withGroups )
						{
							streamIt->second << Indent{ -2 };
							stream << "}\n";
						}
					}

					streamIt->second = std::move( stream );
				}
			}

			std::string const & getName()const
			{
				static std::string const dummy;
				return m_group ? m_group->name : dummy;
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
			DisplayResult & m_streams;
			FramePassGroupStreams * m_parent;
			std::vector< FramePassGroupStreamsPtr > m_children;
			FramePassGroup const * m_group;
			uint32_t m_id;
		};

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
				, DisplayResult & streams
				, FramePassGroupStreams & groups
				, Config const & config )
			{
				auto name = group ? group->name : "External";
				auto ires = groups.emplace( group );
				auto & grstream = *ires.first;
				streams.emplace( grstream.getName(), std::stringstream{} );
				return grstream;
			}

			static FramePassGroupStreams & displayPassNode( FramePassGroupStreams & pstream
				, uint32_t id
				, std::string const & name
				, FramePassGroup const * group
				, std::string_view const & colour
				, DisplayResult & streams
				, std::set< std::string > & nodes
				, FramePassGroupStreams & groups
				, Config const & config )
			{
				streams.emplace( pstream.getName(), std::stringstream{} );
				auto & groupStream = displayGroupNode( pstream, group, streams, groups, config );
				auto & stream = streams.find( groupStream.getName() )->second;
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

				return groupStream;
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
				, DisplayResult & streams
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

				displayPassNode( stream, nodeId, node, group, colour, streams, nodes, groups, config );
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
				, DisplayResult & streams
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
					srcStream = groups.find( &transition.outputAttach.pass->group );
				}

				if ( transition.inputAttach.pass )
				{
					dstNode = transition.inputAttach.pass->name;
					dstStream = groups.find( &transition.inputAttach.pass->group );
				}

				auto curstream = &stream;

				if ( srcStream != &groups
					&& dstStream != &groups )
				{
					if ( srcStream == dstStream )
					{
						curstream = &streams.find( srcStream->getName() )->second;
					}
					else if ( auto groupStreams = getCommonGroup( srcStream, dstStream ) )
					{
						curstream = &streams.find( groupStreams->getName() )->second;
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
				FramePassGroupStreams groups{ config, streams };

				for ( auto & transition : transitions.viewTransitions )
				{
					displayAttachPass( groups, transition.outputAttach, streams, nodes, groups, config );
					displayAttachPass( groups, transition.inputAttach, streams, nodes, groups, config );
				}

				for ( auto & transition : transitions.bufferTransitions )
				{
					displayAttachPass( groups, transition.outputAttach, streams, nodes, groups, config );
					displayAttachPass( groups, transition.inputAttach, streams, nodes, groups, config );
				}

				std::stringstream trstream;

				for ( auto & transition : transitions.viewTransitions )
				{
					displayTransitionEdge( trstream, imgColour, transition, streams, nodes, groups, config );
				}

				for ( auto & transition : transitions.bufferTransitions )
				{
					displayTransitionEdge( trstream, bufColour, transition, streams, nodes, groups, config );
				}

				groups.write( streams, trstream );
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

				auto lhsStream = &displayPassNode( m_groups, lhs->getId(), lhs->getName(), &lhs->getGroup(), passColour, m_streams, m_nodes, m_groups, m_config );
				auto rhsStream = &displayPassNode( m_groups, rhs->getId(), rhs->getName(), &rhs->getGroup(), passColour, m_streams, m_nodes, m_groups, m_config );
				auto curstream = &m_streams.find( std::string{} )->second;

				if ( lhsStream != &m_groups
					&& rhsStream != &m_groups )
				{
					if ( lhsStream == rhsStream )
					{
						curstream = &m_streams.find( lhsStream->getName() )->second;
					}
					else if ( auto streams = getCommonGroup( lhsStream, rhsStream ) )
					{
						curstream = &m_streams.find( streams->getName() )->second;
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
				for ( auto & next : node->getNext() )
				{
					submit( m_streams
						, m_nodes
						, m_groups
						, next
						, m_config
						, m_visited );
				}

				m_groups.write( m_streams );
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
		DisplayResult result;
		FramePassGroupStreams groups{ config, result };
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

		if ( config.splitGroups )
		{
			for ( auto & it : result )
			{
				if ( !it.first.empty() )
				{
					stream << it.second.str();
				}
			}
		}
		else
		{
			auto it = result.find( std::string{} );
			stream << it->second.str();
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
