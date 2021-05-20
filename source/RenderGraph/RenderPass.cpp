/*
See LICENSE file in root folder.
*/
#include "RenderGraph/RenderPass.hpp"

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
			, std::vector< VkClearValue > & clearValues )
		{
			VkImageLayout attachLayout;

			if ( attach.hasFlag( Attachment::Flag::Depth ) )
			{
				if ( attach.isDepthOutput() && attach.isStencilOutput() )
				{
					attachLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
				}
				else if ( attach.isDepthOutput() && attach.isStencilInput() )
				{
					attachLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
				}
				else if ( attach.isDepthInput() && attach.isStencilOutput() )
				{
					attachLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
				}
				else if ( attach.isDepthInput() && attach.isStencilInput() )
				{
					attachLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
				}
				else if ( attach.isDepthOutput() )
				{
					attachLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
				}
				else if ( attach.isStencilOutput() )
				{
					attachLayout = VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL;
				}
				else if ( attach.isDepthInput() )
				{
					attachLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
				}
				else if ( attach.isStencilInput() )
				{
					attachLayout = VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL;
				}
			}
			else
			{
				attachLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			}

			VkAttachmentReference result{ uint32_t( attaches.size() )
				, attachLayout };
			attaches.push_back( { 0u
				, attach.view.data->info.format
				, attach.view.data->image.data->info.samples
				, attach.loadOp
				, attach.storeOp
				, attach.stencilLoadOp
				, attach.stencilStoreOp
				, attach.initialLayout
				, attach.finalLayout } );
			clearValues.push_back( attach.clearValue );
			return result;
		}

		VkAttachmentReference addAttach( Attachment const & attach
			, VkAttachmentDescriptionArray & attaches
			, std::vector< VkClearValue > & clearValues
			, VkPipelineColorBlendAttachmentStateArray & blendAttachs )
		{
			blendAttachs.push_back( attach.blendState );
			return addAttach( attach, attaches, clearValues );
		}
	}

	//*********************************************************************************************

	VkDescriptorPoolSizeArray getBindingsSizes( VkDescriptorSetLayoutBindingArray const & bindings
		, uint32_t maxSets )
	{
		VkDescriptorPoolSizeArray result;

		for ( auto & binding : bindings )
		{
			result.push_back( { binding.descriptorType, binding.descriptorCount * maxSets } );
		}

		return result;
	}

	//*********************************************************************************************

	RenderPass::RenderPass( FramePass const & pass
		, GraphContext const & context
		, RunnableGraph & graph )
		: RunnablePass{ pass
			, context
			, graph }
	{
	}

	RenderPass::~RenderPass()
	{
		if ( m_frameBuffer )
		{
			crgUnregisterObject( m_context, m_frameBuffer );
			m_context.vkDestroyFramebuffer( m_context.device
				, m_frameBuffer
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

	void RenderPass::doRecordInto( VkCommandBuffer commandBuffer )const
	{
		VkDeviceSize offset{};
		VkRenderPassBeginInfo beginInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO
			, nullptr
			, m_renderPass
			, m_frameBuffer
			, m_renderArea
			, uint32_t( m_clearValues.size() )
			, m_clearValues.data() };
		m_context.vkCmdBeginRenderPass( commandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE );
		doSubRecordInto( commandBuffer );
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

		if ( m_pass.depthStencilInOut )
		{
			depthReference = addAttach( *m_pass.depthStencilInOut
				, attaches
				, m_clearValues );
		}

		for ( auto & attach : m_pass.colourInOuts )
		{
			colorReferences.push_back( addAttach( attach
				, attaches
				, m_clearValues
				, m_blendAttachs ) );
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
		checkVkResult( res, "FramePass creation" );
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
		VkImageViewArray attachments;
		uint32_t width{};
		uint32_t height{};
		uint32_t layers{ 1u };

		if ( m_pass.depthStencilInOut )
		{
			attachments.push_back( m_graph.getImageView( *m_pass.depthStencilInOut ) );
			width = m_pass.depthStencilInOut->view.data->image.data->info.extent.width;
			height = m_pass.depthStencilInOut->view.data->image.data->info.extent.height;
		}

		for ( auto & attach : m_pass.colourInOuts )
		{
			attachments.push_back( m_graph.getImageView( attach ) );
			width = attach.view.data->image.data->info.extent.width;
			height = attach.view.data->image.data->info.extent.height;
		}

		m_renderArea.extent.width = width;
		m_renderArea.extent.height = height;
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
			, &m_frameBuffer );
		checkVkResult( res, "Framebuffer creation" );
		crgRegisterObject( m_context, m_pass.name, m_frameBuffer );
	}

	//*********************************************************************************************
}
