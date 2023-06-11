/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#include "RenderGraph/RunnablePasses/RenderQuadHolder.hpp"

#include "RenderGraph/GraphContext.hpp"

namespace crg
{
	//*********************************************************************************************

	RenderQuadHolder::RenderQuadHolder( FramePass const & pass
		, GraphContext & context
		, RunnableGraph & graph
		, rq::Config config
		, uint32_t maxPassCount )
		: m_config{ config.m_texcoordConfig ? std::move( *config.m_texcoordConfig ) : defaultV< Texcoord >
			, config.m_renderPosition ? std::move( *config.m_renderPosition ) : defaultV< VkOffset2D >
			, config.m_depthStencilState ? std::move( *config.m_depthStencilState ) : defaultV< VkPipelineDepthStencilStateCreateInfo >
			, config.m_passIndex ? std::move( *config.m_passIndex ) : defaultV< uint32_t const * >
			, config.m_enabled ? std::move( *config.m_enabled ) : defaultV< bool const * >
			, config.m_isEnabled
			, config.m_recordInto ? std::move( *config.m_recordInto ) : getDefaultV< RunnablePass::RecordCallback >()
			, config.m_end ? std::move( *config.m_end ) : getDefaultV< RunnablePass::RecordCallback >()
			, config.m_instances ? std::move( *config.m_instances ) : 1u
			, config.m_indirectBuffer ? *config.m_indirectBuffer : getDefaultV < IndirectBuffer >() }
		, m_context{ context }
		, m_graph{ graph }
		, m_pipeline{ pass
			, context
			, graph
			, config.m_baseConfig
			, VK_PIPELINE_BIND_POINT_GRAPHICS
			, maxPassCount }
		, m_useTexCoord{ config.m_texcoordConfig }
	{
		m_iaState = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO
			, nullptr
			, 0u
			, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP
			, VK_FALSE };
		m_msState = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO
			, nullptr
			, 0u
			, VK_SAMPLE_COUNT_1_BIT
			, VK_FALSE
			, 0.0f
			, nullptr
			, VK_FALSE
			, VK_FALSE };
		m_rsState = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO
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
	}

	void RenderQuadHolder::initialise( RunnablePass const & runnable
		, VkExtent2D const & renderSize
		, VkRenderPass renderPass
		, VkPipelineColorBlendStateCreateInfo blendState
		, uint32_t index )
	{
		if ( !m_vertexBuffer )
		{
			m_pipeline.initialise();
			m_vertexBuffer = &m_graph.createQuadTriVertexBuffer( m_useTexCoord
				, m_config.texcoordConfig );
			doPreparePipelineStates( renderSize, renderPass, std::move( blendState ) );
		}
		else if ( m_renderPass != renderPass )
		{
			m_pipeline.resetPipeline( m_pipeline.getProgram( index ), index );
			doPreparePipelineStates( renderSize, renderPass, std::move( blendState ) );
		}

		if ( m_renderPass && m_renderPass != renderPass )
		{
			resetRenderPass( renderSize, renderPass, blendState, index );
		}
		else
		{
			doCreatePipeline( index );
		}
	}

	void RenderQuadHolder::resetRenderPass( VkExtent2D const & renderSize
		, VkRenderPass renderPass
		, VkPipelineColorBlendStateCreateInfo blendState
		, uint32_t index )
	{
		m_pipeline.resetPipeline( {}, index );
		doPreparePipelineStates( renderSize, renderPass, std::move( blendState ) );
		doCreatePipeline( index );
	}

	void RenderQuadHolder::resetPipeline( VkPipelineShaderStageCreateInfoArray config
		, uint32_t index )
	{
		m_pipeline.resetPipeline( std::move( config ), index );

		if ( m_renderPass )
		{
			doCreatePipeline( index );
		}
	}

	void RenderQuadHolder::record( RecordContext & context
		, VkCommandBuffer commandBuffer
		, uint32_t index )
	{
		doCreatePipeline( index );
		m_pipeline.recordInto( context, commandBuffer, index );
		m_config.recordInto( context, commandBuffer, index );
		VkDeviceSize offset{};
		m_context.vkCmdBindVertexBuffers( commandBuffer, 0u, 1u, &m_vertexBuffer->buffer.buffer( index ), &offset );

		if ( auto indirectBuffer = m_config.indirectBuffer.buffer.buffer( index ) )
		{
			m_context.vkCmdDrawIndirect( commandBuffer, indirectBuffer, m_config.indirectBuffer.offset, 1u, m_config.indirectBuffer.stride );
		}
		else
		{
			m_context.vkCmdDraw( commandBuffer, 3u, m_config.m_instances, 0u, 0u );
		}
	}

	void RenderQuadHolder::end( RecordContext & context
		, VkCommandBuffer commandBuffer
		, uint32_t index )
	{
		m_config.end( context, commandBuffer, index );
	}

	uint32_t RenderQuadHolder::getPassIndex()const
	{
		return ( m_config.passIndex
			? *m_config.passIndex
			: 0u );
	}

	bool RenderQuadHolder::isEnabled()const
	{
		return ( m_config.isEnabled
			? ( *m_config.isEnabled )()
			: ( m_config.enabled
				? *m_config.enabled
				: true ) );
	}

	void RenderQuadHolder::doPreparePipelineStates( VkExtent2D const & renderSize
		, VkRenderPass renderPass
		, VkPipelineColorBlendStateCreateInfo blendState )
	{
		m_vpState = doCreateViewportState( renderSize, m_viewport, m_scissor );
		m_renderSize = renderSize;
		m_renderPass = renderPass;
		m_blendAttachs = { blendState.pAttachments, blendState.pAttachments + blendState.attachmentCount };
		m_blendState = blendState;
		m_blendState.pAttachments = m_blendAttachs.data();
	}

	void RenderQuadHolder::doCreatePipeline( uint32_t index )
	{
		if ( m_pipeline.getPipeline( index ) )
		{
			return;
		}

		auto & program = m_pipeline.getProgram( index );
		VkGraphicsPipelineCreateInfo createInfo{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO
			, nullptr
			, 0u
			, uint32_t( program.size() )
			, program.data()
			, &getInputState()
			, &m_iaState
			, nullptr
			, &m_vpState
			, &m_rsState
			, &m_msState
			, &m_config.depthStencilState
			, &m_blendState
			, nullptr
			, m_pipeline.getPipelineLayout()
			, m_renderPass
			, 0u
			, VkPipeline{}
			, 0u };
		m_pipeline.createPipeline( index, createInfo );
	}

	VkPipelineViewportStateCreateInfo RenderQuadHolder::doCreateViewportState( VkExtent2D const & renderSize
		, VkViewport & viewport
		, VkRect2D & scissor )
	{
		viewport = { float( m_config.renderPosition.x )
			, float( m_config.renderPosition.y )
			, float( renderSize.width )
			, float( renderSize.height )
			, 0.0f
			, 1.0f };
		scissor = { { m_config.renderPosition.x, m_config.renderPosition.y }
			, { renderSize.width, renderSize.height } };
		return { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO
			, nullptr
			, 0u
			, 1u
			, &viewport
			, 1u
			, &scissor };
	}
}
