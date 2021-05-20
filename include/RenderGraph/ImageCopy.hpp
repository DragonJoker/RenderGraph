/*
See LICENSE file in root folder.
*/
#pragma once

#include "RunnablePass.hpp"

namespace crg
{
	class ImageCopy
		: public RunnablePass
	{
	public:
		ImageCopy( FramePass const & pass
			, GraphContext const & context
			, RunnableGraph & graph
			, VkExtent3D copySize );
		~ImageCopy();

	protected:
		void doInitialise()override;
		void doRecordInto( VkCommandBuffer commandBuffer )const override;
		VkPipelineStageFlags doGetSemaphoreWaitFlags()const override;

	private:
		Attachment m_srcAttach;
		Attachment m_dstAttach;
		VkExtent3D m_copySize;
		VkImage m_srcImage{};
		VkImage m_dstImage{};
	};
}
