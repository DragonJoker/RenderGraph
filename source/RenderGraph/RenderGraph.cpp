/*
This file belongs to RenderGraph.
See LICENSE file in root folder.
*/
#include "RenderGraph/RenderGraph.hpp"

#include "RenderGraph/Exception.hpp"
#include "RenderGraph/RenderPass.hpp"

#include <algorithm>
#include <functional>
#include <iostream>
#include <stdexcept>

namespace crg
{
	namespace details
	{
		struct PassAttach
		{
			Attachment const attach;
			std::vector< RenderPass const * > passes;
		};

		using PassAttachCont = std::vector< PassAttach >;

		template< typename TypeT >
		void filter( std::vector< TypeT > const & inputs
			, std::function< bool( TypeT const & ) > filterFunc
			, std::function< void( TypeT const & ) > trueFunc
			, std::function< void( TypeT const & ) > falseFunc = []( TypeT const & )
			{} )
		{
			for ( auto & input : inputs )
			{
				if ( filterFunc( input ) )
				{
					trueFunc( input );
				}
				else
				{
					falseFunc( input );
				}
			}
		}
		
		template< typename TypeT >
		void filter( std::vector< TypeT > & inputs
			, std::function< bool( TypeT & ) > filterFunc
			, std::function< void( TypeT & ) > trueFunc
			, std::function< void( TypeT & ) > falseFunc = []( TypeT & ){} )
		{
			for ( auto & input : inputs )
			{
				if ( filterFunc( input ) )
				{
					trueFunc( input );
				}
				else
				{
					falseFunc( input );
				}
			}
		}

		inline bool isInRange( uint32_t value
			, uint32_t left
			, uint32_t count )
		{
			return value >= left && value < left + count;
		}

		inline bool areIntersecting( uint32_t lhsLBound
			, uint32_t lhsCount
			, uint32_t rhsLBound
			, uint32_t rhsCount )
		{
			return isInRange( lhsLBound, rhsLBound, rhsCount )
				|| isInRange( rhsLBound, lhsLBound, lhsCount );
		}
		
		inline bool areIntersecting( VkImageSubresourceRange const & lhs
			, VkImageSubresourceRange const & rhs )
		{
			return areIntersecting( lhs.baseMipLevel
					, lhs.levelCount
					, rhs.baseMipLevel
					, rhs.levelCount )
				&& areIntersecting( lhs.baseArrayLayer
					, lhs.layerCount
					, rhs.baseArrayLayer
					, lhs.layerCount );
		}

		inline bool areOverlapping( ImageViewId const & lhs
			, ImageViewId const & rhs )
		{
			return lhs.data->image == rhs.data->image
				&& areIntersecting( lhs.data->subresourceRange
					, rhs.data->subresourceRange );
		}

		void processAttach( Attachment const & attach
			, RenderPass const & pass
			, PassAttachCont & cont
			, std::function< bool( Attachment const & ) > processAttach )
		{
			bool found{ false };
			std::vector< RenderPass const * > passes;

			for ( auto & lookup : cont )
			{
				if ( processAttach( lookup.attach ) )
				{
					if ( lookup.passes.end() == std::find( lookup.passes.begin()
						, lookup.passes.end()
						, &pass ) )
					{
						lookup.passes.push_back( &pass );
					}

					found = true;
				}
			}

			auto it = std::find_if( cont.begin()
				, cont.end()
				, [&attach]( PassAttach const & lookup )
				{
					return lookup.attach.name == attach.name
						&& lookup.attach.view == attach.view;
				} );

			if ( cont.end() == it )
			{
				cont.push_back( PassAttach{ attach } );
				it = std::prev( cont.end() );
			}

			it->passes.push_back( &pass );
		}

		void processTargetAttach( Attachment const & attach
			, RenderPass const & pass
			, PassAttachCont & cont )
		{
			if ( attach.storeOp == VK_ATTACHMENT_STORE_OP_STORE )
			{
				return processAttach( attach
					, pass
					, cont
					, [&attach]( Attachment const & lookup )
					{
						return areOverlapping( lookup.view, attach.view );
					} );
			}
		}

		void processDepthStencilTargetAttach( Attachment const & attach
			, RenderPass const & pass
			, PassAttachCont & cont )
		{
			if ( attach.storeOp == VK_ATTACHMENT_STORE_OP_STORE
				|| attach.stencilStoreOp == VK_ATTACHMENT_STORE_OP_STORE )
			{
				processAttach( attach
					, pass
					, cont
					, [&attach]( Attachment const & lookup )
					{
						return areOverlapping( lookup.view, attach.view );
					} );
			}
		}

		void processTargetAsInputAttach( Attachment const & attach
			, RenderPass const & pass
			, PassAttachCont & cont )
		{
			if ( attach.loadOp == VK_ATTACHMENT_LOAD_OP_LOAD )
			{
				processAttach( attach
					, pass
					, cont
					, [&attach]( Attachment const & lookup )
					{
						return areOverlapping( lookup.view, attach.view );
					} );
			}
		}

		void processInputAttach( Attachment const & attach
			, RenderPass const & pass
			, PassAttachCont & cont )
		{
			return processAttach( attach
				, pass
				, cont
				, [&attach]( Attachment const & lookup )
				{
					return areOverlapping( lookup.view, attach.view );
				} );
		}

		void processTargetAttachs( AttachmentArray const & attachs
			, RenderPass const & pass
			, PassAttachCont & cont )
		{
			for ( auto & attach : attachs )
			{
				processTargetAttach( attach, pass, cont );
			}
		}

		void processTargetAsInputAttachs( AttachmentArray const & attachs
			, RenderPass const & pass
			, PassAttachCont & cont )
		{
			for ( auto & attach : attachs )
			{
				processTargetAsInputAttach( attach, pass, cont );
			}
		}

		void processInputAttachs( AttachmentArray const & attachs
			, RenderPass const & pass
			, PassAttachCont & cont )
		{
			for ( auto & attach : attachs )
			{
				processInputAttach( attach, pass, cont );
			}
		}

		void addDependency( Attachment const & attach
			, std::vector< RenderPass const * > const & srcs
			, std::vector< RenderPass const * > const & dsts
			, RenderPassDependenciesArray & dependencies )
		{
			for ( auto & src : srcs )
			{
				std::vector< RenderPassDependencies * > srcDependencies;
				filter< RenderPassDependencies >( dependencies
					, [&src]( RenderPassDependencies & lookup )
					{
						return lookup.srcPass == src;
					}
					, [&srcDependencies]( RenderPassDependencies & lookup )
					{
						srcDependencies.push_back( &lookup );
					} );

				for ( auto & dst : dsts )
				{
					if ( src != dst )
					{
						auto it = std::find_if( srcDependencies.begin()
							, srcDependencies.end()
							, [&dst]( RenderPassDependencies * lookup )
							{
								return lookup->dstPass == dst;
							} );

						if ( it == srcDependencies.end() )
						{
							dependencies.push_back( { src, dst } );
							srcDependencies.push_back( &dependencies.back() );
							it = std::prev( srcDependencies.end() );
						}

						auto & dep = *it;

						if ( dep->dependencies.end() == std::find( dep->dependencies.begin()
							, dep->dependencies.end()
							, attach ) )
						{
							dep->dependencies.push_back( attach );
						}
					}
				}
			}
		}

		RenderPassDependenciesArray buildPassDependencies( std::vector< RenderPassPtr > const & passes )
		{
			PassAttachCont inputs;
			PassAttachCont outputs;

			for ( auto & pass : passes )
			{
				processInputAttachs( pass->inputs, *pass, inputs );
				processTargetAttachs( pass->colourOutputs, *pass, outputs );
				processTargetAsInputAttachs( pass->colourOutputs, *pass, inputs );

				if ( pass->depthStencilOutput )
				{
					processDepthStencilTargetAttach( *pass->depthStencilOutput, *pass, outputs );
					processTargetAsInputAttach( *pass->depthStencilOutput, *pass, inputs );
				}
			}

			RenderPassDependenciesArray result;

			for ( auto & input : inputs )
			{
				for ( auto & output : outputs )
				{
					if ( areOverlapping( output.attach.view, input.attach.view ) )
					{
						addDependency( output.attach
							, output.passes
							, input.passes
							, result );
					}
				}
			}

			return result;
		}

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

		template< typename TypeT >
		TypeT * getPointer( std::unique_ptr< TypeT > const & v )
		{
			return v.get();
		}

		template< typename TypeT >
		TypeT * getPointer( TypeT * const v )
		{
			return v;
		}

		template< typename TypeT >
		TypeT * getPointer( TypeT & v )
		{
			return &v;
		}

		template< typename TypeT >
		std::vector< TypeT > filterOut( std::vector< TypeT > & inputs
			, std::function< bool( TypeT & ) > filterFunc )
		{
			std::vector< TypeT > result;
			auto it = inputs.begin();

			while ( it != inputs.end() )
			{
				auto & input = *it;

				if ( filterFunc( input ) )
				{
					result.push_back( std::move( input ) );
					it = inputs.erase( it );
				}
				else
				{
					++it;
				}
			}

			return result;
		}

		using RenderPassSet = std::set< RenderPass const * >;

		RenderPassSet retrieveRoots( RenderPassPtrArray const & passes
			, RenderPassDependenciesArray const & dependencies )
		{
			RenderPassSet result;
			filter< RenderPassPtr >( passes
				, [&dependencies]( RenderPassPtr const & pass )
				{
					// We want the passes that are not listed as destination to other passes.
					return dependencies.end() == std::find_if( dependencies.begin()
						, dependencies.end()
						, [&pass]( RenderPassDependencies const & lookup )
						{
							return lookup.dstPass == pass.get();
						} );
				}
				, [&result]( RenderPassPtr const & lookup )
				{
					result.insert( lookup.get() );
				} );
			return result;
		}

		RenderPassSet retrieveLeafs( RenderPassPtrArray const & passes
			, RenderPassDependenciesArray const & dependencies )
		{
			RenderPassSet result;
			filter< RenderPassPtr >( passes
				, [&dependencies]( RenderPassPtr const & pass )
				{
					// We want the passes that are not listed as source to other passes.
					return dependencies.end() == std::find_if( dependencies.begin()
						, dependencies.end()
						, [&pass]( RenderPassDependencies const & lookup )
						{
							return lookup.srcPass == pass.get();
						} );
				}
				, [&result]( RenderPassPtr const & lookup )
				{
					result.insert( lookup.get() );
				} );
			return result;
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

		void buildGraphRec( RenderPass const * curr
			, AttachmentArray prevAttaches
			, RenderPassDependenciesArray const & dependencies
			, GraphNodePtrArray & nodes
			, RootNode & fullGraph
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
				prevNode->attachNode( result, std::move( prevAttaches ) );

				for ( auto & dependency : attaches )
				{
					buildGraphRec( dependency->dstPass
						, dependency->dependencies
						, nextDependencies
						, nodes
						, fullGraph
						, result );
				}
			}
		}

		GraphNodePtrArray buildGraph( std::vector< RenderPassPtr > const & passes
			, RootNode & rootNode
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
			// When an existing graph node is to be added again,
			// the node that was added is replaced by a pi node which groups the nodes with the same name (render pass name).
			// This leads to a grouping of all paths to one node,
			// but lets each other kind of node to have a single next and a single previous.
			GraphAdjacentNode curr{ &rootNode };

			for ( auto & root : roots )
			{
				buildGraphRec( root
					, {}
					, dependencies
					, nodes
					, rootNode
					, curr );
			}

			return nodes;
		}
	}

	RenderGraph::RenderGraph( std::string name )
		: m_root{ std::move( name ) }
	{
	}

	void RenderGraph::add( RenderPass const & pass )
	{
		if ( m_passes.end() != std::find_if( m_passes.begin()
			, m_passes.end()
			, [&pass]( RenderPassPtr const & lookup )
			{
				return lookup->name == pass.name;
			} ) )
		{
			CRG_Exception( "Duplicate RenderPass name detected." );
		}

		m_passes.push_back( std::make_unique< RenderPass >( pass ) );
	}

	void RenderGraph::remove( RenderPass const & pass )
	{
		auto it = std::find_if( m_passes.begin()
			, m_passes.end()
			, [&pass]( RenderPassPtr const & lookup )
			{
				return lookup->name == pass.name;
			} );

		if ( m_passes.end() == it )
		{
			CRG_Exception( "RenderPass was not found." );
		}

		m_passes.erase( it );
	}

	bool RenderGraph::compile()
	{
		if ( m_passes.empty() )
		{
			CRG_Exception( "No RenderPass registered." );
		}

		auto dependencies = details::buildPassDependencies( m_passes );
		m_nodes = details::buildGraph( m_passes, m_root, dependencies );
		return true;
	}

	ImageId RenderGraph::createImage( ImageData const & img )
	{
		auto data = std::make_unique< ImageData >( img );
		ImageId result{ uint32_t( m_images.size() + 1u ), data.get() };
		m_images.insert( { result, std::move( data ) } );
		return result;
	}

	ImageViewId RenderGraph::createView( ImageViewData const & img )
	{
		auto data = std::make_unique< ImageViewData >( img );
		ImageViewId result{ uint32_t( m_imageViews.size() + 1u ), data.get() };
		m_imageViews.insert( { result, std::move( data ) } );
		return result;
	}
}
