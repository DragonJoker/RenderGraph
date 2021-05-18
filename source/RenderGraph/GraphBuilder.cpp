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

			AttachmentTransitionArray buildTransitions( AttachmentArray const & srcOutputs
				, AttachmentArray const & dstInputs
				, FramePass const * srcPass
				, FramePass const * dstPass )
			{
				AttachmentTransitionArray result;

				if ( srcPass == nullptr )
				{
					auto dstInputIt = dstInputs.begin();
					auto end = dstInputs.end();
					std::set< FramePass const * > dstPasses{ dstPass };

					while ( dstInputIt != end )
					{
						result.push_back( AttachmentTransition
							{
								{},
								{ *dstInputIt, dstPasses },
							} );
						++dstInputIt;
					}
				}
				else if ( dstPass == nullptr )
				{
					auto srcOutputIt = srcOutputs.begin();
					auto end = srcOutputs.end();
					std::set< FramePass const * > srcPasses{ srcPass };

					while ( srcOutputIt != end )
					{
						result.push_back( AttachmentTransition
							{
								{ { *srcOutputIt, srcPasses } },
								{ Attachment::createDefault(), {} },
							} );
						++srcOutputIt;
					}
				}
				else
				{
					assert( srcOutputs.size() == dstInputs.size() );
					auto srcOutputIt = srcOutputs.begin();
					auto end = srcOutputs.end();
					auto dstInputIt = dstInputs.begin();
					std::set< FramePass const * > srcPasses{ srcPass };
					std::set< FramePass const * > dstPasses{ dstPass };

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
				}

				return mergeIdenticalTransitions( std::move( result ) );
			}

			void buildGraphRec( FramePass const * curr
				, AttachmentTransitionArray prevTransitions
				, FramePassDependenciesArray const & dependencies
				, GraphNodePtrArray & nodes
				, RootNode & fullGraph
				, AttachmentTransitionArray & allTransitions
				, GraphAdjacentNode prevNode )
			{
				// We want the dependencies for which the current pass is the source.
				std::set< FramePassDependencies const * > attaches;
				FramePassDependenciesArray nextDependencies;
				filter< FramePassDependencies >( dependencies
					, [&curr]( FramePassDependencies const & lookup )
					{
						return lookup.srcPass
							&& curr->name == lookup.srcPass->name;
					}
					, [&attaches]( FramePassDependencies const & lookup )
					{
						attaches.insert( &lookup );
					}
					, [&nextDependencies]( FramePassDependencies const & lookup )
					{
						nextDependencies.push_back( lookup );
					} );

				GraphAdjacentNode result{ createNode( curr, nodes ) };
				allTransitions.insert( allTransitions.end()
					, prevTransitions.begin()
					, prevTransitions.end() );

				if ( prevNode->getKind() == GraphNode::Kind::Root )
				{
					prevNode->attachNode( result
						, std::move( prevTransitions ) );

					for ( auto & dependency : nextDependencies )
					{
						auto transitions = buildTransitions( dependency.srcOutputs
							, dependency.dstInputs
							, dependency.srcPass
							, dependency.dstPass );
						allTransitions.insert( allTransitions.end()
							, transitions.begin()
							, transitions.end() );
					}
				}
				else if ( curr != getFramePass( *prevNode ) )
				{
					prevNode->attachNode( result
						, std::move( prevTransitions ) );
				}

				for ( auto & dependency : attaches )
				{
					auto transitions = buildTransitions( dependency->srcOutputs
						, dependency->dstInputs
						, curr
						, dependency->dstPass );

					if ( dependency->dstPass )
					{
						buildGraphRec( dependency->dstPass
							, std::move( transitions )
							, nextDependencies
							, nodes
							, fullGraph
							, allTransitions
							, result );
					}
					else
					{
						allTransitions.insert( allTransitions.end()
							, transitions.begin()
							, transitions.end() );
					}
				}
			}
		}

		GraphNodePtrArray buildGraph( std::vector< FramePassPtr > const & passes
			, RootNode & rootNode
			, AttachmentTransitionArray & transitions
			, FramePassDependenciesArray const & dependencies )
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
					, transitions
					, curr );
			}

			transitions = mergeIdenticalTransitions( std::move( transitions ) );
			transitions = mergeTransitionsPerInput( std::move( transitions ) );
			transitions = reduceDirectPaths( std::move( transitions ) );
			return nodes;
		}
	}
}
