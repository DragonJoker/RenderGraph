/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#include "RenderGraph/FrameGraph.hpp"

#include "RenderGraph/Exception.hpp"
#include "RenderGraph/FramePass.hpp"
#include "RenderGraph/FramePassGroup.hpp"
#include "RenderGraph/Log.hpp"
#include "RenderGraph/ResourceHandler.hpp"
#include "RenderGraph/RunnableGraph.hpp"
#include "FramePassDependenciesBuilder.hpp"
#include "GraphBuilder.hpp"

#include <algorithm>

namespace crg
{
	namespace fgph
	{
		static FramePassArray sortPasses( FramePassArray const & passes )
		{
			FramePassArray sortedPasses;
			FramePassArray unsortedPasses;

			for ( auto & pass : passes )
			{
				if ( pass->passDepends.empty() )
				{
					sortedPasses.push_back( pass );
				}
				else
				{
					unsortedPasses.push_back( pass );
				}
			}

			if ( sortedPasses.empty() )
			{
				sortedPasses.push_back( unsortedPasses.front() );
				unsortedPasses.erase( unsortedPasses.begin() );
			}

			while ( !unsortedPasses.empty() )
			{
				FramePassArray currentPasses;
				std::swap( currentPasses, unsortedPasses );
				bool added = false;

				for ( auto & pass : currentPasses )
				{
#if !defined( NDEBUG )
					bool processed = false;
#endif
					// Only process this pass if all its dependencies have been processed.
					if ( !std::all_of( pass->passDepends.begin()
						, pass->passDepends.end()
						, [&sortedPasses]( FramePass const * lookup )
						{
							return sortedPasses.end() != std::find( sortedPasses.begin()
								, sortedPasses.end()
								, lookup );
						} ) )
					{
						unsortedPasses.push_back( pass );
#if !defined( NDEBUG )
						processed = true;
#endif
					}
					else if ( auto it = std::find_if( sortedPasses.begin()
						, sortedPasses.end()
						, [&pass]( FramePass const * lookup )
						{
							return lookup->dependsOn( *pass );
						} );
						it != sortedPasses.end() )
					{
						sortedPasses.insert( it, pass );
						added = true;
#if !defined( NDEBUG )
						processed = true;
#endif
					}
					else if ( auto rit = std::find_if( sortedPasses.rbegin()
						, sortedPasses.rend()
						, [&pass]( FramePass const * lookup )
						{
							return pass->dependsOn( *lookup );
						} );
						rit != sortedPasses.rend() )
					{
						sortedPasses.insert( rit.base(), pass );
						added = true;
#if !defined( NDEBUG )
						processed = true;
#endif
					}

					assert( processed && "Couldn't process pass" );
				}

				if ( !added )
				{
					Logger::logError( "Couldn't sort passes" );
					CRG_Exception( "Couldn't sort passes" );
				}
			}

			return sortedPasses;
		}
	}

	FrameGraph::FrameGraph( ResourceHandler & handler
		, std::string name )
		: m_handler{ handler }
		, m_name{ std::move( name ) }
		, m_defaultGroup{ new FramePassGroup{ *this, 0u, m_name } }
		, m_finalState{ handler }
	{
	}

	FramePass & FrameGraph::createPass( std::string const & name
		, RunnablePassCreator runnableCreator )
	{
		return m_defaultGroup->createPass( name, std::move( runnableCreator ) );
	}

	FramePassGroup & FrameGraph::createPassGroup( std::string const & groupName )
	{
		return m_defaultGroup->createPassGroup( groupName );
	}

	ImageId FrameGraph::createImage( ImageData const & img )
	{
		auto result = m_handler.createImageId( img );
		m_images.insert( result );
		return result;
	}

	ImageViewId FrameGraph::createView( ImageViewData const & view )
	{
		auto result = m_handler.createViewId( view );
		m_imageViews.insert( result );
		return result;
	}

	RunnableGraphPtr FrameGraph::compile( GraphContext & context )
	{
		FramePassArray passes;
		m_defaultGroup->listPasses( passes );

		if ( passes.empty() )
		{
			Logger::logWarning( "No FramePass registered." );
			CRG_Exception( "No FramePass registered." );
		}

		passes = fgph::sortPasses( passes );
		GraphNodePtrArray nodes;

		for ( auto & pass : passes )
		{
			auto node = std::make_unique< FramePassNode >( *pass );
			nodes.emplace_back( std::move( node ) );
		}

		FramePassDependencies inputTransitions;
		FramePassDependencies outputTransitions;
		AttachmentTransitions transitions;
		PassDependencyCache imgDepsCache;
		PassDependencyCache bufDepsCache;
		builder::buildPassAttachDependencies( nodes
			, imgDepsCache
			, bufDepsCache
			, inputTransitions
			, outputTransitions
			, transitions );
		RootNode root{ *this };
		builder::buildGraph( root
			, nodes
			, imgDepsCache
			, bufDepsCache
			, transitions );
		ImageMemoryMap images;
		ImageViewMap imageViews;
		return std::make_unique< RunnableGraph >( *this
			, std::move( transitions )
			, std::move( nodes )
			, std::move( root )
			, context );
	}

	LayoutState FrameGraph::getFinalLayoutState( ImageId image
		, ImageViewType viewType
		, VkImageSubresourceRange const & range )const
	{
		return m_finalState.getLayoutState( image, viewType, range );
	}

	LayoutState FrameGraph::getFinalLayoutState( ImageViewId view
		, uint32_t passIndex )const
	{
		if ( view.data->source.empty() )
		{
			return getFinalLayoutState( view.data->image
				, convert( view.data->info.viewType )
				, view.data->info.subresourceRange );
		}

		return getFinalLayoutState( view.data->source[passIndex], 0u );
	}

	AccessState const & FrameGraph::getFinalAccessState( Buffer const & buffer
		, uint32_t passIndex )const
	{
		return m_finalState.getAccessState( buffer.buffer( passIndex ), { 0u, VK_WHOLE_SIZE } );
	}

	void FrameGraph::addInput( ImageId image
		, ImageViewType viewType
		, VkImageSubresourceRange const & range
		, LayoutState const & outputLayout )
	{
		m_inputs.setLayoutState( image
			, viewType
			, range
			, outputLayout );
	}

	void FrameGraph::addInput( ImageViewId view
		, LayoutState const & outputLayout )
	{
		addInput( view.data->image
			, convert( view.data->info.viewType )
			, view.data->info.subresourceRange
			, outputLayout );
	}

	LayoutState FrameGraph::getInputLayoutState( ImageId image
		, ImageViewType viewType
		, VkImageSubresourceRange const & range )const
	{
		return m_inputs.getLayoutState( image
			, viewType
			, range );
	}

	LayoutState FrameGraph::getInputLayoutState( ImageViewId view )const
	{
		return getInputLayoutState( view.data->image
			, convert( view.data->info.viewType )
			, view.data->info.subresourceRange );
	}

	void FrameGraph::addOutput( ImageId image
		, ImageViewType viewType
		, VkImageSubresourceRange const & range
		, LayoutState const & outputLayout )
	{
		m_outputs.setLayoutState( image
			, viewType
			, range
			, outputLayout );
	}

	void FrameGraph::addOutput( ImageViewId view
		, LayoutState const & outputLayout )
	{
		addOutput( view.data->image
			, convert( view.data->info.viewType )
			, view.data->info.subresourceRange
			, outputLayout );
	}

	LayoutState FrameGraph::getOutputLayoutState( ImageId image
		, ImageViewType viewType
		, VkImageSubresourceRange const & range )const
	{
		return m_outputs.getLayoutState( image
			, viewType
			, range );
	}

	LayoutState FrameGraph::getOutputLayoutState( ImageViewId view )const
	{
		return getOutputLayoutState( view.data->image
			, convert( view.data->info.viewType )
			, view.data->info.subresourceRange );
	}

	LayerLayoutStatesMap const & FrameGraph::getOutputLayoutStates()const
	{
		return m_outputs.images;
	}

	void FrameGraph::registerFinalState( RecordContext const & context )
	{
		m_finalState = context;
	}
}
