/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "RenderGraph/RunnablePasses/RenderMeshConfig.hpp"
#include "RenderGraph/RunnablePasses/PipelineHolder.hpp"

namespace crg
{
	class RenderMeshHolder
	{
	public:
		CRG_API RenderMeshHolder( FramePass const & pass
			, GraphContext & context
			, RunnableGraph & graph
			, rm::Config config
			, uint32_t maxPassCount );
		CRG_API ~RenderMeshHolder();

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
		CRG_API uint32_t getPassIndex()const;
		CRG_API bool isEnabled()const;
		CRG_API VkExtent2D getRenderSize()const;

	private:
		void doCreatePipeline( VkExtent2D const & renderSize
			, VkRenderPass renderPass
			, VkPipelineColorBlendStateCreateInfo blendState );
		VkPipelineViewportStateCreateInfo doCreateViewportState( VkExtent2D const & renderSize
			, VkViewportArray & viewports
			, VkScissorArray & scissors );

	private:
		rm::ConfigData m_config;
		FramePass const & m_pass;
		GraphContext & m_context;
		PipelineHolder m_pipeline;
		uint32_t m_maxPassCount;
		VkRenderPass m_renderPass{};
		VkExtent2D m_renderSize{};
		VkPipelineColorBlendStateCreateInfo m_blendState{};
		std::vector< VkPipelineColorBlendAttachmentState > m_blendAttachs{};
	};
}
