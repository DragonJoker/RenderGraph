/*
See LICENSE file in root folder.
*/
#include "RenderGraph/RunnablePasses/RenderPass.hpp"

#include "RenderGraph/Attachment.hpp"
#include "RenderGraph/GraphContext.hpp"
#include "RenderGraph/ImageData.hpp"
#include "RenderGraph/RunnableGraph.hpp"

#include <array>

namespace crg
{
	//*********************************************************************************************

	VkDescriptorPoolSizeArray getBindingsSizes( VkDescriptorSetLayoutBindingArray const & bindings
		, uint32_t maxSets )
	{
		VkDescriptorPoolSizeArray result;

		for ( auto & binding : bindings )
		{
			auto it = std::find_if( result.begin()
				, result.end()
				, [&binding]( VkDescriptorPoolSize const & lookup )
				{
					return binding.descriptorType == lookup.type;
				} );

			if ( it == result.end() )
			{
				result.push_back( { binding.descriptorType, binding.descriptorCount * maxSets } );
			}
			else
			{
				it->descriptorCount += binding.descriptorCount * maxSets;
			}
		}

		return result;
	}

	//*********************************************************************************************

	RenderPass::Callbacks::Callbacks( InitialiseCallback initialise
		, RecordCallback record )
		: Callbacks{ std::move( initialise )
			, std::move( record )
			, getDefaultV< RecordCallback >()
			, getDefaultV< GetSubpassContentsCallback >()
			, getDefaultV< GetPassIndexCallback >()
			, getDefaultV< IsEnabledCallback >() }
	{
	}

	RenderPass::Callbacks::Callbacks( InitialiseCallback initialise
		, RecordCallback record
		, RecordCallback recordDisabled )
		: Callbacks{ std::move( initialise )
			, std::move( record )
			, std::move( recordDisabled )
			, getDefaultV< GetSubpassContentsCallback >()
			, getDefaultV< GetPassIndexCallback >()
			, getDefaultV< IsEnabledCallback >() }
	{
	}

	RenderPass::Callbacks::Callbacks( InitialiseCallback initialise
		, RecordCallback record
		, RecordCallback recordDisabled
		, GetSubpassContentsCallback getSubpassContents )
		: Callbacks{ std::move( initialise )
			, std::move( record )
			, std::move( recordDisabled )
			, std::move( getSubpassContents )
			, getDefaultV< GetPassIndexCallback >()
			, getDefaultV< IsEnabledCallback >() }
	{
	}

	RenderPass::Callbacks::Callbacks( InitialiseCallback initialise
		, RecordCallback record
		, RecordCallback recordDisabled
		, GetSubpassContentsCallback getSubpassContents
		, GetPassIndexCallback getPassIndex )
		: Callbacks{ std::move( initialise )
			, std::move( record )
			, std::move( recordDisabled )
			, std::move( getSubpassContents )
			, std::move( getPassIndex )
			, getDefaultV< IsEnabledCallback >() }
	{
	}

	RenderPass::Callbacks::Callbacks( InitialiseCallback initialise
		, RecordCallback record
		, RecordCallback recordDisabled
		, GetSubpassContentsCallback getSubpassContents
		, GetPassIndexCallback getPassIndex
		, IsEnabledCallback isEnabled )
		: initialise{ std::move( initialise ) }
		, record{ std::move( record ) }
		, recordDisabled{ std::move( recordDisabled ) }
		, getSubpassContents{ std::move( getSubpassContents ) }
		, getPassIndex{ std::move( getPassIndex ) }
		, isEnabled{ std::move( isEnabled ) }
	{
	}

	//*********************************************************************************************

	RenderPass::RenderPass( FramePass const & pass
		, GraphContext & context
		, RunnableGraph & graph
		, Callbacks callbacks
		, VkExtent2D const & size
		, uint32_t maxPassCount
		, bool optional )
		: RunnablePass{ pass
			, context
			, graph
			, { [this](){ doInitialise(); }
				, GetSemaphoreWaitFlagsCallback( [this](){ return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; } )
				, [this]( VkCommandBuffer cb, uint32_t i ){ doRecordInto( cb, i ); }
				, [this]( VkCommandBuffer cb, uint32_t i ){ doRecordDisabledInto( cb, i ); }
				, std::move( callbacks.getPassIndex )
				, std::move( callbacks.isEnabled ) }
			, maxPassCount
			, optional }
		, m_holder{ pass
			, context
			, graph
			, maxPassCount
			, size }
		, m_rpCallbacks{ std::move( callbacks ) }
	{
	}

	void RenderPass::doInitialise()
	{
		m_holder.initialise( *this );
		m_rpCallbacks.initialise();
	}

	void RenderPass::doRecordInto( VkCommandBuffer commandBuffer
		, uint32_t index )
	{
		m_holder.begin( commandBuffer
			, m_rpCallbacks.getSubpassContents()
			, index );
		m_rpCallbacks.record( commandBuffer, index );
		m_holder.end( commandBuffer );
	}

	void RenderPass::doRecordDisabledInto( VkCommandBuffer commandBuffer
		, uint32_t index )
	{
		m_holder.begin( commandBuffer
			, m_rpCallbacks.getSubpassContents()
			, index );
		m_holder.end( commandBuffer );
		m_rpCallbacks.recordDisabled( commandBuffer, index );
	}

	//*********************************************************************************************
}
