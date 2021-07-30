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
		: m_config{ std::move( config.m_texcoordConfig ? *config.m_texcoordConfig : defaultV< Texcoord > )
			, std::move( config.m_renderPosition ? *config.m_renderPosition : defaultV< VkOffset2D > )
			, std::move( config.m_depthStencilState ? *config.m_depthStencilState : defaultV< VkPipelineDepthStencilStateCreateInfo > )
			, std::move( config.m_passIndex ? *config.m_passIndex : defaultV< uint32_t const * > )
			, std::move( config.m_enabled ? *config.m_enabled : defaultV< bool const * > )
			, std::move( config.m_recordInto ? *config.m_recordInto : getDefaultV< RunnablePass::RecordCallback >() )
			, std::move( config.m_recordDisabledInto ? *config.m_recordDisabledInto : getDefaultV< rq::RecordDisabledIntoFunc >() ) }
		, m_pass{ pass }
		, m_context{ context }
		, m_graph{ graph }
		, m_pipeline{ pass
			, context
			, graph
			, config.m_baseConfig
			, VK_PIPELINE_BIND_POINT_GRAPHICS
			, maxPassCount }
		, m_maxPassCount{ maxPassCount }
		, m_useTexCoord{ config.m_texcoordConfig }
	{
	}

	RenderQuadHolder::~RenderQuadHolder()
	{
	}

	void RenderQuadHolder::initialise( RunnablePass const & runnable
		, VkExtent2D const & renderSize
		, VkRenderPass renderPass
		, VkPipelineColorBlendStateCreateInfo blendState )
	{
		m_pipeline.initialise();
		m_vertexBuffer = &m_graph.createQuadTriVertexBuffer( m_useTexCoord
			, m_config.texcoordConfig );
		doCreatePipeline( renderSize, renderPass, std::move( blendState ) );
	}

	void RenderQuadHolder::resetPipeline( VkPipelineShaderStageCreateInfoArray config )
	{
		m_pipeline.resetPipeline( std::move( config ) );
	}

	void RenderQuadHolder::record( VkCommandBuffer commandBuffer
		, uint32_t index )
	{
		m_pipeline.recordInto( commandBuffer, index );
		VkDeviceSize offset{};
		m_context.vkCmdBindVertexBuffers( commandBuffer, 0u, 1u, &m_vertexBuffer->buffer.buffer, &offset );
		m_context.vkCmdDraw( commandBuffer, 3u, 1u, 0u, 0u );
	}

	void RenderQuadHolder::end( VkCommandBuffer commandBuffer
		, uint32_t index )
	{
		m_config.recordInto( commandBuffer, index );
	}

	void RenderQuadHolder::recordDisabled( RunnablePass const & runnable
		, VkCommandBuffer commandBuffer
		, uint32_t index )
	{
		if ( m_config.recordDisabledInto )
		{
			m_config.recordDisabledInto( runnable, commandBuffer, index );
		}
	}

	uint32_t RenderQuadHolder::getPassIndex()const
	{
		return ( m_config.passIndex
			? *m_config.passIndex
			: 0u );
	}

	bool RenderQuadHolder::isEnabled()const
	{
		return ( m_config.enabled
			? *m_config.enabled
			: true );
	}

	void RenderQuadHolder::doCreatePipeline( VkExtent2D const & renderSize
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

		for ( auto index = 0u; index < m_maxPassCount; ++index )
		{
			auto & program = m_pipeline.getProgram( index );
			auto & pipeline = m_pipeline.getPipeline( index );
			VkGraphicsPipelineCreateInfo createInfo{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO
				, nullptr
				, 0u
				, uint32_t( program.size() )
				, program.data()
				, &getInputState()
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
				, VK_NULL_HANDLE
				, 0u };
			auto res = m_context.vkCreateGraphicsPipelines( m_context.device
				, m_context.cache
				, 1u
				, &createInfo
				, m_context.allocator
				, &pipeline );
			checkVkResult( res, m_pass.name + " - Pipeline creation" );
			crgRegisterObject( m_context, m_pass.name, pipeline );
		}
	}

	VkPipelineViewportStateCreateInfo RenderQuadHolder::doCreateViewportState( VkExtent2D const & renderSize
		, VkViewportArray & viewports
		, VkScissorArray & scissors )
	{
		viewports.push_back( { float( m_config.renderPosition.x )
			, float( m_config.renderPosition.y )
			, float( renderSize.width )
			, float( renderSize.height ) } );
		scissors.push_back( { m_config.renderPosition.x
			, m_config.renderPosition.y
			, renderSize.width
			, renderSize.height } );
		return { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO
			, nullptr
			, 0u
			, uint32_t( viewports.size() )
			, viewports.data()
			, uint32_t( scissors.size() )
			, scissors.data() };
	}
}