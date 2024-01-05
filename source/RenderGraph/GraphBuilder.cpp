/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#include "GraphBuilder.hpp"

#include "RenderGraph/FramePass.hpp"
#include "RenderGraph/GraphNode.hpp"

#include <algorithm>

namespace crg
{
	namespace builder
	{
		namespace graph
		{
			static GraphAdjacentNode findNode( FramePass const * pass
				, GraphNodePtrArray const & nodes )
			{
				if ( !pass )
				{
					return nullptr;
				}

				GraphAdjacentNode result{};
				auto it = std::find_if( nodes.begin()
					, nodes.end()
					, [pass]( auto & lookup )
					{
						return getFramePass( *lookup ) == pass;
					} );

				assert( it != nodes.end() );
				result = it->get();
				return result;
			}

			static AttachmentTransitions makeTransition( ViewTransition const & transition )
			{
				return { { transition }, {} };
			}

			static AttachmentTransitions makeTransition( BufferTransition const & transition )
			{
				return { {}, { transition } };
			}

			template< typename DataT >
			static void buildGraph( DataTransitionArrayT< DataT > const & transitions
				, GraphNodePtrArray const & nodes
				, PassDependencyCache & depsCache )
			{
				for ( DataTransitionT< DataT > const & transition : transitions )
				{
					GraphAdjacentNode outputAdjNode = findNode( transition.outputAttach.pass
						, nodes );
					GraphAdjacentNode inputAdjNode = findNode( transition.inputAttach.pass
						, nodes );

					if ( inputAdjNode
						&& outputAdjNode
						&& transition.inputAttach.pass->dependsOn( *transition.outputAttach.pass, transition.data, depsCache ) )
					{
						outputAdjNode->attachNode( inputAdjNode
							, makeTransition( transition ) );
					}
				}
			}
		}

		void buildGraph( RootNode & rootNode
			, GraphNodePtrArray const & nodes
			, PassDependencyCache & imgDepsCache
			, PassDependencyCache & bufDepsCache
			, AttachmentTransitions & transitions )
		{
			graph::buildGraph( transitions.viewTransitions, nodes, imgDepsCache );
			graph::buildGraph( transitions.bufferTransitions, nodes, bufDepsCache );

			for ( auto & node : nodes )
			{
				if ( !node->hasPrev() )
				{
					rootNode.attachNode( node.get(), {} );
				}
			}
		}
	}
}
