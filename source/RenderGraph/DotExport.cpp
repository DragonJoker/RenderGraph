/*
See LICENSE file in root folder.
*/
#include "RenderGraph/DotExport.hpp"
#include "RenderGraph/FrameGraph.hpp"
#include "RenderGraph/FramePass.hpp"
#include "RenderGraph/GraphVisitor.hpp"
#include "RenderGraph/RunnableGraph.hpp"

#include <array>
#include <fstream>
#include <string>
#include <type_traits>

namespace crg::dot
{
	namespace
	{
		class DotOutVisitor
			: public GraphVisitor
		{
		public:
			static void submit( std::ostream & stream
				, ConstGraphAdjacentNode node
				, std::set< ConstGraphAdjacentNode > & visited )
			{
				DotOutVisitor vis{ stream, visited };
				node->accept( &vis );
			}

			static void submit( std::ostream & stream
				, ConstGraphAdjacentNode node )
			{
				std::set< ConstGraphAdjacentNode > visited;
				submit( stream, node, visited );
			}

			static void submit( std::ostream & stream
				, AttachmentTransitionArray const & transitions )
			{
				stream << "digraph \"Transitions\" {\n";

				for ( auto & transition : transitions )
				{
					std::string name{ transition.outputAttach.name + "\\ntransition to\\n" + transition.inputAttach.name };
					stream << "    \"" << name << "\" [ shape=box ];\n";

					if ( transition.outputAttach.pass )
					{
						stream << "    \"" << transition.outputAttach.pass->name << "\" -> \"" << name << "\" [ label=\"" << transition.view.data->name << "\" ];\n";
					}
					else
					{
						stream << "    \"ExternalSource\" -> \"" << name << "\" [ label=\"" << transition.view.data->name << "\" ];\n";
					}

					if ( transition.inputAttach.pass )
					{
						stream << "    \"" << name << "\" -> \"" << transition.inputAttach.pass->name << "\" [ label=\"" << transition.view.data->name << "\" ];\n";
					}
					else
					{
						stream << "    \"" << name << "\" -> \"ExternalDestination\" [ label=\"" << transition.view.data->name << "\" ];\n";
					}
				}

				stream << "}\n";
			}

		private:
			DotOutVisitor( std::ostream & stream
				, std::set< ConstGraphAdjacentNode > & visited )
				: m_stream{ stream }
				, m_visited{ visited }
			{
			}

			void printEdge( ConstGraphAdjacentNode lhs
				, ConstGraphAdjacentNode rhs )
			{
				std::string sep;
				auto transitions = rhs->getInputAttaches( lhs );
				std::sort( transitions.begin()
					, transitions.end()
					, []( AttachmentTransition const & lhs, AttachmentTransition const & rhs )
					{
						return lhs.outputAttach.name < rhs.outputAttach.name;
					} );
				uint32_t index{ 1u };

				for ( auto & transition : transitions )
				{
					std::string name{ "Transition to\\n" + transition.inputAttach.name };
					m_stream << "    \"" << name << "\" [ shape=box ];\n";
					m_stream << "    \"" << lhs->getName() << "\" -> \"" << name << "\" [ label=\"" << transition.view.data->name << "\" ];\n";
					m_stream << "    \"" << name << "\" -> \"" << rhs->getName() << "\" [ label=\"" << transition.view.data->name << "\" ];\n";
				}
			}

			void submit( ConstGraphAdjacentNode node )
			{
				submit( m_stream
					, node
					, m_visited );
			}

			void visitRootNode( RootNode const * node )override
			{
				m_stream << "digraph \"" << node->getName() << "\" {\n";

				for ( auto & next : node->getNext() )
				{
					submit( next );
				}

				m_stream << "}\n";
			}

			void visitFramePassNode( FramePassNode const * node )override
			{
				m_visited.insert( node );
				auto nexts = node->getNext();

				for ( auto & next : nexts )
				{
					printEdge( node, next );

					if ( m_visited.end() == m_visited.find( next ) )
					{
						submit( next );
					}
				}
			}

		private:
			std::ostream & m_stream;
			std::set< GraphNode const * > & m_visited;
		};
	}

	void displayPasses( std::ostream & stream
		, RunnableGraph const & value )
	{
		DotOutVisitor::submit( stream, value.getGraph() );
	}

	void displayTransitions( std::ostream & stream
		, RunnableGraph const & value )
	{
		DotOutVisitor::submit( stream, value.getTransitions() );
	}
}
