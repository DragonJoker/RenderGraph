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
				, std::set< std::string > & nodes
				, ConstGraphAdjacentNode node
				, bool withColours
				, std::set< ConstGraphAdjacentNode > & visited )
			{
				DotOutVisitor vis{ stream, nodes, withColours, visited };
				node->accept( &vis );
			}

			static void submit( std::ostream & stream
				, std::set< std::string > & nodes
				, ConstGraphAdjacentNode node
				, bool withColours )
			{
				std::set< ConstGraphAdjacentNode > visited;
				submit( stream, nodes, node, withColours, visited );
			}

			static void displayNode( std::ostream & stream
				, std::string const & name
				, std::string const & shape
				, std::string_view const & colour
				, std::set< std::string > & nodes
				, bool withColours )
			{
				auto ires = nodes.insert( name );

				if ( ires.second )
				{
					stream << "    \"" << name << "\" [ shape=" << shape;

					if ( withColours )
					{
						stream << " color=\"" << colour << "\"";
					}

					stream << " ];\n";
				}
			}

			static void displayEdge( std::ostream & stream
				, std::string const & from
				, std::string const & to
				, std::string const & label
				, std::string_view const & colour
				, bool withColours )
			{
				stream << "    \"" << from << "\" -> \"" << to << "\" [ label=\"" << label << "\"";

				if ( withColours )
				{
					stream << " color = \"" << colour << "\" fontcolor=\"" << colour << "\"";
				}

				stream << " ];\n";
			}

			static void submit( std::ostream & stream
				, AttachmentTransitions const & transitions
				, bool withColours )
			{
				stream << "digraph \"Transitions\" {\n";
				std::set< std::string > nodes;

				for ( auto & transition : transitions.viewTransitions )
				{
					std::string name{ transition.outputAttach.name + "\\ntransition to\\n" + transition.inputAttach.name };
					displayNode( stream, name, "box", imgColour, nodes, withColours );
					std::string srcNode = "ExternalSource";
					std::string_view srcColour = passColour;
					std::string dstNode = "ExternalDestination";
					std::string_view dstColour = passColour;

					if ( transition.outputAttach.pass )
					{
						srcNode = transition.outputAttach.pass->name;
						srcColour = passColour;
					}

					displayNode( stream, srcNode, "ellipse", srcColour, nodes, withColours );
					displayEdge( stream, srcNode, name, transition.view.data->name, imgColour, withColours );

					if ( transition.inputAttach.pass )
					{
						dstNode = transition.inputAttach.pass->name;
						dstColour = passColour;
					}

					displayNode( stream, dstNode, "ellipse", dstColour, nodes, withColours );
					displayEdge( stream, name, dstNode, transition.view.data->name, imgColour, withColours );
				}

				for ( auto & transition : transitions.bufferTransitions )
				{
					std::string name{ transition.outputAttach.name + "\\ntransition to\\n" + transition.inputAttach.name };
					displayNode( stream, name, "box", bufColour, nodes, withColours );
					std::string srcNode = "ExternalSource";
					std::string_view srcColour = passColour;
					std::string dstNode = "ExternalDestination";
					std::string_view dstColour = passColour;

					if ( transition.outputAttach.pass )
					{
						srcNode = transition.outputAttach.pass->name;
						srcColour = passColour;
					}

					displayNode( stream, srcNode, "ellipse", srcColour, nodes, withColours );
					displayEdge( stream, srcNode, name, transition.buffer.name, bufColour, withColours );

					if ( transition.inputAttach.pass )
					{
						dstNode = transition.inputAttach.pass->name;
						dstColour = passColour;
					}

					displayNode( stream, dstNode, "ellipse", dstColour, nodes, withColours );
					displayEdge( stream, name, dstNode, transition.buffer.name, bufColour, withColours );
				}

				stream << "}\n";
			}

		private:
			DotOutVisitor( std::ostream & stream
				, std::set< std::string > & nodes
				, bool withColours
				, std::set< ConstGraphAdjacentNode > & visited )
				: m_stream{ stream }
				, m_nodes{ nodes }
				, m_visited{ visited }
				, m_withColours{ withColours }
			{
			}

			void printEdge( ConstGraphAdjacentNode lhs
				, ConstGraphAdjacentNode rhs )
			{
				std::string sep;
				auto transitions = rhs->getInputAttaches( lhs );
				std::sort( transitions.viewTransitions.begin()
					, transitions.viewTransitions.end()
					, []( ViewTransition const & lhs, ViewTransition const & rhs )
					{
						return lhs.outputAttach.name < rhs.outputAttach.name;
					} );
				std::sort( transitions.bufferTransitions.begin()
					, transitions.bufferTransitions.end()
					, []( BufferTransition const & lhs, BufferTransition const & rhs )
					{
						return lhs.outputAttach.name < rhs.outputAttach.name;
					} );

				for ( auto & transition : transitions.viewTransitions )
				{
					std::string name{ "Transition to\\n" + transition.inputAttach.name };
					displayNode( m_stream, name, "box", imgColour, m_nodes, m_withColours );
					displayNode( m_stream, lhs->getName(), "ellipse", passColour, m_nodes, m_withColours );
					displayNode( m_stream, rhs->getName(), "ellipse", passColour, m_nodes, m_withColours );
					displayEdge( m_stream, lhs->getName(), name, transition.view.data->name, imgColour, m_withColours );
					displayEdge( m_stream, name, rhs->getName(), transition.view.data->name, imgColour, m_withColours );
				}

				for ( auto & transition : transitions.bufferTransitions )
				{
					std::string name{ "Transition to\\n" + transition.inputAttach.name };
					displayNode( m_stream, name, "box", bufColour, m_nodes, m_withColours );
					displayNode( m_stream, lhs->getName(), "ellipse", passColour, m_nodes, m_withColours );
					displayNode( m_stream, rhs->getName(), "ellipse", passColour, m_nodes, m_withColours );
					displayEdge( m_stream, lhs->getName(), name, transition.buffer.name, bufColour, m_withColours );
					displayEdge( m_stream, name, rhs->getName(), transition.buffer.name, bufColour, m_withColours );
				}
			}

			void submit( ConstGraphAdjacentNode node )
			{
				submit( m_stream
					, m_nodes
					, node
					, m_withColours
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
			std::set< std::string > & m_nodes;
			bool m_withColours;
			std::set< GraphNode const * > & m_visited;
		};
	}

	void displayPasses( std::ostream & stream
		, RunnableGraph const & value
		, bool withColours )
	{
		std::set< std::string > nodes;
		DotOutVisitor::submit( stream, nodes, value.getGraph(), withColours );
	}

	void displayTransitions( std::ostream & stream
		, RunnableGraph const & value
		, bool withColours )
	{
		DotOutVisitor::submit( stream, value.getTransitions(), withColours );
	}
}
