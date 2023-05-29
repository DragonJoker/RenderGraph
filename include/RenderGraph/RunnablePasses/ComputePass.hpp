/*
See LICENSE file in root folder.
*/
#pragma once

#include "RenderGraph/RunnablePass.hpp"
#include "RenderGraph/RunnablePasses/PipelineHolder.hpp"

namespace crg
{
	namespace cp
	{
		template< template< typename ValueT > typename WrapperT >
		struct ConfigT
		{
			/**
			*\param[in] config
			*	The pipeline program.
			*/
			auto & program( VkPipelineShaderStageCreateInfoArray config )
			{
				m_baseConfig.program( std::move( config ) );
				return *this;
			}
			/**
			*\param[in] config
			*	The pipeline programs.
			*/
			auto & programs( std::vector< VkPipelineShaderStageCreateInfoArray > config )
			{
				m_baseConfig.programs( std::move( config ) );
				return *this;
			}
			/**
			*\param[in] config
			*	The pipeline program creator.
			*/
			auto & programCreator( ProgramCreator config )
			{
				m_baseConfig.programCreator( std::move( config ) );
				return *this;
			}
			/**
			*\param[in] config
			*	The descriptor set layout.
			*/
			auto & layout( VkDescriptorSetLayout config )
			{
				m_baseConfig.layout( std::move( config ) );
				return *this;
			}
			/**
			*\param[in] config
			*	The descriptor set layouts.
			*/
			auto & layouts( std::vector< VkDescriptorSetLayout > config )
			{
				m_baseConfig.layouts( std::move( config ) );
				return *this;
			}
			/**
			*\param[in] config
			*	Tells if disabled pass should record render pass begin/end.
			*/
			auto & baseConfig( pp::Config config )
			{
				m_baseConfig = std::move( config );
				return *this;
			}
			/**
			*\param[in] config
			*	The pass index.
			*/
			auto & passIndex( uint32_t const * config )
			{
				m_getPassIndex = [config]()
				{
					return *config;
				};
				return *this;
			}
			/**
			*\param[in] config
			*	The enabled control variable.
			*/
			auto & enabled( bool const * config )
			{
				m_enabled = config;
				return *this;
			}
			/**
			*\param[in] config
			*	The pass index callback.
			*/
			auto & getPassIndex( RunnablePass::GetPassIndexCallback config )
			{
				m_getPassIndex = config;
				return *this;
			}
			/**
			*\param[in] config
			*	The callback checking the enable status of the pass.
			*/
			auto & isEnabled( RunnablePass::IsEnabledCallback config )
			{
				m_isEnabled = config;
				return *this;
			}
			/**
			*\param[in] config
			*	The callback recording the pass.
			*/
			auto & recordInto( RunnablePass::RecordCallback config )
			{
				m_recordInto = config;
				return *this;
			}
			/**
			*\param[in] config
			*	The callback initialising the pass.
			*/
			auto & initialise( RunnablePass::InitialiseCallback config )
			{
				m_initialise = config;
				return *this;
			}
			/**
			*\param[in] config
			*	The callback ending the pass.
			*/
			auto & end( RunnablePass::RecordCallback config )
			{
				m_end = config;
				return *this;
			}
			/**
			*\param[in] config
			*	The X dispatch groups count.
			*/
			auto & groupCountX( uint32_t config )
			{
				m_groupCountX = config;
				return *this;
			}
			/**
			*\param[in] config
			*	The Y dispatch groups count.
			*/
			auto & groupCountY( uint32_t config )
			{
				m_groupCountY = config;
				return *this;
			}
			/**
			*\param[in] config
			*	The Z dispatch groups count.
			*/
			auto & groupCountZ( uint32_t config )
			{
				m_groupCountZ = config;
				return *this;
			}
			/**
			*\param[in] config
			*	The buffer used during indirect compute.
			*/
			auto & indirectBuffer( IndirectBuffer config )
			{
				m_indirectBuffer = config;
				return *this;
			}

			pp::ConfigT< WrapperT > m_baseConfig{};
			WrapperT< RunnablePass::InitialiseCallback > m_initialise{};
			WrapperT< bool const * > m_enabled{};
			WrapperT< RunnablePass::IsEnabledCallback > m_isEnabled{};
			WrapperT< RunnablePass::GetPassIndexCallback > m_getPassIndex{};
			WrapperT< RunnablePass::RecordCallback > m_recordInto{};
			WrapperT< RunnablePass::RecordCallback > m_end{};
			WrapperT< uint32_t > m_groupCountX{};
			WrapperT< uint32_t > m_groupCountY{};
			WrapperT< uint32_t > m_groupCountZ{};
			WrapperT< IndirectBuffer > m_indirectBuffer{};
		};

		template<>
		struct ConfigT< RawTypeT >
		{
			RawTypeT< RunnablePass::InitialiseCallback > initialise{};
			RawTypeT< bool const * > enabled{ nullptr };
			std::optional< RunnablePass::IsEnabledCallback > isEnabled{};
			RawTypeT< RunnablePass::GetPassIndexCallback > getPassIndex{};
			RawTypeT< RunnablePass::RecordCallback > recordInto{};
			RawTypeT< RunnablePass::RecordCallback > end{};
			RawTypeT< uint32_t > groupCountX{ 1u };
			RawTypeT< uint32_t > groupCountY{ 1u };
			RawTypeT< uint32_t > groupCountZ{ 1u };
			RawTypeT< IndirectBuffer > indirectBuffer{ defaultV< IndirectBuffer > };
		};

		using Config = ConfigT< std::optional >;
		using ConfigData = ConfigT< RawTypeT >;
	}

	template<>
	struct DefaultValueGetterT< cp::Config >
	{
		static cp::Config get()
		{
			cp::Config const result{ []()
				{
					return cp::Config{};
				}() };
			return result;
		}
	};

	class ComputePass
		: public RunnablePass
	{
	public:
		CRG_API ComputePass( FramePass const & pass
			, GraphContext & context
			, RunnableGraph & graph
			, ru::Config ruConfig = {}
			, cp::Config cpConfig = {} );
		CRG_API void resetPipeline( VkPipelineShaderStageCreateInfoArray config
			, uint32_t index );

	private:
		void doInitialise( uint32_t index );
		uint32_t doGetPassIndex()const;
		bool doIsEnabled()const;
		void doRecordInto( RecordContext & context
			, VkCommandBuffer commandBuffer
			, uint32_t index );
		void doCreatePipeline( uint32_t index );

	private:
		cp::ConfigData m_cpConfig;
		PipelineHolder m_pipeline;
	};
}
