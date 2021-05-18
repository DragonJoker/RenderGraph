/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#include "RenderGraph/FrameGraph.hpp"

#include "RenderGraph/Exception.hpp"
#include "RenderGraph/FramePass.hpp"
#include "FramePassDependenciesBuilder.hpp"
#include "GraphBuilder.hpp"
#include "ResourceOptimiser.hpp"

#include <algorithm>

namespace crg
{
	FrameGraph::FrameGraph( std::string name )
		: m_root{ std::move( name ) }
	{
	}

	void FrameGraph::add( FramePass const & pass )
	{
		if ( m_passes.end() != std::find_if( m_passes.begin()
			, m_passes.end()
			, [&pass]( FramePassPtr const & lookup )
			{
				return lookup->name == pass.name;
			} ) )
		{
			CRG_Exception( "Duplicate FramePass name detected." );
		}

		for ( auto & attach : pass.sampled )
		{
			m_attachments.emplace_back( attach );
		}

		for ( auto & attach : pass.colourInOuts )
		{
			m_attachments.emplace_back( attach );
		}

		if ( pass.depthStencilInOut )
		{
			m_attachments.emplace_back( *pass.depthStencilInOut );
		}

		m_passes.push_back( std::make_unique< FramePass >( pass ) );
	}

	void FrameGraph::remove( FramePass const & pass )
	{
		auto it = std::find_if( m_passes.begin()
			, m_passes.end()
			, [&pass]( FramePassPtr const & lookup )
			{
				return lookup->name == pass.name;
			} );

		if ( m_passes.end() == it )
		{
			CRG_Exception( "FramePass was not found." );
		}

		m_passes.erase( it );
	}

	void FrameGraph::compile()
	{
		if ( m_passes.empty() )
		{
			CRG_Exception( "No FramePass registered." );
		}

		for ( auto & attach : m_attachments )
		{
			m_attachViews.emplace( attach.viewData.name, createView( attach.viewData ) );
		}

		auto dependencies = builder::buildPassDependencies( m_passes );
		m_nodes = builder::buildGraph( m_passes
			, m_root
			, m_transitions
			, dependencies );
		m_imageAliases = builder::optimiseImages( m_images
			, m_passes
			, dependencies
			, m_transitions
			, m_root );
		m_imageViewAliases = builder::optimiseImageViews( m_imageViews
			, m_passes
			, dependencies
			, m_transitions
			, m_root );
	}

	ImageId FrameGraph::createImage( ImageData const & img )
	{
		auto data = std::make_unique< ImageData >( img );
		ImageId result{ uint32_t( m_images.size() + 1u ), data.get() };
		m_images.insert( { result, std::move( data ) } );
		return result;
	}

	ImageViewId FrameGraph::createView( ImageViewData const & img )
	{
		auto data = std::make_unique< ImageViewData >( img );
		ImageViewId result{ uint32_t( m_imageViews.size() + 1u ), data.get() };
		m_imageViews.insert( { result, std::move( data ) } );
		return result;
	}
}
