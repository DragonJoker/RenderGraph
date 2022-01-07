/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#include "RenderGraph/RunnablePasses/RenderMeshHolder.hpp"

namespace crg
{
	//*********************************************************************************************

	RenderMeshHolder::RenderMeshHolder( FramePass const & pass
		, GraphContext & context
		, RunnableGraph & graph
		, rm::Config config
		, uint32_t maxPassCount )
		: m_config{ config.m_renderPosition ? std::move( *config.m_renderPosition ) : defaultV< VkOffset2D >
			, config.m_depthStencilState ? std::move( *config.m_depthStencilState ) : defaultV< VkPipelineDepthStencilStateCreateInfo >
			, config.m_getPassIndex ? std::move( *config.m_getPassIndex ) : defaultV< RunnablePass::GetPassIndexCallback >
			, config.m_isEnabled ? std::move( *config.m_isEnabled ) : getDefaultV< RunnablePass::IsEnabledCallback >()
			, config.m_recordInto ? std::move( *config.m_recordInto ) : getDefaultV< RunnablePass::RecordCallback >()
			, config.m_end ? std::move( *config.m_end ) : getDefaultV< RunnablePass::RecordCallback >()
			, config.m_getPrimitiveCount ? std::move( *config.m_getPrimitiveCount ) : getDefaultV< GetPrimitiveCountCallback >()
			, config.m_getVertexCount ? std::move( *config.m_getVertexCount ) : getDefaultV< GetVertexCountCallback >()
			, config.m_getIndexType ? std::move( *config.m_getIndexType ) : getDefaultV< GetIndexTypeCallback >()
			, config.m_vertexBuffer ? std::move( *config.m_vertexBuffer ) : getDefaultV< VertexBuffer >()
			, config.m_indexBuffer ? std::move( *config.m_indexBuffer ) : getDefaultV< IndexBuffer >() }
		, m_pass{ pass }
		, m_context{ context }
		, m_pipeline{ pass
			, context
			, graph
			, config.m_baseConfig
			, VK_PIPELINE_BIND_POINT_GRAPHICS
			, maxPassCount }
		, m_maxPassCount{ maxPassCount }
		, m_renderSize{ config.m_renderSize ? *config.m_renderSize : getDefaultV< VkExtent2D >() }
	{
	}

	RenderMeshHolder::~RenderMeshHolder()
	{
	}

	void RenderMeshHolder::initialise( RunnablePass const & runnable
		, VkExtent2D const & renderSize
		, VkRenderPass renderPass
		, VkPipelineColorBlendStateCreateInfo blendState )
	{
		m_pipeline.initialise();
		doCreatePipeline( renderSize, renderPass, std::move( blendState ) );
	}

	void RenderMeshHolder::resetRenderPass( VkExtent2D const & renderSize
		, VkRenderPass renderPass
		, VkPipelineColorBlendStateCreateInfo blendState )
	{
		m_pipeline.resetPipeline( {} );
		doCreatePipeline( renderSize, renderPass, std::move( blendState ) );
	}

	void RenderMeshHolder::resetPipeline( VkPipelineShaderStageCreateInfoArray config )
	{
		m_pipeline.resetPipeline( std::move( config ) );

		if ( m_renderPass )
		{
			doCreatePipeline( m_renderSize, m_renderPass, std::move( m_blendState ) );
		}
	}

	void RenderMeshHolder::record( RecordContext & context
		, VkCommandBuffer commandBuffer
		, uint32_t index )
	{
		m_pipeline.recordInto( context, commandBuffer, index );
		m_config.recordInto( context, commandBuffer, index );
		VkDeviceSize offset{};

		if ( m_config.vertexBuffer.buffer.buffer )
		{
			m_context.vkCmdBindVertexBuffers( commandBuffer, 0u, 1u, &m_config.vertexBuffer.buffer.buffer, &offset );
		}

		if ( m_config.indexBuffer.buffer.buffer )
		{
			m_context.vkCmdBindIndexBuffer( commandBuffer, m_config.indexBuffer.buffer.buffer, offset, m_config.getIndexType() );
			m_context.vkCmdDrawIndexed( commandBuffer, m_config.getPrimitiveCount(), 1u, 0u, 0u, 0u );
		}
		else
		{
			m_context.vkCmdDraw( commandBuffer, m_config.getVertexCount(), 1u, 0u, 0u );
		}
	}

	void RenderMeshHolder::end( RecordContext & context
		, VkCommandBuffer commandBuffer
		, uint32_t index )
	{
		m_config.end( context, commandBuffer, index );
	}

	uint32_t RenderMeshHolder::getPassIndex()const
	{
		return m_config.getPassIndex();
	}

	bool RenderMeshHolder::isEnabled()const
	{
		return m_config.isEnabled();
	}

	VkExtent2D RenderMeshHolder::getRenderSize()const
	{
		return m_renderSize;
	}

	void RenderMeshHolder::doCreatePipeline( VkExtent2D const & renderSize
		, VkRenderPass renderPass
		, VkPipelineColorBlendStateCreateInfo blendState )
	{
		VkVertexInputAttributeDescriptionArray vertexAttribs;
		VkVertexInputBindingDescriptionArray vertexBindings;
		VkViewportArray viewports;
		VkScissorArray scissors;
		auto vpState = doCreateViewportState( renderSize, viewports, scissors );
		VkPipelineInputAssemblyStateCreateInfo iaState{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO
			, nullptr
			, 0u
			, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
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

		for ( auto index = 0u; index < m_maxPassCount; ++index )
		{
			auto & program = m_pipeline.getProgram( index );
			auto & pipeline = m_pipeline.getPipeline( index );
			VkGraphicsPipelineCreateInfo createInfo{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO
				, nullptr
				, 0u
				, uint32_t( program.size() )
				, program.data()
				, &m_config.vertexBuffer.inputState
				, &iaState
				, nullptr
				, &vpState
				, &rsState
				, &msState
				, &m_config.depthStencilState
				, &blendState
				, nullptr
				, m_pipeline.getPipelineLayout()
				, renderPass
				, 0u
				, nullptr
				, 0u };
			auto res = m_context.vkCreateGraphicsPipelines( m_context.device
				, m_context.cache
				, 1u
				, &createInfo
				, m_context.allocator
				, &pipeline );
			checkVkResult( res, m_pass.getGroupName() + " - Pipeline creation" );
			crgRegisterObject( m_context, m_pass.getGroupName(), pipeline );
		}

		m_renderSize = renderSize;
		m_renderPass = renderPass;
		m_blendAttachs = { blendState.pAttachments, blendState.pAttachments + blendState.attachmentCount };
		m_blendState = blendState;
		m_blendState.pAttachments = m_blendAttachs.data();
	}

	VkPipelineViewportStateCreateInfo RenderMeshHolder::doCreateViewportState( VkExtent2D const & renderSize
		, VkViewportArray & viewports
		, VkScissorArray & scissors )
	{
		viewports.push_back( { float( m_config.renderPosition.x )
			, float( m_config.renderPosition.y )
			, float( renderSize.width )
			, float( renderSize.height )
			, 0.0f
			, 1.0f } );
		scissors.push_back( { { m_config.renderPosition.x, m_config.renderPosition.y }
		, { renderSize.width, renderSize.height } } );
		return { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO
			, nullptr
			, 0u
			, uint32_t( viewports.size() )
			, viewports.data()
			, uint32_t( scissors.size() )
			, scissors.data() };
	}
}
