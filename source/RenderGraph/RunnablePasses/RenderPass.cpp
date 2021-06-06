/*
See LICENSE file in root folder.
*/
#include "RenderGraph/RunnablePasses/RenderPass.hpp"

#include "RenderGraph/Attachment.hpp"
#include "RenderGraph/GraphContext.hpp"
#include "RenderGraph/ImageData.hpp"
#include "RenderGraph/RunnableGraph.hpp"

#include <array>

namespace crg
{
	//*********************************************************************************************

	namespace
	{
		VkAttachmentReference addAttach( Attachment const & attach
			, VkAttachmentDescriptionArray & attaches
			, std::vector< VkClearValue > & clearValues
			, VkImageLayout initialLayout
			, VkImageLayout finalLayout
			, bool separateDepthStencilLayouts )
		{
			auto view = attach.view();
			VkAttachmentReference result{ uint32_t( attaches.size() )
				, attach.getImageLayout( separateDepthStencilLayouts ) };
			attaches.push_back( { 0u
				, view.data->info.format
				, view.data->image.data->info.samples
				, attach.image.loadOp
				, attach.image.storeOp
				, attach.image.stencilLoadOp
				, attach.image.stencilStoreOp
				, initialLayout
				, finalLayout } );
			clearValues.push_back( attach.image.clearValue );
			return result;
		}

		VkAttachmentReference addAttach( Attachment const & attach
			, VkAttachmentDescriptionArray & attaches
			, std::vector< VkClearValue > & clearValues
			, VkPipelineColorBlendAttachmentStateArray & blendAttachs
			, VkImageLayout initialLayout
			, VkImageLayout finalLayout
			, bool separateDepthStencilLayouts )
		{
			blendAttachs.push_back( attach.image.blendState );
			return addAttach( attach
				, attaches
				, clearValues
				, initialLayout
				, finalLayout
				, separateDepthStencilLayouts );
		}
	}

	//*********************************************************************************************

	VkDescriptorPoolSizeArray getBindingsSizes( VkDescriptorSetLayoutBindingArray const & bindings
		, uint32_t maxSets )
	{
		VkDescriptorPoolSizeArray result;

		for ( auto & binding : bindings )
		{
			auto it = std::find_if( result.begin()
				, result.end()
				, [&binding]( VkDescriptorPoolSize const & lookup )
				{
					return binding.descriptorType == lookup.type;
				} );

			if ( it == result.end() )
			{
				result.push_back( { binding.descriptorType, binding.descriptorCount * maxSets } );
			}
			else
			{
				it->descriptorCount += binding.descriptorCount * maxSets;
			}
		}

		return result;
	}

	//*********************************************************************************************

	RenderPass::RenderPass( FramePass const & pass
		, GraphContext const & context
		, RunnableGraph & graph
		, uint32_t maxPassCount )
		: RunnablePass{ pass
			, context
			, graph
			, maxPassCount }
	{
	}

	RenderPass::~RenderPass()
	{
		for ( auto frameBuffer : m_frameBuffers )
		{
			crgUnregisterObject( m_context, frameBuffer );
			m_context.vkDestroyFramebuffer( m_context.device
				, frameBuffer
				, m_context.allocator );
		}

		if ( m_renderPass )
		{
			crgUnregisterObject( m_context, m_renderPass );
			m_context.vkDestroyRenderPass( m_context.device
				, m_renderPass
				, m_context.allocator );
		}
	}

	void RenderPass::doInitialise()
	{
		doCreateRenderPass();
		doCreateFramebuffer();
		doSubInitialise();
	}

	void RenderPass::doRecordInto( VkCommandBuffer commandBuffer
		, uint32_t index )
	{
		VkRenderPassBeginInfo beginInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO
			, nullptr
			, m_renderPass
			, m_frameBuffers[index]
			, m_renderArea
			, uint32_t( m_clearValues.size() )
			, m_clearValues.data() };
		m_context.vkCmdBeginRenderPass( commandBuffer
			, &beginInfo
			, doGetSubpassContents( 0u ) );
		doSubRecordInto( commandBuffer, index );
		m_context.vkCmdEndRenderPass( commandBuffer );
	}

	VkPipelineStageFlags RenderPass::doGetSemaphoreWaitFlags()const
	{
		return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	}

	void RenderPass::doCreateRenderPass()
	{
		VkAttachmentDescriptionArray attaches;
		VkAttachmentReferenceArray colorReferences;
		VkAttachmentReference depthReference{};

		for ( auto & attach : m_pass.images )
		{
			auto view = attach.view();
			auto transition = doGetTransition( view );

			if ( attach.isDepth() || attach.isStencil() )
			{
				depthReference = addAttach( attach
					, attaches
					, m_clearValues
					, transition.fromLayout
					, transition.toLayout
					, m_context.separateDepthStencilLayouts );
			}
			else if ( attach.isColour() )
			{
				colorReferences.push_back( addAttach( attach
					, attaches
					, m_clearValues
					, m_blendAttachs
					, transition.fromLayout
					, transition.toLayout
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
				, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT
				, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
				, VK_ACCESS_MEMORY_READ_BIT
				, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT
				, VK_DEPENDENCY_BY_REGION_BIT }
			, { 0u
				, VK_SUBPASS_EXTERNAL
				, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
				, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT
				, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT
				, VK_ACCESS_MEMORY_READ_BIT
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
		checkVkResult( res, m_pass.name + " - RenderPass creation" );
		crgRegisterObject( m_context, m_pass.name, m_renderPass );
	}

	VkPipelineColorBlendStateCreateInfo RenderPass::doCreateBlendState()
	{
		return { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO
			, nullptr
			, 0u
			, VK_FALSE
			, VK_LOGIC_OP_COPY
			, uint32_t( m_blendAttachs.size() )
			, m_blendAttachs.data() };
	}

	void RenderPass::doCreateFramebuffer()
	{
		m_frameBuffers.resize( m_commandBuffers.size() );

		for ( uint32_t index = 0; index < m_commandBuffers.size(); ++index )
		{
			VkImageViewArray attachments;
			uint32_t width{};
			uint32_t height{};
			uint32_t layers{ 1u };

			for ( auto & attach : m_pass.images )
			{
				if ( attach.isColour()
					|| attach.isDepth()
					|| attach.isStencil() )
				{
					auto view = attach.view( index );
					attachments.push_back( m_graph.getImageView( view ) );
					width = view.data->image.data->info.extent.width >> view.data->info.subresourceRange.baseMipLevel;
					height = view.data->image.data->info.extent.height >> view.data->info.subresourceRange.baseMipLevel;
				}
			}

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
			checkVkResult( res, m_pass.name + " - Framebuffer creation" );
			crgRegisterObject( m_context, m_pass.name, *frameBuffer );
		}
	}

	//*********************************************************************************************
}
