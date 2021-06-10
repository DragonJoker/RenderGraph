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

			GraphAdjacentNode insertNodeBefore( GraphNodePtrArray & nodes
				, GraphNodePtr node )
			{
				auto it = std::find_if( nodes.begin()
					, nodes.end()
					, [&node]( GraphNodePtr const & lookup )
					{
						return node->hasInNext( lookup.get() );
					} );
				auto index = std::distance( nodes.begin(), it );
				nodes.emplace( it, std::move( node ) );
				return ( nodes.begin() + index )->get();
			}

			GraphAdjacentNode insertNodeAfter( GraphNodePtrArray & nodes
				, GraphNodePtr node )
			{
				auto it = std::find_if( nodes.rbegin()
					, nodes.rend()
					, [&node]( GraphNodePtr const & lookup )
					{
						return lookup->hasInNext( node.get() );
					} );
				auto index = std::distance( nodes.rbegin(), it );
				nodes.emplace( it.base(), std::move( node ) );
				return ( nodes.rbegin() + index )->get();
			}

			void buildGraph( RootNode & rootNode
				, ViewTransitionArray const & transitions
				, GraphNodePtrArray & nodes )
			{
				for ( auto & transition : transitions )
				{
					GraphAdjacentNode outputAdjNode{};
					GraphAdjacentNode inputAdjNode{};
					GraphNodePtr outputNode{};
					GraphNodePtr inputNode{};
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
							outputNode = std::make_unique< FramePassNode >( *transition.outputAttach.pass );
							outputAdjNode = outputNode.get();
						}
					}
					else
					{
						outputAdjNode = itOutput->get();
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
							inputNode = std::make_unique< FramePassNode >( *transition.inputAttach.pass );
							inputAdjNode = inputNode.get();
						}
					}
					else
					{
						inputAdjNode = itInput->get();
					}

					if ( inputAdjNode
						&& outputAdjNode
						&& transition.inputAttach.pass->dependsOn( *transition.outputAttach.pass, transition.view ) )
					{
						outputAdjNode->attachNode( inputAdjNode, { { transition }, {} } );
					}

					if ( outputNode )
					{
						insertNodeBefore( nodes, std::move( outputNode ) );
					}

					if ( inputNode )
					{
						insertNodeAfter( nodes, std::move( inputNode ) );
					}
				}
			}

			void buildGraph( RootNode & rootNode
				, BufferTransitionArray const & transitions
				, GraphNodePtrArray & nodes )
			{
				for ( auto & transition : transitions )
				{
					GraphAdjacentNode outputAdjNode{};
					GraphAdjacentNode inputAdjNode{};
					GraphNodePtr outputNode{};
					GraphNodePtr inputNode{};
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
							outputNode = std::make_unique< FramePassNode >( *transition.outputAttach.pass );
							outputAdjNode = outputNode.get();
						}
					}
					else
					{
						outputAdjNode = itOutput->get();
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
							inputNode = std::make_unique< FramePassNode >( *transition.inputAttach.pass );
							inputAdjNode = inputNode.get();
						}
					}
					else
					{
						inputAdjNode = itInput->get();
					}

					if ( inputAdjNode
						&& outputAdjNode
						&& transition.inputAttach.pass->dependsOn( *transition.outputAttach.pass, transition.buffer ) )
					{
						outputAdjNode->attachNode( inputAdjNode, { {}, { transition } } );
					}

					if ( outputNode )
					{
						insertNodeBefore( nodes, std::move( outputNode ) );
					}

					if ( inputNode )
					{
						insertNodeAfter( nodes, std::move( inputNode ) );
					}
				}
			}
		}

		GraphNodePtrArray buildGraph( RootNode & rootNode
			, AttachmentTransitions const & transitions )
		{
			GraphNodePtrArray nodes;
			buildGraph( rootNode, transitions.viewTransitions, nodes );
			buildGraph( rootNode, transitions.bufferTransitions, nodes );

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
