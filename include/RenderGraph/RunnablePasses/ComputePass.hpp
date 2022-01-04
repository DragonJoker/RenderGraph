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
				m_passIndex = config;
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

			pp::ConfigT< WrapperT > m_baseConfig;
			WrapperT< uint32_t const * > m_passIndex;
			WrapperT< bool const * > m_enabled;
			WrapperT< RunnablePass::IsEnabledCallback > m_isEnabled;
			WrapperT< RunnablePass::RecordCallback > m_recordInto;
			WrapperT< RunnablePass::RecordCallback > m_end;
			WrapperT< uint32_t > m_groupCountX;
			WrapperT< uint32_t > m_groupCountY;
			WrapperT< uint32_t > m_groupCountZ;
		};

		template<>
		struct ConfigT< RawTypeT >
		{
			RawTypeT< uint32_t const * > passIndex{ nullptr };
			RawTypeT< bool const * > enabled{ nullptr };
			std::optional< RunnablePass::IsEnabledCallback > isEnabled{};
			RawTypeT< RunnablePass::RecordCallback > recordInto{};
			RawTypeT< RunnablePass::RecordCallback > end{};
			RawTypeT< uint32_t > groupCountX{ 1u };
			RawTypeT< uint32_t > groupCountY{ 1u };
			RawTypeT< uint32_t > groupCountZ{ 1u };
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
		CRG_API void resetPipeline( VkPipelineShaderStageCreateInfoArray config );

	private:
		void doInitialise();
		uint32_t doGetPassIndex()const;
		bool doIsEnabled()const;
		void doRecordInto( RecordContext & context
			, VkCommandBuffer commandBuffer
			, uint32_t index );
		void doCreatePipeline();

	private:
		cp::ConfigData m_cpConfig;
		PipelineHolder m_pipeline;
	};
}
