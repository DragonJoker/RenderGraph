/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#include "RenderGraph/RunnablePasses/RenderQuad.hpp"

#include "RenderGraph/Attachment.hpp"
#include "RenderGraph/GraphContext.hpp"
#include "RenderGraph/ImageData.hpp"
#include "RenderGraph/RunnableGraph.hpp"

#include <array>

namespace crg
{
	RenderQuad::RenderQuad( FramePass const & pass
		, GraphContext const & context
		, RunnableGraph & graph
		, uint32_t maxPassCount
		, rq::Config config )
		: RenderPass{ pass
			, context
			, graph
			, config.renderSize ? *config.renderSize : defaultV< VkExtent2D >
			, maxPassCount
			, config.enabled ? true : false }
		, PipelineHolder{ pass
			, context
			, graph
			, std::move( config.baseConfig )
			, VK_PIPELINE_BIND_POINT_GRAPHICS }
		, m_config{ std::move( config.texcoordConfig ? *config.texcoordConfig : defaultV< rq::Texcoord > )
			, std::move( config.renderSize ? *config.renderSize : defaultV< VkExtent2D > )
			, std::move( config.renderPosition ? *config.renderPosition : defaultV< VkOffset2D > )
			, std::move( config.depthStencilState ? *config.depthStencilState : defaultV< VkPipelineDepthStencilStateCreateInfo > )
			, std::move( config.passIndex ? *config.passIndex : defaultV< uint32_t const * > )
			, std::move( config.enabled ? *config.enabled : defaultV< bool const * > )
			, std::move( config.recordDisabledInto ? *config.recordDisabledInto : defaultV< rq::RecordDisabledIntoFunc > ) }
		, m_useTexCoord{ config.texcoordConfig }
	{
		m_descriptorSets.resize( m_commandBuffers.size() );
	}

	RenderQuad::~RenderQuad()
	{
	}

	void RenderQuad::resetPipeline( VkPipelineShaderStageCreateInfoArray config
		, uint32_t index )
	{
		resetCommandBuffer( index );
		doResetPipeline( std::move( config ), index );
		record( index );
	}

	void RenderQuad::doSubInitialise( uint32_t index )
	{
		if ( index == 0u )
		{
			m_vertexBuffer = &m_graph.createQuadVertexBuffer( m_useTexCoord
				, m_config.texcoordConfig.invertU
				, m_config.texcoordConfig.invertV );
		}

		doPreInitialise( index );
	}

	void RenderQuad::doSubRecordInto( VkCommandBuffer commandBuffer
		, uint32_t index )
	{
		doPreRecordInto( commandBuffer, index );
		VkDeviceSize offset{};
		m_context.vkCmdBindVertexBuffers( commandBuffer, 0u, 1u, &m_vertexBuffer->buffer.buffer, &offset );
		m_context.vkCmdDraw( commandBuffer, 4u, 1u, 0u, 0u );
	}

	void RenderQuad::doSubRecordDisabledInto( VkCommandBuffer commandBuffer
		, uint32_t index )
	{
		if ( m_config.recordDisabledInto )
		{
			m_config.recordDisabledInto( *this, commandBuffer, index );
		}
	}

	void RenderQuad::doCreatePipeline( uint32_t index )
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
		auto & program = doGetProgram( index );
		auto & pipeline = doGetPipeline( index );
		VkGraphicsPipelineCreateInfo createInfo{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO
			, nullptr
			, 0u
			, uint32_t( program.size() )
			, program.data()
			, &m_vertexBuffer->inputState
			, &iaState
			, nullptr
			, &vpState
			, &rsState
			, &msState
			, &m_config.depthStencilState
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
			, &pipeline );
		checkVkResult( res, m_pass.name + " - Pipeline creation" );
		crgRegisterObject( m_context, m_pass.name, pipeline );
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

	uint32_t RenderQuad::doGetPassIndex()const
	{
		return m_config.passIndex
			? *m_config.passIndex
			: 0u;
	}

	bool RenderQuad::doIsEnabled()const
	{
		return m_config.enabled
			? *m_config.enabled
			: true;
	}
}
