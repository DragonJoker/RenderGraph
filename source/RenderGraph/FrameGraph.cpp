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

	void FrameGraph::compile()
	{
		if ( m_passes.empty() )
		{
			CRG_Exception( "No FramePass registered." );
		}

		builder::buildPassAttachDependencies( m_passes
			, m_inputTransitions
			, m_outputTransitions
			, m_transitions );
		m_nodes = builder::buildGraph( m_root
			, m_transitions );
		m_imageAliases = builder::optimiseImages( m_images
			, m_inputTransitions
			, m_root );
		m_imageViewAliases = builder::optimiseImageViews( m_imageViews
			, m_inputTransitions
			, m_root );
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
}
