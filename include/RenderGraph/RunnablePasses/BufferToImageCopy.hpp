/*
See LICENSE file in root folder.
*/
#pragma once

#include "RenderGraph/RunnablePass.hpp"

namespace crg
{
	class BufferToImageCopy
		: public RunnablePass
	{
	public:
		CRG_API BufferToImageCopy( FramePass const & pass
			, GraphContext & context
			, RunnableGraph & graph
			, VkOffset3D copyOffset
			, VkExtent3D copySize
			, ru::Config ruConfig = {}
			, uint32_t const * passIndex = nullptr
			, bool const * enabled = nullptr );

	private:
		void doInitialise();
		void doRecordInto( RecordContext & context
			, VkCommandBuffer commandBuffer
			, uint32_t index );
		VkPipelineStageFlags doGetSemaphoreWaitFlags()const;
		uint32_t doGetPassIndex()const;
		bool doIsEnabled()const;

	private:
		VkOffset3D m_copyOffset;
		VkExtent3D m_copySize;
		uint32_t const * m_passIndex;
		bool const * m_enabled;
	};
}
