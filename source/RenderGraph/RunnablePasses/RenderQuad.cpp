/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#include "RenderGraph/RunnablePasses/RenderQuad.hpp"

namespace crg
{
	RenderQuad::RenderQuad( FramePass const & pass
		, GraphContext & context
		, RunnableGraph & graph
		, ru::Config const & ruConfig
		, rq::Config rqConfig )
		: RunnablePass{ pass
			, context
			, graph
			, { defaultV< InitialiseCallback >
				, GetPipelineStateCallback( [](){ return crg::getPipelineState( PipelineStageFlags::eColorAttachmentOutput ); } )
				, [this]( RecordContext & recContext, VkCommandBuffer cb, uint32_t i ){ doRecordInto( recContext, cb, i ); }
				, GetPassIndexCallback( [this](){ return m_renderQuad.getPassIndex(); } )
				, IsEnabledCallback( [this](){ return m_renderQuad.isEnabled(); } ) }
			, { ruConfig.maxPassCount
				, true /*resettable*/
				, ruConfig.prePassActions
				, ruConfig.postPassActions
				, ruConfig.implicitActions } }
		, m_renderQuad{ pass
			, context
			, graph
			, rqConfig
			, ruConfig.maxPassCount }
		, m_renderPass{ pass
			, context
			, graph
			, ruConfig.maxPassCount
			, rqConfig.m_renderSize ? *rqConfig.m_renderSize : getDefaultV< Extent2D >() }
	{
	}

	void RenderQuad::resetPipeline( VkPipelineShaderStageCreateInfoArray config
		, uint32_t index )
	{
		resetCommandBuffer( index );
		m_renderQuad.resetPipeline( std::move( config ), index );
		reRecordCurrent();
	}

	void RenderQuad::doRecordInto( RecordContext & context
		, VkCommandBuffer commandBuffer
		, uint32_t index )
	{
		if ( m_renderPass.initialise( context, *this, index ) )
		{
			m_renderQuad.initialise( m_renderPass.getRenderSize()
				, m_renderPass.getRenderPass( index )
				, m_renderPass.createBlendState()
				, index );
		}

		m_renderPass.begin( context, commandBuffer, VK_SUBPASS_CONTENTS_INLINE, index );
		m_renderQuad.record( context, commandBuffer, index );
		m_renderPass.end( context, commandBuffer );
		m_renderQuad.end( context, commandBuffer, index );
	}
}
