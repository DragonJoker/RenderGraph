/*
See LICENSE file in root folder.
*/
#pragma once

#include "RenderGraph/RunnablePass.hpp"
#include "RenderGraph/WriteDescriptorSet.hpp"

#include <optional>

namespace crg
{
	template<>
	struct DefaultValueGetterT< std::vector< VkPipelineShaderStageCreateInfoArray > >
	{
		static std::vector< VkPipelineShaderStageCreateInfoArray > get()
		{
			return std::vector< VkPipelineShaderStageCreateInfoArray >{};
		}
	};
	
	template<>
	struct DefaultValueGetterT< std::vector< VkDescriptorSetLayout > >
	{
		static std::vector< VkDescriptorSetLayout > get()
		{
			return std::vector< VkDescriptorSetLayout >{};
		}
	};

	namespace pp
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
				m_programs = { std::move( config ) };
				return *this;
			}
			/**
			*\param[in] config
			*	The pipeline programs.
			*/
			auto & programs( std::vector< VkPipelineShaderStageCreateInfoArray > config )
			{
				m_programs = std::move( config );
				return *this;
			}
			/**
			*\param[in] config
			*	The descriptor set layout.
			*/
			auto & layout( VkDescriptorSetLayout config )
			{
				m_layouts = { std::move( config ) };
				return *this;
			}
			/**
			*\param[in] config
			*	The descriptor set layouts.
			*/
			auto & layouts( std::vector< VkDescriptorSetLayout > config )
			{
				m_layouts = std::move( config );
				return *this;
			}

			WrapperT< std::vector< VkPipelineShaderStageCreateInfoArray > > m_programs;
			WrapperT< std::vector< VkDescriptorSetLayout > > m_layouts;
		};

		using Config = ConfigT< std::optional >;
		using ConfigData = ConfigT< RawTypeT >;
	}

	template<>
	struct DefaultValueGetterT< pp::Config >
	{
		static pp::Config get()
		{
			pp::Config const result{ []()
				{
					return pp::Config{};
				}() };
			return result;
		}
	};
}
