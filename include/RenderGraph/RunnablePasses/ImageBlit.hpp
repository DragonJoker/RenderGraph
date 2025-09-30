/*
See LICENSE file in root folder.
*/
#pragma once

#include "RenderGraph/RunnablePass.hpp"

namespace crg
{
	class ImageBlit
		: public RunnablePass
	{
	public:
		CRG_API ImageBlit( FramePass const & pass
			, GraphContext & context
			, RunnableGraph & graph
			, Rect3D const & blitSrc
			, Rect3D const & blitDst
			, FilterMode filter
			, ru::Config ruConfig = {}
			, GetPassIndexCallback passIndex = GetPassIndexCallback( [](){ return 0u; } )
			, IsEnabledCallback isEnabled = IsEnabledCallback( [](){ return true; } ) );

	private:
		void doRecordInto( RecordContext const & context
			, VkCommandBuffer commandBuffer
			, uint32_t index )const;

	private:
		VkOffset3D m_srcOffset;
		VkExtent3D m_srcSize;
		VkOffset3D m_dstOffset;
		VkExtent3D m_dstSize;
		FilterMode m_filter;
	};
}
