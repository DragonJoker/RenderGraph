/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#pragma once

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
			*	The texture coordinates configuration.
			*/
			auto & texcoordConfig( Texcoord config )
			{
				m_texcoordConfig = std::move( config );
				return *this;
			}
			/**
			*\param[in] config
			*	The render area size.
			*/
			auto & renderSize( VkExtent2D config )
			{
				m_renderSize = std::move( config );
				return *this;
			}
			/**
			*\param[in] config
			*	The render position.
			*/
			auto & renderPosition( VkOffset2D config )
			{
				m_renderPosition = std::move( config );
				return *this;
			}
			/**
			*\param[in] config
			*	The depth stencil state.
			*/
			auto & depthStencilState( VkPipelineDepthStencilStateCreateInfo config )
			{
				m_depthStencilState = std::move( config );
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
			*	The callback to recording the pass.
			*/
			auto & recordInto( RunnablePass::RecordCallback config )
			{
				m_recordInto = config;
				return *this;
			}

			pp::ConfigT< WrapperT > m_baseConfig;
			WrapperT< Texcoord > m_texcoordConfig;
			WrapperT< VkOffset2D > m_renderPosition;
			WrapperT< VkPipelineDepthStencilStateCreateInfo > m_depthStencilState;
			WrapperT< uint32_t const * > m_passIndex;
			WrapperT< bool const * > m_enabled;
			WrapperT< RunnablePass::IsEnabledCallback > m_isEnabled;
			WrapperT< RunnablePass::RecordCallback > m_recordInto;
			WrapperT< VkExtent2D > m_renderSize;
		};

		template<>
		struct ConfigT< RawTypeT >
		{
			RawTypeT< Texcoord > texcoordConfig;
			RawTypeT< VkOffset2D > renderPosition;
			RawTypeT< VkPipelineDepthStencilStateCreateInfo > depthStencilState;
			RawTypeT< uint32_t const * > passIndex;
			RawTypeT< bool const * > enabled;
			std::optional< RunnablePass::IsEnabledCallback > isEnabled;
			RawTypeT< RunnablePass::RecordCallback > recordInto;
		};

		using Config = ConfigT< std::optional >;
		using ConfigData = ConfigT< RawTypeT >;
	}

	template<>
	struct DefaultValueGetterT< Texcoord >
	{
		static Texcoord get()
		{
			static Texcoord const result{ false, false };
			return result;
		}
	};

	template<>
	struct DefaultValueGetterT< VkExtent2D >
	{
		static VkExtent2D get()
		{
			static VkExtent2D const result{};
			return result;
		}
	};

	template<>
	struct DefaultValueGetterT< VkOffset2D >
	{
		static VkOffset2D get()
		{
			static VkOffset2D const result{};
			return result;
		}
	};

	template<>
	struct DefaultValueGetterT< VkPipelineDepthStencilStateCreateInfo >
	{
		static VkPipelineDepthStencilStateCreateInfo get()
		{
			static VkPipelineDepthStencilStateCreateInfo const result{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO
				, nullptr
				, 0u
				, VK_FALSE
				, VK_FALSE
				, {}
				, {}
				, {}
				, {}
				, {}
				, {}
				, {} };
			return result;
		}
	};

	template<>
	struct DefaultValueGetterT< uint32_t const * >
	{
		static uint32_t const * get()
		{
			static uint32_t const * const result{};
			return result;
		}
	};

	template<>
	struct DefaultValueGetterT< bool const * >
	{
		static bool const * get()
		{
			static bool const * const result{};
			return result;
		}
	};

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
