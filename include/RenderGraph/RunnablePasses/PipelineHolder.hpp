/*
See LICENSE file in root folder.
*/
#pragma once

#include "RenderGraph/RunnablePasses/PipelineConfig.hpp"

namespace crg
{
	class PipelineHolder
	{
	public:
		PipelineHolder( PipelineHolder const & )noexcept = delete;
		PipelineHolder & operator=( PipelineHolder const & )noexcept = delete;
		PipelineHolder( PipelineHolder && )noexcept = delete;
		PipelineHolder & operator=( PipelineHolder && )noexcept = delete;
		CRG_API PipelineHolder( FramePass const & pass
			, GraphContext & context
			, RunnableGraph & graph
			, pp::Config config
			, VkPipelineBindPoint bindingPoint
			, uint32_t maxPassCount );
		CRG_API virtual ~PipelineHolder()noexcept;

		CRG_API void initialise();
		CRG_API void cleanup()noexcept;
		CRG_API VkPipelineShaderStageCreateInfoArray const & getProgram( uint32_t index );
		CRG_API VkPipeline & getPipeline( uint32_t index );
		CRG_API void createPipeline( uint32_t index
			, std::string const & name
			, VkGraphicsPipelineCreateInfo const & createInfo );
		CRG_API void createPipeline( uint32_t index
			, VkGraphicsPipelineCreateInfo const & createInfo );
		CRG_API void createPipeline( uint32_t index
			, std::string const & name
			, VkComputePipelineCreateInfo const & createInfo );
		CRG_API void createPipeline( uint32_t index
			, VkComputePipelineCreateInfo const & createInfo );
		CRG_API void recordInto( RecordContext const & context
			, VkCommandBuffer commandBuffer
			, uint32_t index );
		CRG_API void resetPipeline( VkPipelineShaderStageCreateInfoArray config
			, uint32_t index );
		CRG_API void createDescriptorSet( uint32_t index );

		VkDescriptorSet getDescriptorSet( uint32_t index )
		{
			createDescriptorSet( index );
			return m_descriptorSets[index].set;
		}

		VkPipelineLayout getPipelineLayout()const
		{
			return m_pipelineLayout;
		}

		FramePass const & getPass()const
		{
			return m_pass;
		}

		GraphContext & getContext()const
		{
			return m_context;
		}

	private:
		void doFillDescriptorBindings();
		void doCreateDescriptorSetLayout();
		void doCreatePipelineLayout();
		void doCreateDescriptorPool();

	protected:
		FramePass const & m_pass;
		GraphContext & m_context;
		RunnableGraph & m_graph;

	protected:
		pp::ConfigData m_baseConfig;
		VkPipelineBindPoint m_bindingPoint;
		VkDescriptorSetLayoutBindingArray m_descriptorBindings;
		VkDescriptorSetLayout m_descriptorSetLayout{};
		VkPipelineLayout m_pipelineLayout{};
		VkDescriptorPool m_descriptorSetPool{};
		struct DescriptorSet
		{
			WriteDescriptorSetArray writes{};
			VkDescriptorSet set{};
		};
		std::vector< DescriptorSet > m_descriptorSets;

	private:
		std::vector< VkPipeline > m_pipelines{};
	};

	template< typename BuilderT >
	class PipelinePassBuilderT
	{
	public:
		/**
		*\param[in] config
		*	The pipeline program.
		*/
		BuilderT & program( VkPipelineShaderStageCreateInfoArray config )
		{
			m_baseConfig.programs( { std::move( config ) } );
			return static_cast< BuilderT & >( *this );
		}
		/**
		*\param[in] config
		*	The pipeline programs.
		*/
		BuilderT & programs( std::vector< VkPipelineShaderStageCreateInfoArray > config )
		{
			m_baseConfig.programs( std::move( config ) );
			return static_cast< BuilderT & >( *this );
		}
		/**
		*\param[in] config
		*	The pipeline program creator.
		*/
		BuilderT & programCreator( ProgramCreator config )
		{
			m_baseConfig.programCreator( std::move( config ) );
			return static_cast< BuilderT & >( *this );
		}
		/**
		*\param[in] config
		*	The push constants range for the pipeline.
		*/
		auto & pushConstants( VkPushConstantRange config )
		{
			m_baseConfig.pushConstants( std::move( config ) );
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
