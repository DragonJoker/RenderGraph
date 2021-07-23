/*
See LICENSE file in root folder.
*/
#pragma once

#include "RenderGraph/RunnablePass.hpp"

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

	class RenderPassHolder
	{
	public:
		CRG_API RenderPassHolder( FramePass const & pass
			, GraphContext & context
			, RunnableGraph & graph
			, uint32_t maxPassCount
			, VkExtent2D const & size = {} );
		CRG_API ~RenderPassHolder();

		CRG_API void initialise( crg::RunnablePass const & runnable );
		CRG_API VkRenderPassBeginInfo getBeginInfo( uint32_t index );
		CRG_API void begin( VkCommandBuffer commandBuffer
			, VkSubpassContents subpassContents
			, uint32_t index );
		CRG_API void end( VkCommandBuffer commandBuffer );
		CRG_API VkPipelineColorBlendStateCreateInfo createBlendState();

		VkRenderPass getRenderPass()const
		{
			return m_renderPass;
		}

		VkFramebuffer getFramebuffer( uint32_t index )const
		{
			return m_frameBuffers[index];
		}

		VkRect2D const & getRenderArea()const
		{
			return m_renderArea;
		}

		std::vector< VkClearValue > const & getClearValues()const
		{
			return m_clearValues;
		}

		VkPipelineColorBlendAttachmentStateArray const & getBlendAttachs()const
		{
			return m_blendAttachs;
		}

	protected:
		CRG_API void doCreateRenderPass( crg::RunnablePass const & runnable );
		CRG_API void doCreateFramebuffer();

	protected:
		FramePass const & m_pass;
		GraphContext & m_context;
		RunnableGraph & m_graph;
		VkExtent2D m_size;
		VkRenderPass m_renderPass{ VK_NULL_HANDLE };
		std::vector< VkFramebuffer > m_frameBuffers;
		VkRect2D m_renderArea{};
		std::vector< VkClearValue > m_clearValues;
		VkPipelineColorBlendAttachmentStateArray m_blendAttachs;
	};

	class RenderPass
		: public RunnablePass
	{
	public:
		template< typename ConfigT, typename BuilderT >
		friend class RenderQuadBuilderT;

		struct SubpassContentsT;
		using GetSubpassContentsCallback = RunnablePass::GetValueCallbackT< SubpassContentsT, VkSubpassContents >;

		struct Callbacks
		{
			CRG_API Callbacks( InitialiseCallback initialise
				, RecordCallback record );
			CRG_API Callbacks( InitialiseCallback initialise
				, RecordCallback record
				, RecordCallback recordDisabled );
			CRG_API Callbacks( InitialiseCallback initialise
				, RecordCallback record
				, RecordCallback recordDisabled
				, GetSubpassContentsCallback getSubpassContents );
			CRG_API Callbacks( InitialiseCallback initialise
				, RecordCallback record
				, RecordCallback recordDisabled
				, GetSubpassContentsCallback getSubpassContents
				, GetPassIndexCallback getPassIndex );
			CRG_API Callbacks( InitialiseCallback initialise
				, RecordCallback record
				, RecordCallback recordDisabled
				, GetSubpassContentsCallback getSubpassContents
				, GetPassIndexCallback getPassIndex
				, IsEnabledCallback isEnabled );

			// RenderPass specifics
			RunnablePass::InitialiseCallback initialise;
			RunnablePass::RecordCallback record;
			RunnablePass::RecordCallback recordDisabled;
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
			, uint32_t maxPassCount = 1u
			, bool optional = false );

		VkRenderPass getRenderPass()const
		{
			return m_holder.getRenderPass();
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

	private:
		void doInitialise();
		void doRecordInto( VkCommandBuffer commandBuffer
			, uint32_t index );
		void doRecordDisabledInto( VkCommandBuffer commandBuffer
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
