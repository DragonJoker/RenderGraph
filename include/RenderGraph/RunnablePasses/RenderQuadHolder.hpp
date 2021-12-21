/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "RenderGraph/RunnablePasses/RenderQuadConfig.hpp"
#include "RenderGraph/RunnablePasses/PipelineHolder.hpp"

namespace crg
{
	class RenderQuadHolder
	{
	public:
		CRG_API RenderQuadHolder( FramePass const & pass
			, GraphContext & context
			, RunnableGraph & graph
			, rq::Config config
			, uint32_t maxPassCount );
		CRG_API ~RenderQuadHolder();

		CRG_API void initialise( RunnablePass const & runnable
			, VkExtent2D const & renderSize
			, VkRenderPass renderPass
			, VkPipelineColorBlendStateCreateInfo blendState );
		CRG_API void resetRenderPass( VkExtent2D const & renderSize
			, VkRenderPass renderPass
			, VkPipelineColorBlendStateCreateInfo blendState );
		CRG_API void resetPipeline( VkPipelineShaderStageCreateInfoArray config );
		CRG_API void record( RecordContext & context
			, VkCommandBuffer commandBuffer
			, uint32_t index );
		CRG_API void end( RecordContext & context
			, VkCommandBuffer commandBuffer
			, uint32_t index );
		CRG_API void recordDisabled( RunnablePass const & runnable
			, RecordContext & context
			, VkCommandBuffer commandBuffer
			, uint32_t index );
		CRG_API uint32_t getPassIndex()const;
		CRG_API bool isEnabled()const;

		VkPipelineVertexInputStateCreateInfo const & getInputState()const
		{
			return m_vertexBuffer->inputState;
		}

		bool isInitialised()const
		{
			return m_vertexBuffer != nullptr;
		}

	private:
		void doCreatePipeline( VkExtent2D const & renderSize
			, VkRenderPass renderPass
			, VkPipelineColorBlendStateCreateInfo blendState );
		VkPipelineViewportStateCreateInfo doCreateViewportState( VkExtent2D const & renderSize
			, VkViewportArray & viewports
			, VkScissorArray & scissors );

	private:
		rq::ConfigData m_config;
		FramePass const & m_pass;
		GraphContext & m_context;
		RunnableGraph & m_graph;
		PipelineHolder m_pipeline;
		uint32_t m_maxPassCount;
		bool m_useTexCoord{ true };
		VertexBuffer const * m_vertexBuffer{};
		VkRenderPass m_renderPass{};
		VkExtent2D m_renderSize{};
		VkPipelineColorBlendStateCreateInfo m_blendState{};
		std::vector< VkPipelineColorBlendAttachmentState > m_blendAttachs{};
	};
}
