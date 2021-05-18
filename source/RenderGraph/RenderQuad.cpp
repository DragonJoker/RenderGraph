/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#include "RenderGraph/RenderQuad.hpp"

#include "RenderGraph/Attachment.hpp"
#include "RenderGraph/GraphContext.hpp"
#include "RenderGraph/ImageData.hpp"
#include "RenderGraph/RunnableGraph.hpp"

#include <array>

namespace crg
{
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
				, attach.viewData.info.format
				, attach.viewData.image.data->info.samples
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

	RenderQuad::RenderQuad( FramePass const & pass
		, GraphContext const & context
		, RunnableGraph & graph
		, rq::Config config )
		: RunnablePass{ pass
			, context
			, graph
			, std::move( config.baseConfig )
			, VK_PIPELINE_BIND_POINT_GRAPHICS }
		, m_config{ std::move( config.texcoordConfig ? *config.texcoordConfig : defaultV< rq::Texcoord > )
			, std::move( config.renderSize ? *config.renderSize : defaultV< VkExtent2D > )
			, std::move( config.renderPosition ? *config.renderPosition : defaultV< VkOffset2D > ) }
		, m_useTexCoord{ config.texcoordConfig }
	{
	}

	RenderQuad::~RenderQuad()
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

	void RenderQuad::doInitialise()
	{
		m_vertexBuffer = &m_graph.createQuadVertexBuffer( m_useTexCoord
			, m_config.texcoordConfig.invertU
			, m_config.texcoordConfig.invertV );
		doCreateFramePass();
		doCreatePipeline();
		doCreateFramebuffer();
	}

	void RenderQuad::doRecordInto( VkCommandBuffer commandBuffer )const
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
		m_context.vkCmdBindVertexBuffers( commandBuffer, 0u, 1u, &m_vertexBuffer->buffer, &offset );
		m_context.vkCmdDraw( commandBuffer, 4u, 1u, 0u, 0u );
		m_context.vkCmdEndRenderPass( commandBuffer );
	}

	void RenderQuad::doCreateFramePass()
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
			, m_bindingPoint
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

	void RenderQuad::doCreatePipeline()
	{
		VkVertexInputAttributeDescriptionArray vertexAttribs;
		VkVertexInputBindingDescriptionArray vertexBindings;
		VkViewportArray viewports;
		VkScissorArray scissors;
		auto vpState = doCreateViewportState( viewports, scissors );
		auto cbState = doCreateBlendState();
		VkPipelineInputAssemblyStateCreateInfo iaState{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO
			, nullptr
			, 0u
			, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP
			, VK_FALSE };
		VkPipelineMultisampleStateCreateInfo msState{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO
			, nullptr
			, 0u
			, VK_SAMPLE_COUNT_1_BIT
			, VK_FALSE
			, 0.0f
			, nullptr
			, VK_FALSE
			, VK_FALSE };
		VkPipelineRasterizationStateCreateInfo rsState{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO
			, nullptr
			, 0u
			, VK_FALSE
			, VK_FALSE
			, VK_POLYGON_MODE_FILL
			, VK_CULL_MODE_NONE
			, VK_FRONT_FACE_COUNTER_CLOCKWISE
			, VK_FALSE
			, 0.0f
			, 0.0f
			, 0.0f
			, 0.0f };
		VkGraphicsPipelineCreateInfo createInfo{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO
			, nullptr
			, 0u
			, uint32_t( m_baseConfig.program.size() )
			, m_baseConfig.program.data()
			, &m_vertexBuffer->inputState
			, &iaState
			, nullptr
			, &vpState
			, &rsState
			, &msState
			, &m_baseConfig.dsState
			, &cbState
			, nullptr
			, m_pipelineLayout
			, m_renderPass
			, 0u
			, VK_NULL_HANDLE
			, 0u };
		auto res = m_context.vkCreateGraphicsPipelines( m_context.device
			, m_context.cache
			, 1u
			, &createInfo
			, m_context.allocator
			, &m_pipeline );
		checkVkResult( res, "Pipeline creation" );
		crgRegisterObject( m_context, m_pass.name, m_pipeline );
	}

	VkPipelineViewportStateCreateInfo RenderQuad::doCreateViewportState( VkViewportArray & viewports
		, VkScissorArray & scissors )
	{
		viewports.push_back( { float( m_config.renderPosition.x )
			, float( m_config.renderPosition.y )
			, float( m_config.renderSize.width )
			, float( m_config.renderSize.height ) } );
		scissors.push_back( { m_config.renderPosition.x
			, m_config.renderPosition.y
			, m_config.renderSize.width
			, m_config.renderSize.height } );
		return { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO
			, nullptr
			, 0u
			, uint32_t( viewports.size())
			, viewports.data()
			, uint32_t( scissors.size())
			, scissors.data() };
	}

	VkPipelineColorBlendStateCreateInfo RenderQuad::doCreateBlendState()
	{
		return { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO
			, nullptr
			, 0u
			, VK_FALSE
			, VK_LOGIC_OP_COPY
			, uint32_t( m_blendAttachs.size() )
			, m_blendAttachs.data() };
	}

	void RenderQuad::doCreateFramebuffer()
	{
		VkImageViewArray attachments;
		uint32_t width{};
		uint32_t height{};
		uint32_t layers{ 1u };

		if ( m_pass.depthStencilInOut )
		{
			attachments.push_back( m_graph.getImageView( *m_pass.depthStencilInOut ) );
			width = m_pass.depthStencilInOut->viewData.image.data->info.extent.width;
			height = m_pass.depthStencilInOut->viewData.image.data->info.extent.height;
		}

		for ( auto & attach : m_pass.colourInOuts )
		{
			attachments.push_back( m_graph.getImageView( attach ) );
			width = attach.viewData.image.data->info.extent.width;
			height = attach.viewData.image.data->info.extent.height;
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
}
