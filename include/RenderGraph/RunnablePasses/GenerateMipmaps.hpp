/*
See LICENSE file in root folder.
*/
#pragma once

#include "RenderGraph/RunnablePass.hpp"

namespace crg
{
	class GenerateMipmaps
		: public RunnablePass
	{
	public:
		CRG_API GenerateMipmaps( FramePass const & pass
			, GraphContext & context
			, RunnableGraph & graph
			, VkImageLayout outputLayout = VK_IMAGE_LAYOUT_UNDEFINED
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
		LayoutState m_outputLayout;
		uint32_t const * m_passIndex;
		bool const * m_enabled;
	};
}
