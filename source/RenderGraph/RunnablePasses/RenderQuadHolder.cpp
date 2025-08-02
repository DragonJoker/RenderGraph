/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#include "RenderGraph/RunnablePasses/RenderQuadHolder.hpp"

#include "RenderGraph/GraphContext.hpp"

namespace crg
{
	namespace rdqdhdr
	{
		static bool isPtrEnabled( bool const * v )
		{
			return v ? *v : true;
		}
	}

	RenderQuadHolder::RenderQuadHolder( FramePass const & pass
		, GraphContext & context
		, RunnableGraph & graph
		, rq::Config config
		, uint32_t maxPassCount )
		: m_config{ config.m_texcoordConfig ? std::move( *config.m_texcoordConfig ) : getDefaultV< Texcoord >()
			, config.m_renderPosition ? std::move( *config.m_renderPosition ) : getDefaultV< Offset2D >()
			, config.m_depthStencilState ? std::move( *config.m_depthStencilState ) : getDefaultV< VkPipelineDepthStencilStateCreateInfo >()
			, config.m_passIndex.has_value() ? *config.m_passIndex : getDefaultV< uint32_t const * >()
			, config.m_enabled.has_value() ? *config.m_enabled : getDefaultV< bool const * >()
			, config.m_isEnabled
			, config.m_recordInto ? std::move( *config.m_recordInto ) : getDefaultV< RunnablePass::RecordCallback >()
			, config.m_end ? std::move( *config.m_end ) : getDefaultV< RunnablePass::RecordCallback >()
			, config.m_instances.has_value() ? *config.m_instances : 1u
			, config.m_indirectBuffer ? *config.m_indirectBuffer : getDefaultV < IndirectBuffer >() }
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

	void RenderQuadHolder::initialise( Extent2D const & renderSize
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
			resetRenderPass( renderSize, renderPass, std::move( blendState ), index );
		}

		doCreatePipeline( index );
	}

	void RenderQuadHolder::resetRenderPass( Extent2D const & renderSize
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

		if ( m_vertexBuffer )
		{
			auto vkBuffer = m_graph.createBuffer( m_vertexBuffer->buffer.data->buffer );
			context->vkCmdBindVertexBuffers( commandBuffer, 0u, 1u
				, &vkBuffer, &getSubresourceRange( m_vertexBuffer->buffer ).offset );
		}

		if ( m_config.indirectBuffer != defaultV< IndirectBuffer > )
		{
			auto indirectBuffer = m_graph.createBuffer( m_config.indirectBuffer.buffer.data->buffer );
			context->vkCmdDrawIndirect( commandBuffer, indirectBuffer, getSubresourceRange( m_config.indirectBuffer.buffer ).offset, 1u, m_config.indirectBuffer.stride );
		}
		else
		{
			context->vkCmdDraw( commandBuffer, 3u, m_config.m_instances, 0u, 0u );
		}
	}

	void RenderQuadHolder::end( RecordContext & context
		, VkCommandBuffer commandBuffer
		, uint32_t index )const
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
			: rdqdhdr::isPtrEnabled( m_config.enabled ) );
	}

	void RenderQuadHolder::doPreparePipelineStates( Extent2D const & renderSize
		, VkRenderPass renderPass
		, VkPipelineColorBlendStateCreateInfo blendState )
	{
		m_vpState = doCreateViewportState( renderSize, m_viewport, m_scissor );
		m_renderSize = renderSize;
		m_renderPass = renderPass;
		m_blendAttachs = { blendState.pAttachments, blendState.pAttachments + blendState.attachmentCount };
		m_blendState = std::move( blendState );
		m_blendState.pAttachments = m_blendAttachs.data();
	}

	void RenderQuadHolder::doCreatePipeline( uint32_t index )
	{
		if ( m_pipeline.getPipeline( index ) )
		{
			return;
		}

		auto & program = m_pipeline.getProgram( index );
		VkGraphicsPipelineCreateInfo createInfo{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO, nullptr, 0u
			, uint32_t( program.size() ), program.data()
			, &getInputState(), &m_iaState, nullptr
			, &m_vpState, &m_rsState, &m_msState
			, &m_config.depthStencilState, &m_blendState, nullptr
			, m_pipeline.getPipelineLayout(), m_renderPass
			, 0u, VkPipeline{}, 0u };
		m_pipeline.createPipeline( index, createInfo );
	}

	VkPipelineViewportStateCreateInfo RenderQuadHolder::doCreateViewportState( Extent2D const & renderSize
		, VkViewport & viewport
		, VkRect2D & scissor )const
	{
		viewport = { float( m_config.renderPosition.x )
			, float( m_config.renderPosition.y )
			, float( renderSize.width )
			, float( renderSize.height )
			, 0.0f
			, 1.0f };
		scissor = { { m_config.renderPosition.x, m_config.renderPosition.y }
			, { renderSize.width, renderSize.height } };
		return { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO, nullptr, 0u
			, 1u, &viewport
			, 1u, &scissor };
	}
}
