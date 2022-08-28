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

	VkAccessFlags getAccessMask( VkImageLayout layout )noexcept
	{
		VkAccessFlags result{ 0u };

		switch ( layout )
		{
		case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
		case VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR:
			result |= VK_ACCESS_MEMORY_READ_BIT;
			break;
		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			result |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			break;
		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			result |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			break;
		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
			result |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
			break;
		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			result |= VK_ACCESS_SHADER_READ_BIT;
			break;
		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			result |= VK_ACCESS_TRANSFER_READ_BIT;
			break;
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			result |= VK_ACCESS_TRANSFER_WRITE_BIT;
			break;
		case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL:
		case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL:
			result |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
			result |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			break;
#ifdef VK_NV_shading_rate_image
		case VK_IMAGE_LAYOUT_SHADING_RATE_OPTIMAL_NV:
			result |= VK_ACCESS_SHADING_RATE_IMAGE_READ_BIT_NV;
			break;
#endif
#ifdef VK_EXT_fragment_density_map
		case VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT:
			result |= VK_ACCESS_FRAGMENT_DENSITY_MAP_READ_BIT_EXT;
			break;
#endif
		default:
			break;
		}

		return result;
	}

	PipelineState getPipelineState( VkPipelineStageFlags flags )noexcept
	{
		VkAccessFlags result{ 0u };

		if ( ( flags & VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT ) == VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT )
		{
			result |= VK_ACCESS_MEMORY_READ_BIT;
		}

		if ( ( flags & VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT ) == VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT )
		{
			result |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
		}

		if ( ( flags & VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT ) == VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT )
		{
			result |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			result |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
		}

		if ( ( flags & VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT ) == VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT )
		{
			result |= VK_ACCESS_SHADER_READ_BIT;
		}

		if ( ( flags & VK_PIPELINE_STAGE_TRANSFER_BIT ) == VK_PIPELINE_STAGE_TRANSFER_BIT )
		{
			result |= VK_ACCESS_TRANSFER_READ_BIT;
			result |= VK_ACCESS_TRANSFER_WRITE_BIT;
		}

		if ( ( flags & VK_PIPELINE_STAGE_SHADING_RATE_IMAGE_BIT_NV ) == VK_PIPELINE_STAGE_SHADING_RATE_IMAGE_BIT_NV )
		{
			result |= VK_ACCESS_SHADING_RATE_IMAGE_READ_BIT_NV;
		}

		if ( ( flags & VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT ) == VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT )
		{
			result |= VK_ACCESS_SHADER_READ_BIT;
		}

		return { result, flags };
	}

	VkPipelineStageFlags getStageMask( VkImageLayout layout )noexcept
	{
		VkPipelineStageFlags result{ 0u };

		switch ( layout )
		{
		case VK_IMAGE_LAYOUT_UNDEFINED:
			result |= VK_PIPELINE_STAGE_HOST_BIT;
			break;
		case VK_IMAGE_LAYOUT_GENERAL:
			result |= VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			break;
		case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
		case VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR:
			result |= VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			break;
		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
		case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL:
		case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL:
		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			result |= VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			break;
		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			result |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			break;
#ifdef VK_EXT_fragment_density_map
		case VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT:
#endif
		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			result |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			break;
		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			result |= VK_PIPELINE_STAGE_TRANSFER_BIT;
			break;
#ifdef VK_NV_shading_rate_image
		case VK_IMAGE_LAYOUT_SHADING_RATE_OPTIMAL_NV:
			result |= VK_PIPELINE_STAGE_SHADING_RATE_IMAGE_BIT_NV;
			break;
#endif
		default:
			break;
		}

		return result;
	}
}
