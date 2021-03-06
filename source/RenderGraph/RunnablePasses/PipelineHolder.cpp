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
		, GraphContext & context
		, RunnableGraph & graph
		, pp::Config config
		, VkPipelineBindPoint bindingPoint
		, uint32_t maxPassCount )
		: m_pass{ pass }
		, m_context{ context }
		, m_graph{ graph }
		, m_baseConfig{ std::move( config.programs ? *config.programs : defaultV< std::vector< VkPipelineShaderStageCreateInfoArray > > )
			, std::move( config.layouts ? *config.layouts : defaultV< std::vector< VkDescriptorSetLayout > > ) }
		, m_bindingPoint{ bindingPoint }
	{
		m_pipelines.resize( m_baseConfig.programs.size() );
		m_descriptorSets.resize( maxPassCount );
	}

	PipelineHolder::~PipelineHolder()
	{
		m_descriptorBindings.clear();

		if ( m_descriptorSetPool )
		{
			crgUnregisterObject( m_context, m_descriptorSetPool );
			m_context.vkDestroyDescriptorPool( m_context.device
				, m_descriptorSetPool
				, m_context.allocator );
		}

		for ( auto & pipeline : m_pipelines )
		{
			crgUnregisterObject( m_context, pipeline );
			m_context.vkDestroyPipeline( m_context.device
				, pipeline
				, m_context.allocator );
		}

		if ( m_pipelineLayout )
		{
			crgUnregisterObject( m_context, m_pipelineLayout );
			m_context.vkDestroyPipelineLayout( m_context.device
				, m_pipelineLayout
				, m_context.allocator );
		}

		if ( m_descriptorSetLayout )
		{
			crgUnregisterObject( m_context, m_descriptorSetLayout );
			m_context.vkDestroyDescriptorSetLayout( m_context.device
				, m_descriptorSetLayout
				, m_context.allocator );
		}
	}

	void PipelineHolder::initialise()
	{
		doFillDescriptorBindings();
		doCreateDescriptorSetLayout();
		doCreatePipelineLayout();
		doCreateDescriptorPool();
	}

	VkPipelineShaderStageCreateInfoArray const & PipelineHolder::getProgram( uint32_t index )const
	{
		if ( m_baseConfig.programs.size() == 1u )
		{
			return m_baseConfig.programs[0];
		}

		assert( m_baseConfig.programs.size() > index );
		return m_baseConfig.programs[index];
	}

	VkPipeline & PipelineHolder::getPipeline( uint32_t index )
	{
		if ( m_baseConfig.programs.size() == 1u )
		{
			assert( m_pipelines.size() == 1u );
			return m_pipelines[0];
		}

		assert( m_pipelines.size() > index );
		return m_pipelines[index];
	}

	void PipelineHolder::recordInto( VkCommandBuffer commandBuffer
		, uint32_t index )
	{
		createDescriptorSet( index );
		auto & pipeline = getPipeline( index );
		m_context.vkCmdBindPipeline( commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline );
		m_context.vkCmdBindDescriptorSets( commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0u, 1u, &m_descriptorSets[index].set, 0u, nullptr );
	}

	void PipelineHolder::resetPipeline( VkPipelineShaderStageCreateInfoArray config )
	{
		for ( uint32_t index = 0; index < m_pipelines.size(); ++index )
		{
			if ( m_pipelines[index] )
			{
				crgUnregisterObject( m_context, m_pipelines[index] );
				m_context.vkDestroyPipeline( m_context.device
					, m_pipelines[index]
					, m_context.allocator );
			}

			assert( m_baseConfig.programs.size() > index );
			m_baseConfig.programs[index] = std::move( config );
		}
	}

	void PipelineHolder::createDescriptorSet( uint32_t index )
	{
		auto & descriptorSet = m_descriptorSets[index];

		if ( descriptorSet.set != VK_NULL_HANDLE )
		{
			return;
		}

		for ( auto & attach : m_pass.images )
		{
			if ( attach.count <= 1u )
			{
				if ( attach.isSampledView() )
				{
					descriptorSet.writes.push_back( WriteDescriptorSet{ attach.binding
						, 0u
						, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
						, VkDescriptorImageInfo{ m_graph.createSampler( attach.image.samplerDesc )
							, m_graph.createImageView( attach.view( index ) )
							, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL } } );
				}
				else if ( attach.isStorageView() )
				{
					descriptorSet.writes.push_back( WriteDescriptorSet{ attach.binding
						, 0u
						, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
						, VkDescriptorImageInfo{ VK_NULL_HANDLE
							, m_graph.createImageView( attach.view( index ) )
							, VK_IMAGE_LAYOUT_GENERAL } } );
				}
			}
			else
			{
				if ( attach.isSampledView() )
				{
					VkDescriptorImageInfoArray imageInfos;

					for ( uint32_t i = 0u; i < attach.count; ++i )
					{
						imageInfos.push_back( { m_graph.createSampler( attach.image.samplerDesc )
							, m_graph.createImageView( attach.view( i ) )
							, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL } );
					}

					descriptorSet.writes.push_back( WriteDescriptorSet{ attach.binding
						, 0u
						, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
						, imageInfos } );
				}
				else if ( attach.isStorageView() )
				{
					VkDescriptorImageInfoArray imageInfos;

					for ( uint32_t i = 0u; i < attach.count; ++i )
					{
						imageInfos.push_back( { VK_NULL_HANDLE
							, m_graph.createImageView( attach.view( i ) )
							, VK_IMAGE_LAYOUT_GENERAL } );
					}

					descriptorSet.writes.push_back( WriteDescriptorSet{ attach.binding
						, 0u
						, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
						, imageInfos } );
				}
			}
		}

		for ( auto & uniform : m_pass.buffers )
		{
			descriptorSet.writes.push_back( uniform.getBufferWrite() );
		}

		VkDescriptorSetAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO
			, nullptr
			, m_descriptorSetPool
			, 1u
			, &m_descriptorSetLayout };
		auto res = m_context.vkAllocateDescriptorSets( m_context.device
			, &allocateInfo
			, &descriptorSet.set );
		checkVkResult( res, m_pass.name + " - DescriptorSet allocation" );
		crgRegisterObject( m_context, m_pass.name, descriptorSet.set );

		for ( auto & write : descriptorSet.writes )
		{
			write.update( descriptorSet.set );
		}

		auto descriptorWrites = makeVkArray< VkWriteDescriptorSet >( descriptorSet.writes );
		m_context.vkUpdateDescriptorSets( m_context.device
			, uint32_t( descriptorWrites.size() )
			, descriptorWrites.data()
			, 0u
			, nullptr );
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

		for ( auto & attach : m_pass.images )
		{
			if ( attach.isSampledView() )
			{
				m_descriptorBindings.push_back( { attach.binding
					, imageDescriptor
					, std::max( 1u, attach.count )
					, shaderStage
					, nullptr } );
			}
			else if ( attach.isStorageView() )
			{
				m_descriptorBindings.push_back( { attach.binding
					, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
					, std::max( 1u, attach.count )
					, shaderStage
					, nullptr } );
			}
		}

		for ( auto & uniform : m_pass.buffers )
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
		auto res = m_context.vkCreateDescriptorSetLayout( m_context.device
			, &createInfo
			, m_context.allocator
			, &m_descriptorSetLayout );
		checkVkResult( res, m_pass.name + " - DescriptorSetLayout creation" );
		crgRegisterObject( m_context, m_pass.name, m_descriptorSetLayout );
	}

	void PipelineHolder::doCreatePipelineLayout()
	{
		std::vector< VkDescriptorSetLayout > layouts;
		layouts.push_back( m_descriptorSetLayout );
		layouts.insert( layouts.end()
			, m_baseConfig.layouts.begin()
			, m_baseConfig.layouts.end() );
		VkPipelineLayoutCreateInfo createInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO
			, nullptr
			, 0u
			, uint32_t( layouts.size() )
			, layouts.data()
			, 0u                       // pushConstantRangeCount
			, nullptr };               // pPushConstantRanges
		auto res = m_context.vkCreatePipelineLayout( m_context.device
			, &createInfo
			, m_context.allocator
			, &m_pipelineLayout );
		checkVkResult( res, m_pass.name + " - PipeliineLayout creation" );
		crgRegisterObject( m_context, m_pass.name, m_pipelineLayout );
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
		auto res = m_context.vkCreateDescriptorPool( m_context.device
			, &createInfo
			, m_context.allocator
			, &m_descriptorSetPool );
		checkVkResult( res, m_pass.name + " - DescriptorPool creation" );
		crgRegisterObject( m_context, m_pass.name, m_descriptorSetPool );
	}
}
