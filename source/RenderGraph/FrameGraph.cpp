/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#include "RenderGraph/FrameGraph.hpp"

#include "RenderGraph/Exception.hpp"
#include "RenderGraph/FramePass.hpp"
#include "RenderGraph/FramePassGroup.hpp"
#include "RenderGraph/ResourceHandler.hpp"
#include "RenderGraph/RunnableGraph.hpp"
#include "FramePassDependenciesBuilder.hpp"
#include "GraphBuilder.hpp"
#include "ResourceOptimiser.hpp"

#include <algorithm>

namespace crg
{
	namespace fgph
	{
		static bool dependsOn( FramePass const & pass
			, FramePass const * lookup )
		{
			return pass.passDepends.end() != std::find_if( pass.passDepends.begin()
				, pass.passDepends.end()
				, [lookup]( FramePass const * depLookup )
				{
					return depLookup == lookup;
				} );
		}

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
						continue;
					}

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
		, m_defaultGroup{ new FramePassGroup{ *this, 0u, m_name } }
		, m_finalState{ handler }
	{
	}

	FramePass & FrameGraph::createPass( std::string const & name
		, RunnablePassCreator runnableCreator )
	{
		return m_defaultGroup->createPass( name, runnableCreator );
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
			, std::move( passes )
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
		m_finalState = context;
	}

	VkExtent3D getExtent( ImageId const & image )
	{
		return image.data->info.extent;
	}

	VkExtent3D getExtent( ImageViewId const & image )
	{
		return getExtent( image.data->image );
	}

	VkExtent3D getMipExtent( ImageViewId const & image )
	{
		auto result = getExtent( image.data->image );
		result.width >>= image.data->info.subresourceRange.baseMipLevel;
		result.height >>= image.data->info.subresourceRange.baseMipLevel;
		result.depth >>= image.data->info.subresourceRange.baseMipLevel;
		return result;
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
