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
			, ru::Config ruConfig = {}
			, GetPassIndexCallback passIndex = GetPassIndexCallback( [](){ return 0u; } )
			, IsEnabledCallback isEnabled = IsEnabledCallback( [](){ return true; } ) );

	private:
		void doInitialise();
		void doRecordInto( RecordContext & context
			, VkCommandBuffer commandBuffer
			, uint32_t index );
		VkPipelineStageFlags doGetSemaphoreWaitFlags()const;

	private:
		VkExtent3D m_copySize;
	};
}
