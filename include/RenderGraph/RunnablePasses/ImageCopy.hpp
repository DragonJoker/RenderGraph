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

	private:
		void doInitialise();
		void doRecordInto( VkCommandBuffer commandBuffer
			, uint32_t index );
		VkPipelineStageFlags doGetSemaphoreWaitFlags()const;
		uint32_t doGetPassIndex()const;
		bool doIsEnabled()const;

	private:
		VkExtent3D m_copySize;
		uint32_t const * m_passIndex;
		bool const * m_enabled;
	};
}
