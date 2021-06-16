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

	namespace pp
	{
		template< template< typename ValueT > typename WrapperT >
		struct ConfigT
		{
			WrapperT< std::vector< VkPipelineShaderStageCreateInfoArray > > programs;
		};

		using Config = ConfigT< std::optional >;
		using ConfigData = ConfigT< RawTypeT >;
	}

	class PipelineHolder
	{
	public:
		CRG_API PipelineHolder( FramePass const & pass
			, GraphContext const & context
			, RunnableGraph & graph
			, pp::Config config
			, VkPipelineBindPoint bindingPoint );
		CRG_API virtual ~PipelineHolder();

	protected:
		CRG_API VkPipelineShaderStageCreateInfoArray const & doGetProgram( uint32_t index )const;
		CRG_API VkPipeline & doGetPipeline( uint32_t index );
		CRG_API void doPreInitialise( uint32_t index );
		CRG_API void doPreRecordInto( VkCommandBuffer commandBuffer
			, uint32_t index );
		CRG_API void doResetPipeline( VkPipelineShaderStageCreateInfoArray config
			, uint32_t index );
		CRG_API virtual void doCreatePipeline( uint32_t index ) = 0;

	private:
		void doFillDescriptorBindings();
		void doCreateDescriptorSetLayout();
		void doCreatePipelineLayout();
		void doCreateDescriptorPool();
		void doCreateDescriptorSet( uint32_t index );

	protected:
		FramePass const & m_phPass;
		GraphContext const & m_phContext;
		RunnableGraph & m_phGraph;

	protected:
		pp::ConfigData m_baseConfig;
		VkPipelineBindPoint m_bindingPoint;
		VkDescriptorSetLayoutBindingArray m_descriptorBindings;
		VkDescriptorSetLayout m_descriptorSetLayout{ VK_NULL_HANDLE };
		VkPipelineLayout m_pipelineLayout{ VK_NULL_HANDLE };
		VkDescriptorPool m_descriptorSetPool{ VK_NULL_HANDLE };
		struct DescriptorSet
		{
			WriteDescriptorSetArray writes;
			VkDescriptorSet set;
		};
		std::vector< DescriptorSet > m_descriptorSets;

	private:
		std::vector< VkPipeline > m_pipelines{};
	};

	template< typename BuilderT >
	class PipelinePassBuilderT
	{
	public:
		CRG_API PipelinePassBuilderT() = default;
		/**
		*\param[in] config
		*	The pipeline program.
		*/
		BuilderT & program( VkPipelineShaderStageCreateInfoArray config )
		{
			m_baseConfig.programs = { std::move( config ) };
			return static_cast< BuilderT & >( *this );
		}
		/**
		*\param[in] config
		*	The pipeline programs.
		*/
		BuilderT & programs( std::vector< VkPipelineShaderStageCreateInfoArray > config )
		{
			m_baseConfig.programs = std::move( config );
			return static_cast< BuilderT & >( *this );
		}

	protected:
		pp::Config m_baseConfig;
	};

	class PipelinePassBuilder
		: public PipelinePassBuilderT< PipelinePassBuilder >
	{
	};
}
