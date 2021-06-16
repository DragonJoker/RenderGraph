/*
See LICENSE file in root folder.
*/
#include "RenderGraph/RunnablePasses/PipelineHolder.hpp"

#include "RenderGraph/GraphContext.hpp"
#include "RenderGraph/RunnableGraph.hpp"
#include "RenderGraph/RunnablePasses/RenderPass.hpp"

#include <cassert>

namespace crg
{
	namespace
	{
		VkDescriptorPoolSizeArray convert( VkDescriptorSetLayoutBindingArray const & bindings
			, uint32_t maxSets )
		{
			VkDescriptorPoolSizeArray result;

			for ( auto & binding : bindings )
			{
				result.push_back( { binding.descriptorType, binding.descriptorCount * maxSets } );
			}

			return result;
		}
	}

	PipelineHolder::PipelineHolder( FramePass const & pass
		, GraphContext const & context
		, RunnableGraph & graph
		, pp::Config config
		, VkPipelineBindPoint bindingPoint )
		: m_phPass{ pass }
		, m_phContext{ context }
		, m_phGraph{ graph }
		, m_baseConfig{ std::move( config.programs ? *config.programs : defaultV< std::vector< VkPipelineShaderStageCreateInfoArray > > ) }
		, m_bindingPoint{ bindingPoint }
	{
		m_pipelines.resize( m_baseConfig.programs.size() );
	}

	PipelineHolder::~PipelineHolder()
	{
		m_descriptorBindings.clear();

		if ( m_descriptorSetPool )
		{
			crgUnregisterObject( m_phContext, m_descriptorSetPool );
			m_phContext.vkDestroyDescriptorPool( m_phContext.device
				, m_descriptorSetPool
				, m_phContext.allocator );
		}

		for ( auto & pipeline : m_pipelines )
		{
			crgUnregisterObject( m_phContext, pipeline );
			m_phContext.vkDestroyPipeline( m_phContext.device
				, pipeline
				, m_phContext.allocator );
		}

		if ( m_pipelineLayout )
		{
			crgUnregisterObject( m_phContext, m_pipelineLayout );
			m_phContext.vkDestroyPipelineLayout( m_phContext.device
				, m_pipelineLayout
				, m_phContext.allocator );
		}

		if ( m_descriptorSetLayout )
		{
			crgUnregisterObject( m_phContext, m_descriptorSetLayout );
			m_phContext.vkDestroyDescriptorSetLayout( m_phContext.device
				, m_descriptorSetLayout
				, m_phContext.allocator );
		}
	}

	VkPipelineShaderStageCreateInfoArray const & PipelineHolder::doGetProgram( uint32_t index )const
	{
		if ( m_baseConfig.programs.size() == 1u )
		{
			return m_baseConfig.programs[0];
		}

		assert( m_baseConfig.programs.size() > index );
		return m_baseConfig.programs[index];
	}

	VkPipeline & PipelineHolder::doGetPipeline( uint32_t index )
	{
		if ( m_baseConfig.programs.size() == 1u )
		{
			assert( m_pipelines.size() == 1u );
			return m_pipelines[0];
		}

		assert( m_pipelines.size() > index );
		return m_pipelines[index];
	}

	void PipelineHolder::doPreInitialise( uint32_t index )
	{
		if ( index == 0u )
		{
			doFillDescriptorBindings();
			doCreateDescriptorSetLayout();
			doCreatePipelineLayout();
			doCreateDescriptorPool();
		}

		doCreatePipeline( index );
	}

	void PipelineHolder::doPreRecordInto( VkCommandBuffer commandBuffer
		, uint32_t index )
	{
		doCreateDescriptorSet( index );
		auto & pipeline = doGetPipeline( index );
		m_phContext.vkCmdBindPipeline( commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline );
		m_phContext.vkCmdBindDescriptorSets( commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0u, 1u, &m_descriptorSets[index].set, 0u, nullptr );
	}

	void PipelineHolder::doResetPipeline( VkPipelineShaderStageCreateInfoArray config
		, uint32_t index )
	{
		assert( m_pipelines.size() > index );

		if ( m_pipelines[index] )
		{
			crgUnregisterObject( m_phContext, m_pipelines[index] );
			m_phContext.vkDestroyPipeline( m_phContext.device
				, m_pipelines[index]
				, m_phContext.allocator );
		}

		assert( m_baseConfig.programs.size() > index );
		m_baseConfig.programs[index] = std::move( config );
		doCreatePipeline( index );
	}

	void PipelineHolder::doFillDescriptorBindings()
	{
		VkShaderStageFlags shaderStage = ( VK_PIPELINE_BIND_POINT_COMPUTE == m_bindingPoint )
			? VK_SHADER_STAGE_COMPUTE_BIT
			: ( VK_SHADER_STAGE_VERTEX_BIT
				| VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT
				| VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT
				| VK_SHADER_STAGE_GEOMETRY_BIT
				| VK_SHADER_STAGE_FRAGMENT_BIT );
		auto imageDescriptor = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

		for ( auto & attach : m_phPass.images )
		{
			if ( attach.isSampledView() )
			{
				m_descriptorBindings.push_back( { attach.binding
					, imageDescriptor
					, 1u
					, shaderStage
					, nullptr } );
			}
			else if ( attach.isStorageView() )
			{
				m_descriptorBindings.push_back( { attach.binding
					, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
					, 1u
					, shaderStage
					, nullptr } );
			}
		}

		for ( auto & uniform : m_phPass.buffers )
		{
			m_descriptorBindings.push_back( { uniform.binding
				, uniform.getDescriptorType()
				, 1u
				, shaderStage
				, nullptr } );
		}
	}

	void PipelineHolder::doCreateDescriptorSetLayout()
	{
		VkDescriptorSetLayoutCreateInfo createInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO
			, nullptr
			, 0u
			, static_cast< uint32_t >( m_descriptorBindings.size() )
			, m_descriptorBindings.data() };
		auto res = m_phContext.vkCreateDescriptorSetLayout( m_phContext.device
			, &createInfo
			, m_phContext.allocator
			, &m_descriptorSetLayout );
		checkVkResult( res, m_phPass.name + " - DescriptorSetLayout creation" );
		crgRegisterObject( m_phContext, m_phPass.name, m_descriptorSetLayout );
	}

	void PipelineHolder::doCreatePipelineLayout()
	{
		VkPipelineLayoutCreateInfo createInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO
			, nullptr
			, 0u
			, 1u                       // setLayoutCount
			, &m_descriptorSetLayout   // pSetLayouts
			, 0u                       // pushConstantRangeCount
			, nullptr };               // pPushConstantRanges
		auto res = m_phContext.vkCreatePipelineLayout( m_phContext.device
			, &createInfo
			, m_phContext.allocator
			, &m_pipelineLayout );
		checkVkResult( res, m_phPass.name + " - PipeliineLayout creation" );
		crgRegisterObject( m_phContext, m_phPass.name, m_pipelineLayout );
	}

	void PipelineHolder::doCreateDescriptorPool()
	{
		assert( m_descriptorSetLayout );
		auto sizes = getBindingsSizes( m_descriptorBindings, m_descriptorSets.size() );
		VkDescriptorPoolCreateInfo createInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO
			, nullptr
			, 0u
			, uint32_t( m_descriptorSets.size() )
			, uint32_t( sizes.size() )
			, sizes.data() };
		auto res = m_phContext.vkCreateDescriptorPool( m_phContext.device
			, &createInfo
			, m_phContext.allocator
			, &m_descriptorSetPool );
		checkVkResult( res, m_phPass.name + " - DescriptorPool creation" );
		crgRegisterObject( m_phContext, m_phPass.name, m_descriptorSetPool );
	}

	void PipelineHolder::doCreateDescriptorSet( uint32_t index )
	{
		auto & descriptorSet = m_descriptorSets[index];

		if ( descriptorSet.set != VK_NULL_HANDLE )
		{
			return;
		}

		for ( auto & attach : m_phPass.images )
		{
			if ( attach.isSampledView() )
			{
				descriptorSet.writes.push_back( WriteDescriptorSet{ attach.binding
					, 0u
					, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
					, VkDescriptorImageInfo{ m_phGraph.createSampler( attach.image.samplerDesc )
					, m_phGraph.createImageView( attach.view( index ) )
					, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL } } );
			}
			else if ( attach.isStorageView() )
			{
				descriptorSet.writes.push_back( WriteDescriptorSet{ attach.binding
					, 0u
					, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
					, VkDescriptorImageInfo{ VK_NULL_HANDLE
					, m_phGraph.createImageView( attach.view( index ) )
					, VK_IMAGE_LAYOUT_GENERAL } } );
			}
		}

		for ( auto & uniform : m_phPass.buffers )
		{
			descriptorSet.writes.push_back( uniform.getBufferWrite() );
		}

		VkDescriptorSetAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO
			, nullptr
			, m_descriptorSetPool
			, 1u
			, &m_descriptorSetLayout };
		auto res = m_phContext.vkAllocateDescriptorSets( m_phContext.device
			, &allocateInfo
			, &descriptorSet.set );
		checkVkResult( res, m_phPass.name + " - DescriptorSet allocation" );
		crgRegisterObject( m_phContext, m_phPass.name, descriptorSet.set );

		for ( auto & write : descriptorSet.writes )
		{
			write.update( descriptorSet.set );
		}

		auto descriptorWrites = makeVkArray< VkWriteDescriptorSet >( descriptorSet.writes );
		m_phContext.vkUpdateDescriptorSets( m_phContext.device
			, uint32_t( descriptorWrites.size() )
			, descriptorWrites.data()
			, 0u
			, nullptr );
	}
}
