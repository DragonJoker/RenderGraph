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
			result.emplace_back( static_cast< VkType const & >( element ) );
		}

		return result;
	}

	struct SubpassContentsT;
	using GetSubpassContentsCallback = GetValueCallbackT< SubpassContentsT, VkSubpassContents >;
	template<>
	struct DefaultValueGetterT< GetSubpassContentsCallback >
	{
		static GetSubpassContentsCallback get()
		{
			GetSubpassContentsCallback const result{ [](){ return VK_SUBPASS_CONTENTS_INLINE; } };
			return result;
		}
	};

	class RenderPass
		: public RunnablePass
	{
	public:
		template< typename ConfigT, typename BuilderT >
		friend class RenderQuadBuilderT;

		struct Callbacks
		{
			CRG_API Callbacks();

			Callbacks & onInitialise( std::function< void( uint32_t passIndex ) > config )
			{
				initialise = InitialiseCallback{ std::move( config ) };
				return *this;
			}

			Callbacks & onRecord( std::function< void( RecordContext &, VkCommandBuffer, uint32_t ) > config )
			{
				record = RecordCallback{ std::move( config ) };
				return *this;
			}

			Callbacks & onGetSubpassContents( std::function< VkSubpassContents() > config )
			{
				getSubpassContents = GetSubpassContentsCallback{ std::move( config ) };
				return *this;
			}

			Callbacks & onGetPassIndex( std::function< uint32_t() > config )
			{
				getPassIndex = GetPassIndexCallback{ std::move( config ) };
				return *this;
			}

			Callbacks & onIsEnabled( std::function< bool() > config )
			{
				isEnabled = IsEnabledCallback{ std::move( config ) };
				return *this;
			}

			// RenderPass specifics
			InitialiseCallback initialise{ defaultV< InitialiseCallback > };
			RecordCallback record{ defaultV< RecordCallback > };
			GetSubpassContentsCallback getSubpassContents{ defaultV< GetSubpassContentsCallback > };
			// Passed to RunnablePass
			GetPassIndexCallback getPassIndex{ defaultV< GetPassIndexCallback > };
			IsEnabledCallback isEnabled{ defaultV< IsEnabledCallback > };
		};

	public:
		CRG_API RenderPass( FramePass const & pass
			, GraphContext & context
			, RunnableGraph & graph
			, Callbacks callbacks
			, Extent2D size = {}
			, ru::Config const & ruConfig = {} );

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
}
