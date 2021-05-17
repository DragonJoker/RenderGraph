/*
This file belongs to RenderGraph.
See LICENSE file in root folder.
*/
#include "RenderGraph/RenderGraph.hpp"

#include "GraphBuilder.hpp"
#include "RenderPassDependenciesBuilder.hpp"
#include "ResourceOptimiser.hpp"

#include "RenderGraph/Exception.hpp"
#include "RenderGraph/RenderPass.hpp"

#include <algorithm>

namespace crg
{
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

	void RenderGraph::compile()
	{
		if ( m_passes.empty() )
		{
			CRG_Exception( "No RenderPass registered." );
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
