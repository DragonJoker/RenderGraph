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
	struct DefaultValueGetterT< VkPipelineShaderStageCreateInfoArray >
	{
		static VkPipelineShaderStageCreateInfoArray get()
		{
			return VkPipelineShaderStageCreateInfoArray{};
		}
	};

	namespace pp
	{
		template< template< typename ValueT > typename WrapperT >
		struct ConfigT
		{
			WrapperT< VkPipelineShaderStageCreateInfoArray > program;
		};

		using Config = ConfigT< std::optional >;
		using ConfigData = ConfigT< RawTypeT >;
	}

	class PipelinePass
		: public RunnablePass
	{
	public:
		PipelinePass( FramePass const & pass
			, GraphContext const & context
			, RunnableGraph & graph
			, pp::Config config
			, VkPipelineBindPoint bindingPoint );
		virtual ~PipelinePass();

	protected:
		void doInitialise()override;

	private:
		void doFillDescriptorBindings();
		void doCreateDescriptorSetLayout();
		void doCreatePipelineLayout();
		void doCreateDescriptorPool();
		void doCreateDescriptorSet();

	protected:
		pp::ConfigData m_baseConfig;
		VkPipelineBindPoint m_bindingPoint;
		WriteDescriptorSetArray m_descriptorWrites;
		VkDescriptorSetLayoutBindingArray m_descriptorBindings;
		VkDescriptorSetLayout m_descriptorSetLayout{ VK_NULL_HANDLE };
		VkPipelineLayout m_pipelineLayout{ VK_NULL_HANDLE };
		VkPipeline m_pipeline{ VK_NULL_HANDLE };
		VkDescriptorPool m_descriptorSetPool{ VK_NULL_HANDLE };
		VkDescriptorSet m_descriptorSet{ VK_NULL_HANDLE };
	};

	template< typename BuilderT >
	class PipelinePassBuilderT
	{
	public:
		PipelinePassBuilderT() = default;
		/**
		*\param[in] config
		*	The pipeline program.
		*/
		BuilderT & program( VkPipelineShaderStageCreateInfoArray config )
		{
			m_config.program = std::move( config );
			return static_cast< BuilderT & >( *this );
		}

	protected:
		pp::Config m_config;
	};

	class PipelinePassBuilder
		: public PipelinePassBuilderT< PipelinePassBuilder >
	{
	};
}
