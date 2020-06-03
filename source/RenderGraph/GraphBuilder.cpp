/*
This file belongs to RenderGraph.
See LICENSE file in root folder.
*/
#include "GraphBuilder.hpp"

#include "RenderGraph/Exception.hpp"
#include "RenderGraph/GraphNode.hpp"
#include "RenderGraph/RenderPass.hpp"

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

			GraphAdjacentNode find( RenderPass const * pass
				, GraphNodePtrArray const & nodes )
			{
				return findIf( nodes
					, GraphNode::Kind::RenderPass
					, [&pass]( GraphAdjacentNode lookup )
					{
						return ( pass == &nodeCast< RenderPassNode >( *lookup ).getRenderPass() );
					} );
			}

			GraphAdjacentNode find( std::string const & name
				, GraphNodePtrArray const & nodes )
			{
				return findIf( nodes
					, GraphNode::Kind::Undefined
					, [&name]( GraphAdjacentNode lookup )
					{
						return name == lookup->getName();
					} );
			}

			GraphAdjacentNode createNode( RenderPass const * pass
				, GraphNodePtrArray & nodes )
			{
				auto result = find( pass, nodes );

				if ( !result )
				{
					nodes.push_back( std::make_unique< RenderPassNode >( *pass ) );
					result = nodes.back().get();
				}

				return result;
			}

			AttachmentTransitionArray buildTransitions( AttachmentArray const & srcOutputs
				, AttachmentArray const & dstInputs
				, RenderPass const * srcPass
				, RenderPass const * dstPass )
			{
				assert( srcOutputs.size() == dstInputs.size() );
				AttachmentTransitionArray result;
				auto srcOutputIt = srcOutputs.begin();
				auto end = srcOutputs.end();
				auto dstInputIt = dstInputs.begin();
				std::set< RenderPass const * > srcPasses{ srcPass };
				std::set< RenderPass const * > dstPasses{ dstPass };

				while ( srcOutputIt != end )
				{
					result.push_back( AttachmentTransition
						{
							{ { *srcOutputIt, srcPasses } },
							{ *dstInputIt, dstPasses },
						} );
					++srcOutputIt;
					++dstInputIt;
				}

				return mergeIdenticalTransitions( std::move( result ) );
			}

			void buildGraphRec( RenderPass const * curr
				, AttachmentTransitionArray prevAttaches
				, RenderPassDependenciesArray const & dependencies
				, GraphNodePtrArray & nodes
				, RootNode & fullGraph
				, AttachmentTransitionArray & allAttaches
				, GraphAdjacentNode prevNode )
			{
				if ( prevNode->getKind() == GraphNode::Kind::Root
					|| curr != getRenderPass( *prevNode ) )
				{
					// We want the dependencies for which the current pass is the source.
					std::set< RenderPassDependencies const * > attaches;
					RenderPassDependenciesArray nextDependencies;
					filter< RenderPassDependencies >( dependencies
						, [&curr]( RenderPassDependencies const & lookup )
						{
							return curr->name == lookup.srcPass->name;
						}
						, [&attaches]( RenderPassDependencies const & lookup )
						{
							attaches.insert( &lookup );
						}
							, [&nextDependencies]( RenderPassDependencies const & lookup )
						{
							nextDependencies.push_back( lookup );
						} );

					GraphAdjacentNode result{ createNode( curr, nodes ) };
					allAttaches.insert( allAttaches.end()
						, prevAttaches.begin()
						, prevAttaches.end() );
					prevNode->attachNode( result
						, std::move( prevAttaches ) );

					for ( auto & dependency : attaches )
					{
						buildGraphRec( dependency->dstPass
							, buildTransitions( dependency->srcOutputs
								, dependency->dstInputs
								, curr
								, dependency->dstPass )
							, nextDependencies
							, nodes
							, fullGraph
							, allAttaches
							, result );
					}
				}
			}
		}

		GraphNodePtrArray buildGraph( std::vector< RenderPassPtr > const & passes
			, RootNode & rootNode
			, AttachmentTransitionArray & allAttaches
			, RenderPassDependenciesArray const & dependencies )
		{
			GraphNodePtrArray nodes;
			// Retrieve root and leave passes.
			auto roots = retrieveRoots( passes, dependencies );

			if ( roots.empty() )
			{
				CRG_Exception( "No root to start with" );
			}

			auto leaves = retrieveLeafs( passes, dependencies );

			if ( leaves.empty() )
			{
				CRG_Exception( "No leaf to end with" );
			}

			// Build paths from each root pass to leaf pass
			GraphAdjacentNode curr{ &rootNode };

			for ( auto & root : roots )
			{
				buildGraphRec( root
					, {}
					, dependencies
					, nodes
					, rootNode
					, allAttaches
					, curr );
			}

			allAttaches = mergeIdenticalTransitions( std::move( allAttaches ) );
			allAttaches = mergeTransitionsPerInput( std::move( allAttaches ) );
			allAttaches = reduceDirectPaths( std::move( allAttaches ) );
			return nodes;
		}
	}
}
