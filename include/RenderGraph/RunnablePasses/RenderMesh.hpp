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
			, ru::Config ruConfig
			, rm::Config rmConfig );
		CRG_API ~RenderMesh()override = default;

		CRG_API void resetPipeline( VkPipelineShaderStageCreateInfoArray config
			, uint32_t index );

	private:
		void doInitialise();
		void doRecordInto( RecordContext & context
			, VkCommandBuffer commandBuffer
			, uint32_t index );

	private:
		RenderMeshHolder m_renderMesh;
		RenderPassHolder m_renderPass;
	};
}
