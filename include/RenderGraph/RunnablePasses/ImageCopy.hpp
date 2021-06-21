/*
See LICENSE file in root folder.
*/
#pragma once

#include "RenderGraph/RunnablePass.hpp"

namespace crg
{
	class ImageCopy
		: public RunnablePass
	{
	public:
		CRG_API ImageCopy( FramePass const & pass
			, GraphContext & context
			, RunnableGraph & graph
			, VkExtent3D copySize
			, uint32_t maxPassCount = 1u
			, bool optional = false
			, uint32_t const * passIndex = nullptr
			, bool const * enabled = nullptr );
		CRG_API ~ImageCopy();

	protected:
		CRG_API void doInitialise()override;
		CRG_API void doRecordInto( VkCommandBuffer commandBuffer
			, uint32_t index )override;
		CRG_API VkPipelineStageFlags doGetSemaphoreWaitFlags()const override;
		CRG_API uint32_t doGetPassIndex()const override;
		CRG_API bool doIsEnabled()const override;

	private:
		VkExtent3D m_copySize;
		uint32_t const * m_passIndex;
		bool const * m_enabled;
	};
}
