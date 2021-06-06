/*
See LICENSE file in root folder.
*/
#include "RenderGraph/RunnablePasses/PipelinePass.hpp"

#include "RenderGraph/GraphContext.hpp"
#include "RenderGraph/RunnableGraph.hpp"

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

		template< typename VkType, typename LibType >
		inline std::vector< VkType > makeVkArray( std::vector< LibType > const & input )
		{
			std::vector< VkType > result;
			result.reserve( input.size() );

			for ( auto const & element : input )
			{
				result.emplace_back( element );
			}

			return result;
		}
	}

	PipelinePass::PipelinePass( FramePass const & pass
		, GraphContext const & context
		, RunnableGraph & graph
		, pp::Config config
		, VkPipelineBindPoint bindingPoint )
		: RunnablePass{ pass, context, graph }
		, m_baseConfig{ std::move( config.program ? *config.program : defaultV< VkPipelineShaderStageCreateInfoArray > ) }
		, m_bindingPoint{ bindingPoint }
	{
		m_descriptorSets.resize( m_commandBuffers.size() );
	}

	PipelinePass::~PipelinePass()
	{
		m_descriptorBindings.clear();

		if ( m_descriptorSetPool )
		{
			crgUnregisterObject( m_context, m_descriptorSetPool );
			m_context.vkDestroyDescriptorPool( m_context.device
				, m_descriptorSetPool
				, m_context.allocator );
		}

		if ( m_pipeline )
		{
			crgUnregisterObject( m_context, m_pipeline );
			m_context.vkDestroyPipeline( m_context.device
				, m_pipeline
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

	void PipelinePass::doInitialise()
	{
		doFillDescriptorBindings();
		doCreateDescriptorSetLayout();
		doCreatePipelineLayout();
		doCreateDescriptorPool();
	}

	void PipelinePass::doRecordInto( VkCommandBuffer commandBuffer
		, uint32_t index )
	{
		doCreateDescriptorSet( index );
		RunnablePass::doRecordInto( commandBuffer, index );
	}

	void PipelinePass::doFillDescriptorBindings()
	{
		VkShaderStageFlags shaderStage = ( m_bindingPoint == VK_PIPELINE_BIND_POINT_GRAPHICS
			? VK_SHADER_STAGE_FRAGMENT_BIT
			: VK_SHADER_STAGE_COMPUTE_BIT );

		for ( auto & attach : m_pass.images )
		{
			if ( attach.isSampled() )
			{
				m_descriptorBindings.push_back( { attach.binding
					, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
					, 1u
					, shaderStage
					, nullptr } );
			}
			else if ( attach.isStorage() )
			{
				m_descriptorBindings.push_back( { attach.binding
					, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
					, 1u
					, shaderStage
					, nullptr } );
			}
		}

		shaderStage = ( m_bindingPoint == VK_PIPELINE_BIND_POINT_GRAPHICS
			? VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
			: VK_SHADER_STAGE_COMPUTE_BIT );

		for ( auto & uniform : m_pass.buffers )
		{
			m_descriptorBindings.push_back( { uniform->dstBinding
				, uniform->descriptorType
				, uniform->descriptorCount
				, shaderStage
				, nullptr } );
		}

		for ( auto & uniform : m_pass.bufferViews )
		{
			m_descriptorBindings.push_back( { uniform->dstBinding
				, uniform->descriptorType
				, uniform->descriptorCount
				, shaderStage
				, nullptr } );
		}
	}

	void PipelinePass::doCreateDescriptorSetLayout()
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

	void PipelinePass::doCreatePipelineLayout()
	{
		VkPipelineLayoutCreateInfo createInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO
			, nullptr
			, 0u
			, 1u                       // setLayoutCount
			, &m_descriptorSetLayout   // pSetLayouts
			, 0u                       // pushConstantRangeCount
			, nullptr };               // pPushConstantRanges
		auto res = m_context.vkCreatePipelineLayout( m_context.device
			, &createInfo
			, m_context.allocator
			, &m_pipelineLayout );
		checkVkResult( res, m_pass.name + " - PipeliineLayout creation" );
		crgRegisterObject( m_context, m_pass.name, m_pipelineLayout );
	}

	void PipelinePass::doCreateDescriptorPool()
	{
		assert( m_descriptorSetLayout );
		auto sizes = convert( m_descriptorBindings, m_commandBuffers.size() );
		VkDescriptorPoolCreateInfo createInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO
			, nullptr
			, 0u
			, 1u
			, uint32_t( sizes.size() )
			, sizes.data() };
		auto res = m_context.vkCreateDescriptorPool( m_context.device
			, &createInfo
			, m_context.allocator
			, &m_descriptorSetPool );
		checkVkResult( res, m_pass.name + " - DescriptorPool creation" );
		crgRegisterObject( m_context, m_pass.name, m_descriptorSetPool );
	}

	void PipelinePass::doCreateDescriptorSet( uint32_t index )
	{
		auto & descriptorSet = m_descriptorSets[index];

		if ( m_bindingPoint == VK_PIPELINE_BIND_POINT_GRAPHICS )
		{
			for ( auto & attach : m_pass.images )
			{
				if ( attach.isSampled() )
				{
					descriptorSet.writes.push_back( WriteDescriptorSet{ attach.binding
						, 0u
						, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
						, VkDescriptorImageInfo{ m_graph.createSampler( attach.samplerDesc )
							, m_graph.getImageView( attach.view( index ) )
							, attach.initialLayout } } );
				}
				else if ( attach.isStorage() )
				{
					descriptorSet.writes.push_back( WriteDescriptorSet{ attach.binding
						, 0u
						, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
						, VkDescriptorImageInfo{ VK_NULL_HANDLE
							, m_graph.getImageView( attach.view( index ) )
							, attach.initialLayout } } );
				}
			}
		}

		for ( auto & uniform : m_pass.buffers )
		{
			descriptorSet.writes.push_back( uniform );
		}

		for ( auto & uniform : m_pass.bufferViews )
		{
			descriptorSet.writes.push_back( uniform );
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
}
