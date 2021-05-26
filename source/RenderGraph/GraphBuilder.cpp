/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#include "GraphBuilder.hpp"

#include "RenderGraph/Exception.hpp"
#include "RenderGraph/FramePass.hpp"
#include "RenderGraph/GraphNode.hpp"

#include <algorithm>

namespace crg
{
	namespace builder
	{
		namespace
		{
			template< typename PredT >
			GraphAdjacentNode findIf( GraphNodePtrArray const & nodes
				, GraphNode::Kind kind
				, PredT predicate )
			{
				GraphAdjacentNode result{ nullptr };
				auto it = std::find_if( nodes.begin()
					, nodes.end()
					, [&kind, &predicate]( GraphNodePtr const & lookup )
					{
						return ( ( kind == GraphNode::Kind::Undefined || kind == lookup->getKind() )
							&& predicate( lookup.get() ) );
					} );

				if ( it == nodes.end() )
				{
					return nullptr;
				}

				return it->get();
			}

			GraphAdjacentNode find( FramePass const * pass
				, GraphNodePtrArray const & nodes )
			{
				return findIf( nodes
					, GraphNode::Kind::FramePass
					, [&pass]( GraphAdjacentNode lookup )
					{
						return ( pass == &nodeCast< FramePassNode >( *lookup ).getFramePass() );
					} );
			}

			GraphAdjacentNode createNode( FramePass const * pass
				, GraphNodePtrArray & nodes )
			{
				auto result = find( pass, nodes );

				if ( !result )
				{
					nodes.push_back( std::make_unique< FramePassNode >( *pass ) );
					result = nodes.back().get();
				}

				return result;
			}
		}

		GraphNodePtrArray buildGraph( RootNode & rootNode
			, AttachmentTransitionArray const & transitions )
		{
			GraphNodePtrArray nodes;

			for ( auto & transition : transitions )
			{
				GraphAdjacentNode outputNode{};
				GraphAdjacentNode inputNode{};
				auto itOutput = std::find_if( nodes.begin()
					, nodes.end()
					, [&transition]( auto & lookup )
					{
						return getFramePass( *lookup ) == transition.outputAttach.pass;
					} );

				if ( itOutput == nodes.end() )
				{
					if ( transition.outputAttach.pass )
					{
						nodes.push_back( std::make_unique< FramePassNode >( *transition.outputAttach.pass ) );
						outputNode = nodes.back().get();
					}
				}
				else
				{
					outputNode = itOutput->get();
				}

				auto itInput = std::find_if( nodes.begin()
					, nodes.end()
					, [&transition]( auto & lookup )
					{
						return getFramePass( *lookup ) == transition.inputAttach.pass;
					} );

				if ( itInput == nodes.end() )
				{
					if ( transition.inputAttach.pass )
					{
						nodes.push_back( std::make_unique< FramePassNode >( *transition.inputAttach.pass ) );
						inputNode = nodes.back().get();
					}
				}
				else
				{
					inputNode = itInput->get();
				}

				if ( inputNode
					&& outputNode
					&& transition.inputAttach.pass->dependsOn( *transition.outputAttach.pass ) )
				{
					outputNode->attachNode( inputNode, { transition } );
				}
			}

			for ( auto & node : nodes )
			{
				if ( !node->hasPrev() )
				{
					rootNode.attachNode( node.get(), {} );
				}
			}

			return nodes;
		}
	}
}
