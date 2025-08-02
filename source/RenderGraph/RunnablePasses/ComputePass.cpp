/*
See LICENSE file in root folder.
*/
#include "RenderGraph/RunnablePasses/ComputePass.hpp"

#include "RenderGraph/GraphContext.hpp"
#include "RenderGraph/RunnableGraph.hpp"

#include <array>

namespace crg
{
	namespace cppss
	{
		static bool isPtrEnabled( bool const * v )
		{
			return v ? *v : true;
		}
	}

	ComputePass::ComputePass( FramePass const & pass
		, GraphContext & context
		, RunnableGraph & graph
		, ru::Config const & ruConfig
		, cp::Config cpConfig )
		: RunnablePass{ pass
			, context
			, graph
			, { [this]( uint32_t index ){ doInitialise( index ); }
				, GetPipelineStateCallback( [](){ return crg::getPipelineState( PipelineStageFlags::eComputeShader ); } )
				, [this]( RecordContext & recContext, VkCommandBuffer cb, uint32_t i ){ doRecordInto( recContext, cb, i ); }
				, GetPassIndexCallback( [this](){ return doGetPassIndex(); } )
				, IsEnabledCallback( [this](){ return doIsEnabled(); } )
				, IsComputePassCallback( [](){ return true; } ) }
			, ruConfig }
		, m_cpConfig{ cpConfig.m_initialise ? std::move( *cpConfig.m_initialise ) : getDefaultV< RunnablePass::InitialiseCallback >()
			, cpConfig.m_enabled.has_value() ? std::move( *cpConfig.m_enabled ) : getDefaultV< bool const * >()
			, cpConfig.m_isEnabled
			, cpConfig.m_getPassIndex ? std::move( *cpConfig.m_getPassIndex ) : getDefaultV< RunnablePass::GetPassIndexCallback >()
			, cpConfig.m_recordInto ? std::move( *cpConfig.m_recordInto ) : getDefaultV< RunnablePass::RecordCallback >()
			, cpConfig.m_end ? std::move( *cpConfig.m_end ) : getDefaultV< RunnablePass::RecordCallback >()
			, cpConfig.m_groupCountX.has_value() ? *cpConfig.m_groupCountX : 1u
			, cpConfig.m_groupCountY.has_value() ? *cpConfig.m_groupCountY : 1u
			, cpConfig.m_groupCountZ.has_value() ? *cpConfig.m_groupCountZ : 1u
			, cpConfig.m_getGroupCountX ? std::optional< cp::GetGroupCountCallback >( std::move( *cpConfig.m_getGroupCountX ) ) : std::nullopt
			, cpConfig.m_getGroupCountY ? std::optional< cp::GetGroupCountCallback >( std::move( *cpConfig.m_getGroupCountY ) ) : std::nullopt
			, cpConfig.m_getGroupCountZ ? std::optional< cp::GetGroupCountCallback >( std::move( *cpConfig.m_getGroupCountZ ) ) : std::nullopt
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
			? ( *m_cpConfig.isEnabled )()
			: cppss::isPtrEnabled( m_cpConfig.enabled ) );
	}

	void ComputePass::doRecordInto( RecordContext & context
		, VkCommandBuffer commandBuffer
		, uint32_t index )
	{
		m_pipeline.recordInto( context, commandBuffer, index );
		m_cpConfig.recordInto( context, commandBuffer, index );

		if ( m_cpConfig.indirectBuffer != defaultV< IndirectBuffer > )
		{
			auto indirectBuffer = m_graph.createBuffer( m_cpConfig.indirectBuffer.buffer.data->buffer );
			context->vkCmdDispatchIndirect( commandBuffer, indirectBuffer, getSubresourceRange( m_cpConfig.indirectBuffer.buffer ).offset );
		}
		else
		{
			context->vkCmdDispatch( commandBuffer
				, ( m_cpConfig.getGroupCountX ? ( *m_cpConfig.getGroupCountX )() : m_cpConfig.groupCountX )
				, ( m_cpConfig.getGroupCountY ? ( *m_cpConfig.getGroupCountY )() : m_cpConfig.groupCountY )
				, ( m_cpConfig.getGroupCountZ ? ( *m_cpConfig.getGroupCountZ )() : m_cpConfig.groupCountZ ) );
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
			, getPipelineLayout()
			, VkPipeline{}
			, 0u };
		m_pipeline.createPipeline( index, createInfo );
	}
}
