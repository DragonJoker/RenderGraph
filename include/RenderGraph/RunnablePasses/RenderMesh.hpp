/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "RenderGraph/RunnablePasses/RenderMeshHolder.hpp"
#include "RenderGraph/RunnablePasses/RenderPass.hpp"

namespace crg
{
	class RenderMesh
		: public RunnablePass
	{
	public:
		CRG_API RenderMesh( FramePass const & pass
			, GraphContext & context
			, RunnableGraph & graph
			, ru::Config const & ruConfig
			, rm::Config rmConfig );
		CRG_API ~RenderMesh()noexcept override = default;

		CRG_API void resetPipelineLayout( std::vector< VkDescriptorSetLayout > const & layouts
			, std::vector< VkPushConstantRange > const & ranges
			, VkPipelineShaderStageCreateInfoArray const & config
			, uint32_t index );
		CRG_API void resetPipeline( VkPipelineShaderStageCreateInfoArray config
			, uint32_t index );

	private:
		void doRecordInto( RecordContext & context
			, VkCommandBuffer commandBuffer
			, uint32_t index );

	private:
		RenderMeshHolder m_renderMesh;
		RenderPassHolder m_renderPass;
	};
}
