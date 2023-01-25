/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#include "RenderGraph/RunnablePasses/RenderMesh.hpp"

namespace crg
{
	RenderMesh::RenderMesh( FramePass const & pass
		, GraphContext & context
		, RunnableGraph & graph
		, ru::Config ruConfig
		, rm::Config rmConfig )
		: RunnablePass{ pass
			, context
			, graph
			, { [this](){ doInitialise(); }
				, GetPipelineStateCallback( [](){ return crg::getPipelineState( VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT ); } )
				, [this]( RecordContext & recContext, VkCommandBuffer cb, uint32_t i ){ doRecordInto( recContext, cb, i ); }
				, GetPassIndexCallback( [this](){ return m_renderMesh.getPassIndex(); } )
				, IsEnabledCallback( [this](){ return m_renderMesh.isEnabled(); } ) }
			, { ruConfig.maxPassCount
				, true /*resettable*/
				, ruConfig.actions } }
		, m_renderMesh{ pass
			, context
			, graph
			, std::move( rmConfig )
			, ruConfig.maxPassCount }
		, m_renderPass{ pass
			, context
			, graph
			, ruConfig.maxPassCount
			, m_renderMesh.getRenderSize() }
	{
	}

	void RenderMesh::resetPipeline( VkPipelineShaderStageCreateInfoArray config )
	{
		resetCommandBuffer();
		m_renderMesh.resetPipeline( std::move( config ) );
		reRecordCurrent();
	}

	void RenderMesh::doInitialise()
	{
	}

	void RenderMesh::doRecordInto( RecordContext & context
		, VkCommandBuffer commandBuffer
		, uint32_t index )
	{
		if ( m_renderPass.initialise( context, *this, index ) )
		{
			m_renderMesh.cleanup();
			m_renderMesh.initialise( *this
				, m_renderPass.getRenderSize()
				, m_renderPass.getRenderPass( index )
				, m_renderPass.createBlendState() );
		}

		m_renderPass.begin( context, commandBuffer, VK_SUBPASS_CONTENTS_INLINE, index );
		m_renderMesh.record( context, commandBuffer, index );
		m_renderPass.end( context, commandBuffer );
		m_renderMesh.end( context, commandBuffer, index );
	}
}
