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
		, ru::Config ruConfig
		, rq::Config rqConfig )
		: RunnablePass{ pass
			, context
			, graph
			, { [this]( uint32_t ){ doInitialise(); }
				, GetPipelineStateCallback( [](){ return crg::getPipelineState( VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT ); } )
				, [this]( RecordContext & recContext, VkCommandBuffer cb, uint32_t i ){ doRecordInto( recContext, cb, i ); }
				, GetPassIndexCallback( [this](){ return m_renderQuad.getPassIndex(); } )
				, IsEnabledCallback( [this](){ return m_renderQuad.isEnabled(); } ) }
			, { ruConfig.maxPassCount
				, true /*resettable*/
				, ruConfig.actions } }
		, m_renderQuad{ pass
			, context
			, graph
			, rqConfig
			, ruConfig.maxPassCount }
		, m_renderPass{ pass
			, context
			, graph
			, ruConfig.maxPassCount
			, std::move( rqConfig.m_renderSize ? *rqConfig.m_renderSize : defaultV< VkExtent2D > ) }
	{
	}

	void RenderQuad::resetPipeline( VkPipelineShaderStageCreateInfoArray config
		, uint32_t index )
	{
		resetCommandBuffer( index );
		m_renderQuad.resetPipeline( std::move( config ), index );
		reRecordCurrent();
	}

	void RenderQuad::doInitialise()
	{
	}

	void RenderQuad::doRecordInto( RecordContext & context
		, VkCommandBuffer commandBuffer
		, uint32_t index )
	{
		if ( m_renderPass.initialise( context, *this, index ) )
		{
			m_renderQuad.initialise( *this
				, m_renderPass.getRenderSize()
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
