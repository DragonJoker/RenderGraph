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

			void buildGraphRec( FramePass const * curr
				, AttachmentTransitionArray const & transitions
				, GraphNodePtrArray & nodes
				, RootNode & fullGraph
				, GraphAdjacentNode prevNode )
			{
				AttachmentTransitionArray inputTransitions;
				AttachmentTransitionArray nextTransitions;
				FramePassSet dstPasses;

				// List the transitions for which the current pass is the source.
				for ( auto & transition : transitions )
				{
					if ( curr == transition.dstAttach.pass )
					{
						inputTransitions.push_back( transition );
					}
					else
					{
						nextTransitions.push_back( transition );

						if ( curr == transition.srcAttach.pass
							&& transition.dstAttach.pass )
						{
							dstPasses.insert( transition.dstAttach.pass );
						}
					}
				}

				GraphAdjacentNode result{ createNode( curr, nodes ) };

				if ( prevNode->getKind() == GraphNode::Kind::Root
					|| curr != getFramePass( *prevNode ) )
				{
					prevNode->attachNode( result
						, std::move( inputTransitions ) );
				}

				for ( auto & dstPass : dstPasses )
				{
					buildGraphRec( dstPass
						, nextTransitions
						, nodes
						, fullGraph
						, result );
				}
			}
		}

		GraphNodePtrArray buildGraph( RootNode & rootNode
			, FramePassDependenciesMap const & dependencies
			, AttachmentTransitionArray const & transitions )
		{
			GraphNodePtrArray nodes;
			// Retrieve root and leave passes.
			auto roots = retrieveRoots( dependencies );

			if ( roots.empty() )
			{
				CRG_Exception( "No root to start with" );
			}

			auto leaves = retrieveLeafs( dependencies );

			if ( leaves.empty() )
			{
				CRG_Exception( "No leaf to end with" );
			}

			// Build paths from each root pass to leaf pass
			GraphAdjacentNode curr{ &rootNode };

			for ( auto & root : roots )
			{
				buildGraphRec( root
					, transitions
					, nodes
					, rootNode
					, curr );
			}

			return nodes;
		}
	}
}
