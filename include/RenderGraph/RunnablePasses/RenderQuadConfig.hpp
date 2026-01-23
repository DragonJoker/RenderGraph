/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "RenderGraph/BufferViewData.hpp"
#include "RenderGraph/RunnableGraph.hpp"
#include "RenderGraph/RunnablePasses/PipelineConfig.hpp"

namespace crg
{
	namespace rq
	{
		template< template< typename ValueT > typename WrapperT >
		struct ConfigT
		{
			/**
			*\param[in] config
			*	The pipeline program.
			*/
			auto & program( VkPipelineShaderStageCreateInfoArray const & config )
			{
				m_baseConfig.program( config );
				return *this;
			}
			/**
			*\param[in] config
			*	The pipeline programs.
			*/
			auto & programs( std::vector< VkPipelineShaderStageCreateInfoArray > const & config )
			{
				m_baseConfig.programs( config );
				return *this;
			}
			/**
			*\param[in] config
			*	The pipeline program creator.
			*/
			auto & programCreator( ProgramCreator const & config )
			{
				m_baseConfig.programCreator( config );
				return *this;
			}
			/**
			*\param[in] config
			*	The descriptor set layout.
			*/
			auto & layout( VkDescriptorSetLayout const & config )
			{
				m_baseConfig.layout( config );
				return *this;
			}
			/**
			*\param[in] config
			*	The descriptor set layouts.
			*/
			auto & layouts( std::vector< VkDescriptorSetLayout > const & config )
			{
				m_baseConfig.layouts( config );
				return *this;
			}
			/**
			*\param[in] config
			*	The push constants range.
			*/
			auto & pushConstants( VkPushConstantRange const & config )
			{
				m_baseConfig.pushConstants( VkPushConstantRangeArray{ config } );
				return *this;
			}
			/**
			*\param[in] config
			*	The push constants range.
			*/
			auto & pushConstants( VkPushConstantRangeArray const & config )
			{
				m_baseConfig.pushConstants( config );
				return *this;
			}
			/**
			*\param[in] config
			*	Tells if disabled pass should record render pass begin/end.
			*/
			auto & baseConfig( pp::Config const & config )
			{
				m_baseConfig = config;
				return *this;
			}
			/**
			*\param[in] config
			*	The texture coordinates configuration.
			*/
			auto & texcoordConfig( Texcoord const & config )
			{
				m_texcoordConfig = config;
				return *this;
			}
			/**
			*\param[in] config
			*	The render area size.
			*/
			auto & renderSize( Extent2D const & config )
			{
				m_renderSize = config;
				return *this;
			}
			/**
			*\param[in] config
			*	The render position.
			*/
			auto & renderPosition( Offset2D const & config )
			{
				m_renderPosition = config;
				return *this;
			}
			/**
			*\param[in] config
			*	The depth stencil state.
			*/
			auto & depthStencilState( VkPipelineDepthStencilStateCreateInfo const & config )
			{
				m_depthStencilState = config;
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
			auto & isEnabled( RunnablePass::IsEnabledCallback const & config )
			{
				m_isEnabled = config;
				return *this;
			}
			/**
			*\param[in] config
			*	The callback recording the pass.
			*/
			auto & recordInto( RunnablePass::RecordCallback const & config )
			{
				m_recordInto = config;
				return *this;
			}
			/**
			*\param[in] config
			*	The callback ending the pass.
			*/
			auto & end( RunnablePass::RecordCallback const & config )
			{
				m_end = config;
				return *this;
			}
			/**
			*\param[in] config
			*	The instances count.
			*/
			auto & instances( uint32_t config )
			{
				m_instances = config;
				return *this;
			}
			/**
			*\param[in] config
			*	The indirect buffer.
			*/
			auto & indirectBuffer( IndirectBuffer const & config )
			{
				m_indirectBuffer = config;
				return *this;
			}

			pp::ConfigT< WrapperT > m_baseConfig{};
			WrapperT< Texcoord > m_texcoordConfig{};
			WrapperT< Offset2D > m_renderPosition{};
			WrapperT< VkPipelineDepthStencilStateCreateInfo > m_depthStencilState{};
			WrapperT< uint32_t const * > m_passIndex{};
			WrapperT< bool const * > m_enabled{};
			WrapperT< RunnablePass::IsEnabledCallback > m_isEnabled{};
			WrapperT< RunnablePass::RecordCallback > m_recordInto{};
			WrapperT< RunnablePass::RecordCallback > m_end{};
			WrapperT< uint32_t > m_instances{};
			WrapperT< Extent2D > m_renderSize{};
			WrapperT< IndirectBuffer > m_indirectBuffer{};
		};

		template<>
		struct ConfigT< RawTypeT >
		{
			RawTypeT< Texcoord > texcoordConfig;
			RawTypeT< Offset2D > renderPosition;
			RawTypeT< VkPipelineDepthStencilStateCreateInfo > depthStencilState;
			RawTypeT< uint32_t const * > passIndex;
			RawTypeT< bool const * > enabled;
			std::optional< RunnablePass::IsEnabledCallback > isEnabled;
			RawTypeT< RunnablePass::RecordCallback > recordInto;
			RawTypeT< RunnablePass::RecordCallback > end;
			RawTypeT< uint32_t > m_instances;
			RawTypeT< IndirectBuffer > indirectBuffer{ BufferViewId{}, 0u };
		};

		using Config = ConfigT< std::optional >;
		using ConfigData = ConfigT< RawTypeT >;
	}

	template<>
	struct DefaultValueGetterT< rq::Config >
	{
		static rq::Config get()
		{
			rq::Config const result{ []()
				{
					return rq::Config{};
				}() };
			return result;
		}
	};
}
