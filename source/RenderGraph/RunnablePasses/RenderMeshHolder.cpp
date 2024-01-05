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
		: m_config{ config.m_renderPosition ? std::move( *config.m_renderPosition ) : getDefaultV< VkOffset2D >()
			, config.m_depthStencilState ? std::move( *config.m_depthStencilState ) : getDefaultV< VkPipelineDepthStencilStateCreateInfo >()
			, config.m_getPassIndex ? std::move( *config.m_getPassIndex ) : getDefaultV< RunnablePass::GetPassIndexCallback >()
			, config.m_isEnabled ? std::move( *config.m_isEnabled ) : getDefaultV< RunnablePass::IsEnabledCallback >()
			, config.m_recordInto ? std::move( *config.m_recordInto ) : getDefaultV< RunnablePass::RecordCallback >()
			, config.m_end ? std::move( *config.m_end ) : getDefaultV< RunnablePass::RecordCallback >()
			, config.m_getPrimitiveCount ? std::move( *config.m_getPrimitiveCount ) : getDefaultV< GetPrimitiveCountCallback >()
			, config.m_getVertexCount ? std::move( *config.m_getVertexCount ) : getDefaultV< GetVertexCountCallback >()
			, config.m_getIndexType ? std::move( *config.m_getIndexType ) : getDefaultV< GetIndexTypeCallback >()
			, config.m_getCullMode ? std::move( *config.m_getCullMode ) : getDefaultV< GetCullModeCallback >()
			, config.m_vertexBuffer ? std::move( *config.m_vertexBuffer ) : getDefaultV< VertexBuffer >()
			, config.m_indexBuffer ? std::move( *config.m_indexBuffer ) : getDefaultV< IndexBuffer >()
			, config.m_indirectBuffer ? *config.m_indirectBuffer : getDefaultV< IndirectBuffer >() }
		, m_pipeline{ pass
			, context
			, graph
			, config.m_baseConfig
			, VK_PIPELINE_BIND_POINT_GRAPHICS
			, maxPassCount }
		, m_renderSize{ config.m_renderSize ? *config.m_renderSize : getDefaultV< VkExtent2D >() }
	{
		m_iaState = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO
			, nullptr
			, 0u
			, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
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
			, m_config.getCullMode()
			, VK_FRONT_FACE_COUNTER_CLOCKWISE
			, VK_FALSE
			, 0.0f
			, 0.0f
			, 0.0f
			, 0.0f };
	}

	void RenderMeshHolder::initialise( VkExtent2D const & renderSize
		, VkRenderPass renderPass
		, VkPipelineColorBlendStateCreateInfo blendState
		, uint32_t index )
	{
		m_pipeline.initialise();
		doPreparePipelineStates( renderSize, renderPass, std::move( blendState ) );
		doCreatePipeline( index );
	}

	void RenderMeshHolder::cleanup()
	{
		m_pipeline.cleanup();
	}

	void RenderMeshHolder::resetRenderPass( VkExtent2D const & renderSize
		, VkRenderPass renderPass
		, VkPipelineColorBlendStateCreateInfo blendState
		, uint32_t index )
	{
		m_pipeline.resetPipeline( {}, index );
		doPreparePipelineStates( renderSize, renderPass, std::move( blendState ) );
		doCreatePipeline( index );
	}

	void RenderMeshHolder::resetPipeline( VkPipelineShaderStageCreateInfoArray config
		, uint32_t index )
	{
		m_pipeline.resetPipeline( std::move( config ), index );

		if ( m_renderPass )
		{
			doCreatePipeline( index );
		}
	}

	void RenderMeshHolder::record( RecordContext & context
		, VkCommandBuffer commandBuffer
		, uint32_t index )
	{
		doCreatePipeline( index );
		m_pipeline.recordInto( context, commandBuffer, index );
		m_config.recordInto( context, commandBuffer, index );
		VkDeviceSize offset{};

		if ( m_config.vertexBuffer.buffer.buffer( index ) )
		{
			context->vkCmdBindVertexBuffers( commandBuffer, 0u, 1u, &m_config.vertexBuffer.buffer.buffer( index ), &offset );
		}

		if ( auto indirectBuffer = m_config.indirectBuffer.buffer.buffer( index ) )
		{
			if ( auto indexBuffer = m_config.indexBuffer.buffer.buffer( index ) )
			{
				context->vkCmdBindIndexBuffer( commandBuffer, indexBuffer, offset, m_config.getIndexType() );
				context->vkCmdDrawIndexedIndirect( commandBuffer, indirectBuffer, m_config.indirectBuffer.offset, 1u, m_config.indirectBuffer.stride );
			}
			else
			{
				context->vkCmdDrawIndirect( commandBuffer, indirectBuffer, m_config.indirectBuffer.offset, 1u, m_config.indirectBuffer.stride );
			}
		}
		else if ( auto indexBuffer = m_config.indexBuffer.buffer.buffer( index ) )
		{
			context->vkCmdBindIndexBuffer( commandBuffer, indexBuffer, offset, m_config.getIndexType() );
			context->vkCmdDrawIndexed( commandBuffer, m_config.getPrimitiveCount(), 1u, 0u, 0u, 0u );
		}
		else
		{
			context->vkCmdDraw( commandBuffer, m_config.getVertexCount(), 1u, 0u, 0u );
		}
	}

	void RenderMeshHolder::end( RecordContext & context
		, VkCommandBuffer commandBuffer
		, uint32_t index )const
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

	void RenderMeshHolder::doPreparePipelineStates( VkExtent2D const & renderSize
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

	void RenderMeshHolder::doCreatePipeline( uint32_t index )
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
			, &m_config.vertexBuffer.inputState
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

	VkPipelineViewportStateCreateInfo RenderMeshHolder::doCreateViewportState( VkExtent2D const & renderSize
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
		return { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO
			, nullptr
			, 0u
			, 1u
			, & viewport
			, 1u
			, & scissor };
	}
}
