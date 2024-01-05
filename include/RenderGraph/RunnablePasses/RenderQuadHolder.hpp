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

		CRG_API void initialise( VkExtent2D const & renderSize
			, VkRenderPass renderPass
			, VkPipelineColorBlendStateCreateInfo blendState
			, uint32_t index );
		CRG_API void resetRenderPass( VkExtent2D const & renderSize
			, VkRenderPass renderPass
			, VkPipelineColorBlendStateCreateInfo blendState
			, uint32_t index );
		CRG_API void resetPipeline( VkPipelineShaderStageCreateInfoArray config
			, uint32_t index );
		CRG_API void record( RecordContext & context
			, VkCommandBuffer commandBuffer
			, uint32_t index );
		CRG_API void end( RecordContext & context
			, VkCommandBuffer commandBuffer
			, uint32_t index )const;
		CRG_API uint32_t getPassIndex()const;
		CRG_API bool isEnabled()const;

		VkPipelineVertexInputStateCreateInfo const & getInputState()const
		{
			return m_vertexBuffer->inputState;
		}

		VkPipelineLayout getPipelineLayout()const
		{
			return m_pipeline.getPipelineLayout();
		}

		bool isInitialised()const
		{
			return m_vertexBuffer != nullptr;
		}

	private:
		void doPreparePipelineStates( VkExtent2D const & renderSize
			, VkRenderPass renderPass
			, VkPipelineColorBlendStateCreateInfo blendState );
		void doCreatePipeline( uint32_t passIndex );
		VkPipelineViewportStateCreateInfo doCreateViewportState( VkExtent2D const & renderSize
			, VkViewport & viewport
			, VkRect2D & scissor )const;

	private:
		rq::ConfigData m_config;
		RunnableGraph & m_graph;
		PipelineHolder m_pipeline;
		bool m_useTexCoord{ true };
		VertexBuffer const * m_vertexBuffer{};
		VkRenderPass m_renderPass{};
		VkExtent2D m_renderSize{};
		VkViewport m_viewport{};
		VkRect2D m_scissor{};
		VkPipelineViewportStateCreateInfo m_vpState{};
		VkPipelineInputAssemblyStateCreateInfo m_iaState{};
		VkPipelineMultisampleStateCreateInfo m_msState{};
		VkPipelineRasterizationStateCreateInfo m_rsState{};
		VkPipelineColorBlendStateCreateInfo m_blendState{};
		std::vector< VkPipelineColorBlendAttachmentState > m_blendAttachs{};
	};
}
