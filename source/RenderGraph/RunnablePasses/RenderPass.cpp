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
			, getDefaultV< GetSubpassContentsCallback >()
			, getDefaultV< GetPassIndexCallback >()
			, getDefaultV< IsEnabledCallback >() }
	{
	}

	RenderPass::Callbacks::Callbacks( InitialiseCallback initialise
		, RecordCallback record
		, GetSubpassContentsCallback getSubpassContents )
		: Callbacks{ std::move( initialise )
			, std::move( record )
			, std::move( getSubpassContents )
			, getDefaultV< GetPassIndexCallback >()
			, getDefaultV< IsEnabledCallback >() }
	{
	}

	RenderPass::Callbacks::Callbacks( InitialiseCallback initialise
		, RecordCallback record
		, GetSubpassContentsCallback getSubpassContents
		, GetPassIndexCallback getPassIndex )
		: Callbacks{ std::move( initialise )
			, std::move( record )
			, std::move( getSubpassContents )
			, std::move( getPassIndex )
			, getDefaultV< IsEnabledCallback >() }
	{
	}

	RenderPass::Callbacks::Callbacks( InitialiseCallback initialise
		, RecordCallback record
		, GetSubpassContentsCallback getSubpassContents
		, GetPassIndexCallback getPassIndex
		, IsEnabledCallback isEnabled )
		: initialise{ std::move( initialise ) }
		, record{ std::move( record ) }
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
		, ru::Config ruConfig )
		: RunnablePass{ pass
			, context
			, graph
			, { [this](){ doInitialise(); }
				, GetSemaphoreWaitFlagsCallback( [](){ return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; } )
				, [this]( RecordContext & recContext, VkCommandBuffer cb, uint32_t i ){ doRecordInto( recContext, cb, i ); }
				, std::move( callbacks.getPassIndex )
				, std::move( callbacks.isEnabled ) }
			, ruConfig }
		, m_rpCallbacks{ std::move( callbacks ) }
		, m_holder{ pass
			, context
			, graph
			, ruConfig.maxPassCount
			, size }
	{
	}

	void RenderPass::doInitialise()
	{
	}

	void RenderPass::doRecordInto( RecordContext & context
		, VkCommandBuffer commandBuffer
		, uint32_t index )
	{
		if ( m_holder.initialise( context, *this ) )
		{
			m_rpCallbacks.initialise();
		}

		m_holder.begin( context
			, commandBuffer
			, m_rpCallbacks.getSubpassContents()
			, index );
		m_rpCallbacks.record( context
			, commandBuffer
			, index );
		m_holder.end( context
			, commandBuffer );
	}

	//*********************************************************************************************
}
