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
			, GraphContext const & context
			, RunnableGraph & graph
			, VkExtent3D copySize );
		CRG_API ~ImageCopy();

	protected:
		CRG_API void doInitialise()override;
		CRG_API void doRecordInto( VkCommandBuffer commandBuffer )const override;
		CRG_API VkPipelineStageFlags doGetSemaphoreWaitFlags()const override;

	private:
		Attachment m_srcAttach;
		Attachment m_dstAttach;
		VkExtent3D m_copySize;
		VkImage m_srcImage{};
		VkImage m_dstImage{};
	};
}
