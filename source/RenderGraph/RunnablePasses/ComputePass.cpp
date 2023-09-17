/*
See LICENSE file in root folder.
*/
#include "RenderGraph/RunnablePasses/ComputePass.hpp"

#include "RenderGraph/GraphContext.hpp"
#include "RenderGraph/ImageData.hpp"
#include "RenderGraph/RunnableGraph.hpp"

#include <array>

namespace crg
{
	ComputePass::ComputePass( FramePass const & pass
		, GraphContext & context
		, RunnableGraph & graph
		, ru::Config ruConfig
		, cp::Config cpConfig )
		: RunnablePass{ pass
			, context
			, graph
			, { [this]( uint32_t index ){ doInitialise( index ); }
				, GetPipelineStateCallback( [](){ return crg::getPipelineState( VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT ); } )
				, [this]( RecordContext & recContext, VkCommandBuffer cb, uint32_t i ){ doRecordInto( recContext, cb, i ); }
				, GetPassIndexCallback( [this](){ return doGetPassIndex(); } )
				, IsEnabledCallback( [this](){ return doIsEnabled(); } )
				, IsComputePassCallback( [](){ return true; } ) }
			, ruConfig }
		, m_cpConfig{ cpConfig.m_initialise ? std::move( *cpConfig.m_initialise ) : defaultV< RunnablePass::InitialiseCallback >
			, cpConfig.m_enabled ? std::move( *cpConfig.m_enabled ) : defaultV< bool const * >
			, cpConfig.m_isEnabled
			, cpConfig.m_getPassIndex ? std::move( *cpConfig.m_getPassIndex ) : defaultV< RunnablePass::GetPassIndexCallback >
			, cpConfig.m_recordInto ? std::move( *cpConfig.m_recordInto ) : getDefaultV< RunnablePass::RecordCallback >()
			, cpConfig.m_end ? std::move( *cpConfig.m_end ) : getDefaultV< RunnablePass::RecordCallback >()
			, cpConfig.m_groupCountX ? std::move( *cpConfig.m_groupCountX ) : 1u
			, cpConfig.m_groupCountY ? std::move( *cpConfig.m_groupCountY ) : 1u
			, cpConfig.m_groupCountZ ? std::move( *cpConfig.m_groupCountZ ) : 1u
			, cpConfig.m_indirectBuffer ? *cpConfig.m_indirectBuffer : getDefaultV < IndirectBuffer >() }
		, m_pipeline{ pass
			, context
			, graph
			, std::move( cpConfig.m_baseConfig )
			, VK_PIPELINE_BIND_POINT_COMPUTE
			, ruConfig.maxPassCount }
	{
	}

	void ComputePass::resetPipeline( VkPipelineShaderStageCreateInfoArray config
		, uint32_t index )
	{
		resetCommandBuffer( index );
		m_pipeline.resetPipeline( std::move( config ), index );
		doCreatePipeline( index );
		reRecordCurrent();
	}

	VkPipelineLayout ComputePass::getPipelineLayout()const
	{
		return m_pipeline.getPipelineLayout();
	}

	void ComputePass::doInitialise( uint32_t index )
	{
		m_pipeline.initialise();
		doCreatePipeline( index );
		m_cpConfig.initialise( index );
	}

	uint32_t ComputePass::doGetPassIndex()const
	{
		return m_cpConfig.getPassIndex();
	}

	bool ComputePass::doIsEnabled()const
	{
		return ( m_cpConfig.isEnabled
			? ( *m_cpConfig.isEnabled )( )
			: ( m_cpConfig.enabled
				? *m_cpConfig.enabled
				: true ) );
	}

	void ComputePass::doRecordInto( RecordContext & context
		, VkCommandBuffer commandBuffer
		, uint32_t index )
	{
		m_pipeline.recordInto( context, commandBuffer, index );
		m_cpConfig.recordInto( context, commandBuffer, index );

		if ( auto indirectBuffer = m_cpConfig.indirectBuffer.buffer.buffer( index ) )
		{ 
			m_context.vkCmdDispatchIndirect( commandBuffer, indirectBuffer, m_cpConfig.indirectBuffer.offset );
		}
		else
		{
			m_context.vkCmdDispatch( commandBuffer, m_cpConfig.groupCountX, m_cpConfig.groupCountY, m_cpConfig.groupCountZ );
		}

		m_cpConfig.end( context, commandBuffer, index );
	}

	void ComputePass::doCreatePipeline( uint32_t index )
	{
		auto & program = m_pipeline.getProgram( index );
		VkComputePipelineCreateInfo createInfo{ VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO
			, nullptr
			, 0u
			, program.front()
			, m_pipeline.getPipelineLayout()
			, VkPipeline{}
			, 0u };
		m_pipeline.createPipeline( index, createInfo );
	}
}
