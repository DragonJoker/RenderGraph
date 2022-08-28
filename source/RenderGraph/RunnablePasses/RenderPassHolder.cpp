/*
See LICENSE file in root folder.
*/
#include "RenderGraph/RunnablePasses/RenderPassHolder.hpp"

#include "RenderGraph/Attachment.hpp"
#include "RenderGraph/GraphContext.hpp"
#include "RenderGraph/ImageData.hpp"
#include "RenderGraph/RunnableGraph.hpp"

#include <array>

namespace crg
{
	//*********************************************************************************************

	namespace rpHolder
	{
		static VkAttachmentReference addAttach( RecordContext & context
			, Attachment const & attach
			, VkAttachmentDescriptionArray & attaches
			, std::vector< RenderPassHolder::Entry > & viewAttaches
			, std::vector< VkClearValue > & clearValues
			, LayoutState initialLayout
			, LayoutState finalLayout
			, bool separateDepthStencilLayouts )
		{
			auto view = attach.view();
			VkAttachmentReference result{ uint32_t( attaches.size() )
				, attach.getImageLayout( separateDepthStencilLayouts ) };
			auto from = context.getLayoutState( view );

			if ( from.layout == VK_IMAGE_LAYOUT_UNDEFINED )
			{
				from = initialLayout;
			}

			attaches.push_back( { 0u
				, view.data->info.format
				, view.data->image.data->info.samples
				, attach.image.loadOp
				, attach.image.storeOp
				, attach.image.stencilLoadOp
				, attach.image.stencilStoreOp
				, from.layout
				, finalLayout.layout } );
			viewAttaches.push_back( { view, from, finalLayout } );
			clearValues.push_back( attach.image.clearValue );
			return result;
		}

		static VkAttachmentReference addAttach( RecordContext & context
			, Attachment const & attach
			, VkAttachmentDescriptionArray & attaches
			, std::vector< RenderPassHolder::Entry > & viewAttaches
			, std::vector< VkClearValue > & clearValues
			, VkPipelineColorBlendAttachmentStateArray & blendAttachs
			, LayoutState initialLayout
			, LayoutState finalLayout
			, bool separateDepthStencilLayouts )
		{
			blendAttachs.push_back( attach.image.blendState );
			return addAttach( context
				, attach
				, attaches
				, viewAttaches
				, clearValues
				, initialLayout
				, finalLayout
				, separateDepthStencilLayouts );
		}

		static bool checkAttaches( RecordContext const & context
			, std::vector< RenderPassHolder::Entry > const & attaches )
		{
			auto it = std::find_if( attaches.begin()
				, attaches.end()
				, [&context]( RenderPassHolder::Entry const & lookup )
				{
					return context.getLayoutState( lookup.view ).layout != lookup.input.layout;
				} );
			return it == attaches.end();
		}
	}

	//*********************************************************************************************

	RenderPassHolder::RenderPassHolder( FramePass const & pass
		, GraphContext & context
		, RunnableGraph & graph
		, uint32_t maxPassCount
		, VkExtent2D const & size )
		: m_pass{ pass }
		, m_context{ context }
		, m_graph{ graph }
		, m_size{ size }
	{
		m_frameBuffers.resize( maxPassCount );
	}

	RenderPassHolder::~RenderPassHolder()
	{
		doCleanup();
	}

	bool RenderPassHolder::initialise( RecordContext & context
			, crg::RunnablePass const & runnable )
	{
		if ( m_renderPass
			&& rpHolder::checkAttaches( context, m_attaches ) )
		{
			return false;
		}

		doCleanup();
		doCreateRenderPass( context
			, runnable
			, context.getPrevPipelineState()
			, context.getNextPipelineState() );
		doCreateFramebuffer();
		return true;
	}

	VkRenderPassBeginInfo RenderPassHolder::getBeginInfo( uint32_t index )
	{
		return VkRenderPassBeginInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO
			, nullptr
			, getRenderPass()
			, getFramebuffer( index )
			, getRenderArea()
			, uint32_t( getClearValues().size() )
			, getClearValues().data() };
	}

	void RenderPassHolder::begin( RecordContext & context
		, VkCommandBuffer commandBuffer
		, VkSubpassContents subpassContents
		, uint32_t index )
	{
		for ( auto & attach : m_attaches )
		{
			context.setLayoutState( attach.view
				, attach.input );
		}

		auto beginInfo = getBeginInfo( index );
		m_context.vkCmdBeginRenderPass( commandBuffer
			, &beginInfo
			, subpassContents );
	}

	void RenderPassHolder::end( RecordContext & context
			, VkCommandBuffer commandBuffer )
	{
		m_context.vkCmdEndRenderPass( commandBuffer );

		for ( auto & attach : m_attaches )
		{
			context.setLayoutState( attach.view
				, attach.output );
		}
	}

	void RenderPassHolder::doCreateRenderPass( RecordContext & context
			, crg::RunnablePass const & runnable
			, PipelineState const & previousState
			, PipelineState const & nextState )
	{
		VkAttachmentDescriptionArray attaches;
		VkAttachmentReferenceArray colorReferences;
		VkAttachmentReference depthReference{};

		for ( auto & attach : m_pass.images )
		{
			auto view = attach.view();
			auto transition = runnable.getTransition( 0u, view );

			if ( attach.isDepthAttach() || attach.isStencilAttach() )
			{
				depthReference = rpHolder::addAttach( context
					, attach
					, attaches
					, m_attaches
					, m_clearValues
					, transition.from
					, transition.to
					, m_context.separateDepthStencilLayouts );
			}
			else if ( attach.isColourAttach() )
			{
				colorReferences.push_back( rpHolder::addAttach( context
					, attach
					, attaches
					, m_attaches
					, m_clearValues
					, m_blendAttachs
					, transition.from
					, transition.to
					, m_context.separateDepthStencilLayouts ) );
			}
		}

		VkSubpassDescription subpassDesc{ 0u
			, VK_PIPELINE_BIND_POINT_GRAPHICS
			, 0u
			, nullptr
			, uint32_t( colorReferences.size() )
			, colorReferences.data()
			, nullptr
			, depthReference.layout ? &depthReference : nullptr
			, 0u
			, nullptr };
		VkSubpassDependencyArray dependencies{
			{ VK_SUBPASS_EXTERNAL
				, 0u
				, previousState.pipelineStage
				, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
				, previousState.access
				, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT
				, VK_DEPENDENCY_BY_REGION_BIT }
			, { 0u
				, VK_SUBPASS_EXTERNAL
				, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
				, nextState.pipelineStage
				, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT
				, nextState.access
				, VK_DEPENDENCY_BY_REGION_BIT } };
		VkRenderPassCreateInfo createInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO
			, nullptr
			, 0u
			, uint32_t( attaches.size() )
			, attaches.data()
			, 1u
			, &subpassDesc
			, uint32_t( dependencies.size() )
			, dependencies.data() };
		auto res = m_context.vkCreateRenderPass( m_context.device
			, &createInfo
			, m_context.allocator
			, &m_renderPass );
		checkVkResult( res, m_pass.getGroupName() + " - RenderPass creation" );
		crgRegisterObject( m_context, m_pass.getGroupName(), m_renderPass );
	}

	VkPipelineColorBlendStateCreateInfo RenderPassHolder::createBlendState()
	{
		return { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO
			, nullptr
			, 0u
			, VK_FALSE
			, VK_LOGIC_OP_COPY
			, uint32_t( m_blendAttachs.size() )
			, m_blendAttachs.data()
			, {} };
	}

	void RenderPassHolder::doCreateFramebuffer()
	{
		for ( uint32_t index = 0; index < m_frameBuffers.size(); ++index )
		{
			VkImageViewArray attachments;
			uint32_t width{};
			uint32_t height{};
			uint32_t layers{ 1u };

			for ( auto & attach : m_pass.images )
			{
				if ( attach.isColourAttach()
					|| attach.isDepthAttach()
					|| attach.isStencilAttach() )
				{
					auto view = attach.view( index );
					attachments.push_back( m_graph.createImageView( view ) );
					width = view.data->image.data->info.extent.width >> view.data->info.subresourceRange.baseMipLevel;
					height = view.data->image.data->info.extent.height >> view.data->info.subresourceRange.baseMipLevel;
					layers = std::max( layers, view.data->info.subresourceRange.layerCount );
				}
			}

			width = std::max( width, m_size.width );
			height = std::max( height, m_size.height );
			m_renderArea.extent.width = width;
			m_renderArea.extent.height = height;
			auto frameBuffer = &m_frameBuffers[index];
			VkFramebufferCreateInfo createInfo{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO
				, nullptr
				, 0u
				, m_renderPass
				, uint32_t( attachments.size() )
				, attachments.data()
				, width
				, height
				, layers };
			auto res = m_context.vkCreateFramebuffer( m_context.device
				, &createInfo
				, m_context.allocator
				, frameBuffer );
			checkVkResult( res, m_pass.getGroupName() + " - Framebuffer creation" );
			crgRegisterObject( m_context, m_pass.getGroupName(), *frameBuffer );
		}
	}

	void RenderPassHolder::doCleanup()
	{
		m_attaches.clear();
		m_clearValues.clear();
		m_blendAttachs.clear();

		for ( auto & frameBuffer : m_frameBuffers )
		{
			// Don't clear vector, only delete its values (it is sized at creation).
			if ( frameBuffer )
			{
				crgUnregisterObject( m_context, frameBuffer );
				m_context.vkDestroyFramebuffer( m_context.device
					, frameBuffer
					, m_context.allocator );
				frameBuffer = {};
			}
		}

		if ( m_renderPass )
		{
			crgUnregisterObject( m_context, m_renderPass );
			m_context.vkDestroyRenderPass( m_context.device
				, m_renderPass
				, m_context.allocator );
			m_renderPass = {};
		}
	}
}
