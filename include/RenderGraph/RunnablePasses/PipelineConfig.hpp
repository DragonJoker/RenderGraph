/*
See LICENSE file in root folder.
*/
#pragma once

#include "RenderGraph/RunnablePass.hpp"
#include "RenderGraph/WriteDescriptorSet.hpp"

#include <optional>

namespace crg
{
	struct ProgramCreator
	{
		uint32_t maxCount{};
		std::function< VkPipelineShaderStageCreateInfoArray( uint32_t ) > create;
	};

	template<>
	struct DefaultValueGetterT< std::vector< VkPipelineShaderStageCreateInfoArray > >
	{
		static inline std::vector< VkPipelineShaderStageCreateInfoArray > const value{};

		static std::vector< VkPipelineShaderStageCreateInfoArray > get()
		{
			return value;
		}
	};
	
	template<>
	struct DefaultValueGetterT< std::vector< VkDescriptorSetLayout > >
	{
		static inline std::vector< VkDescriptorSetLayout > const value{};

		static std::vector< VkDescriptorSetLayout > get()
		{
			return value;
		}
	};

	template<>
	struct DefaultValueGetterT< Extent2D >
	{
		static inline Extent2D const value{};

		static Extent2D get()
		{
			return value;
		}
	};

	template<>
	struct DefaultValueGetterT< Offset2D >
	{
		static inline Offset2D const value{};

		static Offset2D get()
		{
			return value;
		}
	};

	template<>
	struct DefaultValueGetterT< ProgramCreator >
	{
		static inline ProgramCreator const value{};

		static ProgramCreator get()
		{
			return value;
		}
	};

	template<>
	struct DefaultValueGetterT< VkPipelineDepthStencilStateCreateInfo >
	{
		static inline VkPipelineDepthStencilStateCreateInfo const value{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO
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

		static VkPipelineDepthStencilStateCreateInfo get()
		{
			return value;
		}
	};

	template<>
	struct DefaultValueGetterT< VkPushConstantRangeArray >
	{
		static inline VkPushConstantRangeArray const value{};

		static VkPushConstantRangeArray get()
		{
			return value;
		}
	};

	template<>
	struct DefaultValueGetterT< uint32_t const * >
	{
		static inline uint32_t const * const value{};

		static uint32_t const * get()
		{
			return value;
		}
	};

	template<>
	struct DefaultValueGetterT< bool const * >
	{
		static inline bool const * const value{};

		static bool const * get()
		{
			return value;
		}
	};

	namespace pp
	{
		template< template< typename ValueT > typename WrapperT >
		struct ConfigT
		{
			ConfigT( ConfigT && )noexcept = default;
			ConfigT & operator=( ConfigT && )noexcept = default;
			ConfigT( ConfigT const & ) = default;
			ConfigT & operator=( ConfigT const & ) = default;
			explicit ConfigT( WrapperT< std::vector< VkPipelineShaderStageCreateInfoArray > > programs = {}
				, WrapperT< ProgramCreator > programCreator = {}
				, WrapperT< std::vector< VkDescriptorSetLayout > > layouts = {}
				, WrapperT< VkPushConstantRangeArray > pushConstants = {} )
				: m_programs{ std::move( programs ) }
				, m_programCreator{ std::move( programCreator ) }
				, m_layouts{ std::move( layouts ) }
				, m_pushConstants{ std::move( pushConstants ) }
			{
			}
			/**
			*\param[in] config
			*	The pipeline program.
			*/
			auto & program( VkPipelineShaderStageCreateInfoArray const & config )
			{
				m_programs = { config };
				return *this;
			}
			/**
			*\param[in] config
			*	The pipeline programs.
			*/
			auto & programs( std::vector< VkPipelineShaderStageCreateInfoArray > const & config )
			{
				m_programs = config;
				return *this;
			}
			/**
			*\param[in] config
			*	The pipeline program creator.
			*/
			auto & programCreator( ProgramCreator const & config )
			{
				m_programCreator = config;
				return *this;
			}
			/**
			*\param[in] config
			*	The descriptor set layout.
			*/
			auto & layout( VkDescriptorSetLayout const & config )
			{
				m_layouts = { config };
				return *this;
			}
			/**
			*\param[in] config
			*	The descriptor set layouts.
			*/
			auto & layouts( std::vector< VkDescriptorSetLayout > const & config )
			{
				m_layouts = config;
				return *this;
			}
			/**
			*\param[in] config
			*	The push constants range.
			*/
			auto & pushConstants( VkPushConstantRange const & config )
			{
				pushConstants( VkPushConstantRangeArray{ config } );
				return *this;
			}
			/**
			*\param[in] config
			*	The push constants range.
			*/
			auto & pushConstants( VkPushConstantRangeArray const & config )
			{
				m_pushConstants = config;
				return *this;
			}

			WrapperT< std::vector< VkPipelineShaderStageCreateInfoArray > > m_programs;
			WrapperT< ProgramCreator > m_programCreator;
			WrapperT< std::vector< VkDescriptorSetLayout > > m_layouts;
			WrapperT< VkPushConstantRangeArray > m_pushConstants;
		};

		using Config = ConfigT< std::optional >;
		using ConfigData = ConfigT< RawTypeT >;
	}

	template<>
	struct DefaultValueGetterT< pp::Config >
	{
		static inline pp::Config const value{ [](){ return pp::Config{}; }() };

		static pp::Config get()
		{
			return value;
		}
	};
}
