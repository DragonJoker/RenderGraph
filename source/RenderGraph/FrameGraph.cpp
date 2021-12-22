/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#include "RenderGraph/FrameGraph.hpp"

#include "RenderGraph/Exception.hpp"
#include "RenderGraph/FramePass.hpp"
#include "RenderGraph/ResourceHandler.hpp"
#include "RenderGraph/RunnableGraph.hpp"
#include "FramePassDependenciesBuilder.hpp"
#include "GraphBuilder.hpp"
#include "ResourceOptimiser.hpp"

#include <algorithm>

namespace crg
{
	namespace
	{
		bool dependsOn( FramePass const & pass
			, FramePass const * lookup )
		{
			return pass.passDepends.end() != std::find_if( pass.passDepends.begin()
				, pass.passDepends.end()
				, [lookup]( FramePass const * depLookup )
				{
					return depLookup == lookup;
				} );
		}

		std::vector< FramePass const * > sortPasses( FramePassPtrArray const & passes )
		{
			std::vector< FramePass const * > sortedPasses;
			std::vector< FramePass const * > unsortedPasses;

			for ( auto & pass : passes )
			{
				if ( pass->passDepends.empty() )
				{
					sortedPasses.push_back( pass.get() );
				}
				else
				{
					unsortedPasses.push_back( pass.get() );
				}
			}

			if ( sortedPasses.empty() )
			{
				sortedPasses.push_back( unsortedPasses.front() );
				unsortedPasses.erase( unsortedPasses.begin() );
			}

			while ( !unsortedPasses.empty() )
			{
				std::vector< FramePass const * > currentPasses;
				std::swap( currentPasses, unsortedPasses );
				bool added = false;

				for ( auto & pass : currentPasses )
				{
					auto it = std::find_if( sortedPasses.begin()
						, sortedPasses.end()
						, [&pass]( FramePass const * lookup )
						{
							return dependsOn( *lookup, pass );
						} );

					if ( it != sortedPasses.end() )
					{
						sortedPasses.insert( it, pass );
						added = true;
					}
					else
					{
						auto rit = std::find_if( sortedPasses.rbegin()
							, sortedPasses.rend()
							, [&pass]( FramePass const * lookup )
							{
								return dependsOn( *pass, lookup );
							} );

						if ( rit != sortedPasses.rend() )
						{
							sortedPasses.insert( rit.base(), pass );
							added = true;
						}
						else
						{
							unsortedPasses.push_back( pass );
						}
					}
				}

				if ( !added )
				{
					throw std::runtime_error{ "Couldn't sort passes:" };
				}
			}

			return sortedPasses;
		}
	}

	FrameGraph::FrameGraph( ResourceHandler & handler
		, std::string name )
		: m_handler{ handler }
		, m_name{ std::move( name ) }
	{
	}

	FramePass & FrameGraph::createPass( std::string const & name
		, RunnablePassCreator runnableCreator )
	{
		FramePassPtr pass{ new FramePass{ *this
			, uint32_t( m_passes.size() + 1u )
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
		if ( m_passes.empty() )
		{
			CRG_Exception( "No FramePass registered." );
		}

		auto sortedPasses = sortPasses( m_passes );
		GraphNodePtrArray nodes;

		for ( auto & pass : sortedPasses )
		{
			auto node = std::make_unique< FramePassNode >( *pass );
			nodes.emplace_back( std::move( node ) );
		}

		FramePassDependencies inputTransitions;
		FramePassDependencies outputTransitions;
		AttachmentTransitions transitions;
		builder::buildPassAttachDependencies( nodes
			, inputTransitions
			, outputTransitions
			, transitions );
		RootNode root{ *this };
		builder::buildGraph( root
			, nodes
			, transitions );
		ImageMemoryMap images;
		ImageViewMap imageViews;
		return std::make_unique< RunnableGraph >( *this
			, std::move( inputTransitions )
			, std::move( outputTransitions )
			, std::move( transitions )
			, std::move( nodes )
			, std::move( root )
			, context );
	}

	LayoutState FrameGraph::getFinalLayoutState( ImageViewId view )const
	{
		return m_finalState.getLayoutState( view );
	}

	AccessState FrameGraph::getFinalAccessState( Buffer const & buffer )const
	{
		return m_finalState.getAccessState( buffer.buffer, { 0u, VK_WHOLE_SIZE } );
	}

	void FrameGraph::registerFinalState( RecordContext const & context )
	{
		m_finalState = context.getData();
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
