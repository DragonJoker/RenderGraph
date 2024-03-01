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
						processed = true;
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

		static bool match( VkImageSubresourceRange const & lhsRange
			, VkImageSubresourceRange const & rhsRange )noexcept
		{
			return ( ( lhsRange.aspectMask & rhsRange.aspectMask ) != 0 )
				&& lhsRange.baseArrayLayer == rhsRange.baseArrayLayer
				&& lhsRange.layerCount == rhsRange.layerCount
				&& lhsRange.baseMipLevel == rhsRange.baseMipLevel
				&& lhsRange.levelCount == rhsRange.levelCount;
		}

		static bool match( ImageId const & image
			, VkImageViewType lhsType
			, VkImageViewType rhsType
			, VkImageSubresourceRange const & lhsRange
			, VkImageSubresourceRange const & rhsRange )noexcept
		{
			auto result = lhsType == rhsType;

			if ( !result )
			{
				result = match( getVirtualRange( image, lhsType, lhsRange )
					, getVirtualRange( image, rhsType, rhsRange ) );
			}
			else
			{
				result = match( lhsRange, rhsRange );
			}

			return result;
		}

		static bool match( ImageId const & image
			, VkImageViewCreateInfo const & lhsInfo
			, VkImageViewCreateInfo const & rhsInfo )noexcept
		{
			return lhsInfo.flags == rhsInfo.flags
				&& lhsInfo.format == rhsInfo.format
				&& match( image
					, lhsInfo.viewType, rhsInfo.viewType
					, lhsInfo.subresourceRange, rhsInfo.subresourceRange );
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
		, VkImageViewType viewType
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
				, view.data->info.viewType
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
		, VkImageViewType viewType
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
			, view.data->info.viewType
			, view.data->info.subresourceRange
			, outputLayout );
	}

	LayoutState FrameGraph::getInputLayoutState( ImageId image
		, VkImageViewType viewType
		, VkImageSubresourceRange const & range )const
	{
		return m_inputs.getLayoutState( image
			, viewType
			, range );
	}

	LayoutState FrameGraph::getInputLayoutState( ImageViewId view )const
	{
		return getInputLayoutState( view.data->image
			, view.data->info.viewType
			, view.data->info.subresourceRange );
	}

	void FrameGraph::addOutput( ImageId image
		, VkImageViewType viewType
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
			, view.data->info.viewType
			, view.data->info.subresourceRange
			, outputLayout );
	}

	LayoutState FrameGraph::getOutputLayoutState( ImageId image
		, VkImageViewType viewType
		, VkImageSubresourceRange const & range )const
	{
		return m_outputs.getLayoutState( image
			, viewType
			, range );
	}

	LayoutState FrameGraph::getOutputLayoutState( ImageViewId view )const
	{
		return getOutputLayoutState( view.data->image
			, view.data->info.viewType
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

	LayoutState makeLayoutState( VkImageLayout layout )
	{
		return { layout
			, crg::getAccessMask( layout )
			, crg::getStageMask( layout ) };
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

	VkImageAspectFlags getAspectMask( VkFormat format )noexcept
	{
		return VkImageAspectFlags( isDepthStencilFormat( format )
			? VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT
			: ( isDepthFormat( format )
				? VK_IMAGE_ASPECT_DEPTH_BIT
				: ( isStencilFormat( format )
					? VK_IMAGE_ASPECT_STENCIL_BIT
					: VK_IMAGE_ASPECT_COLOR_BIT ) ) );
	}

	LayoutState const & addSubresourceRangeLayout( LayerLayoutStates & ranges
		, VkImageSubresourceRange const & range
		, LayoutState const & newLayout )
	{
		for ( uint32_t layerIdx = 0u; layerIdx < range.layerCount; ++layerIdx )
		{
			auto & layers = ranges.try_emplace( range.baseArrayLayer + layerIdx ).first->second;

			for ( uint32_t levelIdx = 0u; levelIdx < range.levelCount; ++levelIdx )
			{
				layers.insert_or_assign( range.baseMipLevel + levelIdx, newLayout );
			}
		}

		return newLayout;
	}

	LayoutState getSubresourceRangeLayout( LayerLayoutStates const & ranges
		, VkImageSubresourceRange const & range )
	{
		std::map< VkImageLayout, LayoutState > states;

		for ( uint32_t layerIdx = 0u; layerIdx < range.layerCount; ++layerIdx )
		{
			auto layerIt = ranges.find( range.baseArrayLayer + layerIdx );

			if ( layerIt != ranges.end() )
			{
				auto & layers = layerIt->second;

				for ( uint32_t levelIdx = 0u; levelIdx < range.levelCount; ++levelIdx )
				{
					auto it = layers.find( range.baseMipLevel + levelIdx );

					if ( it != layers.end() )
					{
						auto state = it->second;
						auto [rit, res] = states.emplace( state.layout, state );

						if ( !res )
						{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnull-dereference"
							rit->second.state.access |= state.state.access;
#pragma GCC diagnostic pop
						}
					}
				}
			}
		}

		if ( states.empty() )
		{
			return { VK_IMAGE_LAYOUT_UNDEFINED
				, getAccessMask( VK_IMAGE_LAYOUT_UNDEFINED )
				, getStageMask( VK_IMAGE_LAYOUT_UNDEFINED ) };
		}

		return states.begin()->second;
	}

	VkImageSubresourceRange getVirtualRange( ImageId const & image
		, VkImageViewType viewType
		, VkImageSubresourceRange const & range )noexcept
	{
		VkImageSubresourceRange result = range;

		if ( viewType == VK_IMAGE_VIEW_TYPE_3D
			&& ( range.levelCount == 1u
				|| range.levelCount == image.data->info.mipLevels ) )
		{
			result.baseArrayLayer = 0u;
			result.layerCount = getExtent( image ).depth >> range.baseMipLevel;
		}

		return result;
	}

	bool match( ImageViewData const & lhs, ImageViewData const & rhs )noexcept
	{
		return lhs.image.id == rhs.image.id
			&& fgph::match( lhs.image
				, lhs.info
				, rhs.info );
	}

	bool isDepthFormat( VkFormat fmt )noexcept
	{
		return fmt == VK_FORMAT_D16_UNORM
			|| fmt == VK_FORMAT_X8_D24_UNORM_PACK32
			|| fmt == VK_FORMAT_D32_SFLOAT
			|| fmt == VK_FORMAT_D16_UNORM_S8_UINT
			|| fmt == VK_FORMAT_D24_UNORM_S8_UINT
			|| fmt == VK_FORMAT_D32_SFLOAT_S8_UINT;
	}

	bool isStencilFormat( VkFormat fmt )noexcept
	{
		return fmt == VK_FORMAT_S8_UINT
			|| fmt == VK_FORMAT_D16_UNORM_S8_UINT
			|| fmt == VK_FORMAT_D24_UNORM_S8_UINT
			|| fmt == VK_FORMAT_D32_SFLOAT_S8_UINT;
	}

	bool isColourFormat( VkFormat fmt )noexcept
	{
		return !isDepthFormat( fmt ) && !isStencilFormat( fmt );
	}

	bool isDepthStencilFormat( VkFormat fmt )noexcept
	{
		return isDepthFormat( fmt ) && isStencilFormat( fmt );
	}

	ImageViewId const & resolveView( ImageViewId const & view
		, uint32_t passIndex )
	{
		return view.data->source.empty()
			? view
			: view.data->source[passIndex];
	}
}
