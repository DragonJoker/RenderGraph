/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#include "RenderGraph/FrameGraph.hpp"

#include "RenderGraph/Exception.hpp"
#include "RenderGraph/FramePass.hpp"
#include "RenderGraph/RunnableGraph.hpp"
#include "FramePassDependenciesBuilder.hpp"
#include "GraphBuilder.hpp"
#include "ResourceOptimiser.hpp"

#include <algorithm>

namespace crg
{
	FrameGraph::FrameGraph( std::string name )
		: m_name{ std::move( name ) }
	{
	}

	FramePass & FrameGraph::createPass( std::string const & name
		, RunnablePassCreator runnableCreator )
	{
		FramePassPtr pass{ new FramePass{ *this
			, name
			, runnableCreator } };

		if ( m_passes.end() != std::find_if( m_passes.begin()
			, m_passes.end()
			, [&pass]( FramePassPtr const & lookup )
			{
				return lookup->name == pass->name;
			} ) )
		{
			CRG_Exception( "Duplicate FramePass name detected." );
		}

		auto result = pass.get();
		m_passes.emplace_back( std::move( pass ) );
		return *result;
	}

	RunnableGraphPtr FrameGraph::compile( GraphContext context )
	{
		if ( m_passes.empty() )
		{
			CRG_Exception( "No FramePass registered." );
		}

		// Transitions for which the pass is the destination.
		FramePassDependenciesMap inputTransitions;
		// Transitions for which the pass is the source.
		FramePassDependenciesMap outputTransitions;
		// All transitions.
		AttachmentTransitionArray transitions;
		builder::buildPassAttachDependencies( m_passes
			, inputTransitions
			, outputTransitions
			, transitions );
		RootNode root{ m_name };
		auto nodes = builder::buildGraph( root
			, transitions );
		ImageMemoryMap images;
		ImageViewMap imageViews;
		return std::make_unique< RunnableGraph >( *this
			, std::move( inputTransitions )
			, std::move( outputTransitions )
			, std::move( transitions )
			, std::move( nodes )
			, std::move( root )
			, std::move( context ) );
	}

	ImageId FrameGraph::createImage( ImageData const & img )
	{
		auto data = std::make_unique< ImageData >( img );
		ImageId result{ uint32_t( m_images.size() + 1u ), data.get() };
		m_images.insert( { result, std::move( data ) } );
		return result;
	}

	ImageViewId FrameGraph::createView( ImageViewData const & view )
	{
		auto it = std::find_if( m_imageViews.begin()
			, m_imageViews.end()
			, [&view]( ImageViewIdDataOwnerCont::value_type const & lookup )
			{
				return *lookup.second == view;
			} );
		ImageViewId result{};

		if ( it == m_imageViews.end() )
		{
			auto data = std::make_unique< ImageViewData >( view );
			result = { uint32_t( m_imageViews.size() + 1u ), data.get() };
			m_imageViews.insert( { result, std::move( data ) } );
		}
		else
		{
			result = it->first;
		}

		return result;
	}

	void FrameGraph::setFinalLayout( ImageViewId view
		, VkImageLayout layout )
	{
		m_finalLayouts[view] = layout;
	}

	VkImageLayout FrameGraph::getFinalLayout( ImageViewId view )const
	{
		auto it = m_finalLayouts.find( view );

		if ( it == m_finalLayouts.end() )
		{
			return VK_IMAGE_LAYOUT_UNDEFINED;
		}

		return it->second;
	}

	VkExtent3D getExtent( ImageId const & image )
	{
		return image.data->info.extent;
	}

	VkExtent3D getExtent( ImageViewId const & image )
	{
		return getExtent( image.data->image );
	}

	VkFormat getFormat( ImageId const & image )
	{
		return image.data->info.format;
	}

	VkFormat getFormat( ImageViewId const & image )
	{
		return image.data->info.format;
	}

	VkImageType getImageType( ImageId const & image )
	{
		return image.data->info.imageType;
	}

	VkImageType getImageType( ImageViewId const & image )
	{
		return getImageType( image.data->image );
	}

	uint32_t getMipLevels( ImageId const & image )
	{
		return image.data->info.mipLevels;
	}

	uint32_t getMipLevels( ImageViewId const & image )
	{
		return image.data->info.subresourceRange.levelCount;
	}

	uint32_t getArrayLayers( ImageId const & image )
	{
		return image.data->info.arrayLayers;
	}

	uint32_t getArrayLayers( ImageViewId const & image )
	{
		return image.data->info.subresourceRange.layerCount;
	}
}
