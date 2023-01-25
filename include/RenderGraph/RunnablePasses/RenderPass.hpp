/*
See LICENSE file in root folder.
*/
#pragma once

#include "RenderGraph/RunnablePasses/RenderPassHolder.hpp"

namespace crg
{
	CRG_API VkDescriptorPoolSizeArray getBindingsSizes( VkDescriptorSetLayoutBindingArray const & bindings
		, uint32_t maxSets );

	template< typename VkType, typename LibType >
	inline std::vector< VkType > makeVkArray( std::vector< LibType > const & input )
	{
		std::vector< VkType > result;
		result.reserve( input.size() );

		for ( auto const & element : input )
		{
			result.emplace_back( element );
		}

		return result;
	}

	class RenderPass
		: public RunnablePass
	{
	public:
		template< typename ConfigT, typename BuilderT >
		friend class RenderQuadBuilderT;

		struct SubpassContentsT;
		using GetSubpassContentsCallback = GetValueCallbackT< SubpassContentsT, VkSubpassContents >;

		struct Callbacks
		{
			CRG_API Callbacks( InitialiseCallback initialise
				, RecordCallback record );
			CRG_API Callbacks( InitialiseCallback initialise
				, RecordCallback record
				, GetSubpassContentsCallback getSubpassContents );
			CRG_API Callbacks( InitialiseCallback initialise
				, RecordCallback record
				, GetSubpassContentsCallback getSubpassContents
				, GetPassIndexCallback getPassIndex );
			CRG_API Callbacks( InitialiseCallback initialise
				, RecordCallback record
				, GetSubpassContentsCallback getSubpassContents
				, GetPassIndexCallback getPassIndex
				, IsEnabledCallback isEnabled );

			// RenderPass specifics
			RunnablePass::InitialiseCallback initialise;
			RunnablePass::RecordCallback record;
			GetSubpassContentsCallback getSubpassContents;
			// Passed to RunnablePass
			RunnablePass::GetPassIndexCallback getPassIndex;
			RunnablePass::IsEnabledCallback isEnabled;
		};

	public:
		CRG_API RenderPass( FramePass const & pass
			, GraphContext & context
			, RunnableGraph & graph
			, Callbacks callbacks
			, VkExtent2D const & size = {}
			, ru::Config ruConfig = {} );

		VkRenderPass getRenderPass( uint32_t passIndex )const
		{
			return m_holder.getRenderPass( passIndex );
		}

	protected:
		VkPipelineColorBlendStateCreateInfo doCreateBlendState()
		{
			return m_holder.createBlendState();
		}

		VkPipelineColorBlendAttachmentStateArray const & doGetBlendAttachs()const
		{
			return m_holder.getBlendAttachs();
		}

		RenderPassHolder const & doGetHolder()const
		{
			return m_holder;
		}

	private:
		void doRecordInto( RecordContext & context
			, VkCommandBuffer commandBuffer
			, uint32_t index );

	private:
		Callbacks m_rpCallbacks;
		RenderPassHolder m_holder;
	};

	template<>
	struct DefaultValueGetterT< RenderPass::GetSubpassContentsCallback >
	{
		static RenderPass::GetSubpassContentsCallback get()
		{
			RenderPass::GetSubpassContentsCallback const result{ [](){ return VK_SUBPASS_CONTENTS_INLINE; } };
			return result;
		}
	};
}
