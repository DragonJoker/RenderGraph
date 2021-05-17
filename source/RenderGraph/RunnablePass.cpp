/*
This file belongs to RenderGraph.
See LICENSE file in root folder.
*/
#include "RenderGraph/RunnablePass.hpp"

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

	RunnablePass::RunnablePass( RenderPass const & pass
		, GraphContext const & context
		, RunnableGraph const & graph
		, rp::Config config
		, VkPipelineBindPoint bindingPoint )
		: m_baseConfig{ std::move( config.program ? *config.program : defaultV< VkPipelineShaderStageCreateInfoArray > )
			, std::move( config.pushRanges ? *config.pushRanges : defaultV< VkPushConstantRangeArray > )
			, std::move( config.dsState ? *config.dsState : defaultV< VkPipelineDepthStencilStateCreateInfo > )
			, std::move( config.descriptorWrites ? *config.descriptorWrites : defaultV< WriteDescriptorSetArray > ) }
		, m_pass{ pass }
		, m_context{ context }
		, m_graph{ graph }
		, m_bindingPoint{ bindingPoint }
		, m_descriptorWrites{ m_baseConfig.descriptorWrites }
	{
	}

	RunnablePass::~RunnablePass()
	{
		m_descriptorWrites.clear();
		m_descriptorBindings.clear();

		if ( m_semaphore )
		{
			crgUnregisterObject( m_context, m_semaphore );
			m_context.vkDestroySemaphore( m_context.device
				, m_semaphore
				, m_context.allocator );
		}

		if ( m_descriptorSet )
		{
			crgUnregisterObject( m_context, m_descriptorSet );
			m_context.vkFreeDescriptorSets( m_context.device
				, m_descriptorSetPool
				, 1u
				, &m_descriptorSet );
		}

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

		if ( m_sampler )
		{
			crgUnregisterObject( m_context, m_sampler );
			m_context.vkDestroySampler( m_context.device
				, m_sampler
				, m_context.allocator );
		}
	}

	void RunnablePass::initialise()
	{
		doCreateSampler();
		doFillDescriptorBindings();
		doCreateDescriptorSetLayout();
		doCreatePipelineLayout();
		doCreateDescriptorPool();
		doCreateDescriptorSet();
		doInitialise();
		doCreateCommandPool();
		doCreateCommandBuffer();
		doCreateSemaphore();
	}

	void RunnablePass::record()const
	{
		VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
			, nullptr
			, 0u
			, nullptr };
		m_context.vkBeginCommandBuffer( m_commandBuffer, &beginInfo );
		recordInto( m_commandBuffer );
		m_context.vkEndCommandBuffer( m_commandBuffer );
	}

	void RunnablePass::recordInto( VkCommandBuffer commandBuffer )const
	{
		m_context.vkCmdBindPipeline( commandBuffer, m_bindingPoint, m_pipeline );
		m_context.vkCmdBindDescriptorSets( commandBuffer, m_bindingPoint, m_pipelineLayout, 0u, 1u, &m_descriptorSet, 0u, nullptr );
		doRecordInto( commandBuffer );
	}

	SemaphoreWait RunnablePass::run( SemaphoreWait toWait
		, VkQueue queue )const
	{
		VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO
			, nullptr
			, 1u
			, &toWait.semaphore
			, & toWait.dstStageMask
			, 1u
			, &m_commandBuffer
			, 1u
			, &m_semaphore };
		m_context.vkQueueSubmit( queue
			, 1u
			, &submitInfo
			, VK_NULL_HANDLE );
		return { m_semaphore
			, VkPipelineStageFlags( m_bindingPoint == VK_PIPELINE_BIND_POINT_GRAPHICS
				? VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
				: VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT ) };
	}

	void RunnablePass::doCreateSampler()
	{
		if ( m_bindingPoint == VK_PIPELINE_BIND_POINT_GRAPHICS )
		{
			VkSamplerCreateInfo createInfo{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO
				, nullptr
				, 0u
				, VK_FILTER_LINEAR
				, VK_FILTER_LINEAR
				, VK_SAMPLER_MIPMAP_MODE_LINEAR
				, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
				, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
				, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
				, 0.0f // mipLodBias
				, VK_FALSE // anisotropyEnable
				, 0.0f // maxAnisotropy
				, VK_FALSE // compareEnable
				, VK_COMPARE_OP_ALWAYS // compareOp
				, -1000.0f
				, 1000.0f
				, VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK
				, VK_FALSE };
			auto res = m_context.vkCreateSampler( m_context.device
				, &createInfo
				, m_context.allocator
				, &m_sampler );
			checkVkResult( res, "Sampler creation" );
			crgRegisterObject( m_context, m_pass.name, m_sampler );
		}
	}

	void RunnablePass::doFillDescriptorBindings()
	{
		auto descriptorWrites = m_descriptorWrites;
		m_descriptorWrites.clear();
		std::sort( descriptorWrites.begin()
			, descriptorWrites.end()
			, []( WriteDescriptorSet const & lhs, WriteDescriptorSet const & rhs )
			{
				return lhs->dstBinding < rhs->dstBinding;
			} );
		uint32_t index = 0u;
		auto itSampled = m_pass.sampled.begin();
		VkShaderStageFlags shaderStage = ( m_bindingPoint == VK_PIPELINE_BIND_POINT_GRAPHICS
			? VK_SHADER_STAGE_FRAGMENT_BIT
			: VK_SHADER_STAGE_COMPUTE_BIT );
		auto imageDescriptor = ( m_bindingPoint == VK_PIPELINE_BIND_POINT_GRAPHICS
			? VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
			: VK_DESCRIPTOR_TYPE_STORAGE_IMAGE );
		auto imageSampler = ( m_bindingPoint == VK_PIPELINE_BIND_POINT_GRAPHICS
			? m_sampler
			: VK_NULL_HANDLE );

		for ( auto & write : descriptorWrites )
		{
			while ( write->dstBinding != index
				&& itSampled != m_pass.sampled.end() )
			{
				m_descriptorBindings.push_back( { index
					, imageDescriptor
					, 1u
					, shaderStage
					, nullptr } );
				m_descriptorWrites.push_back( WriteDescriptorSet{ index
					, 0u
					, imageDescriptor
					, VkDescriptorImageInfo{ imageSampler
					, m_graph.getImageView( *itSampled )
					, itSampled->initialLayout } } );
				++itSampled;
				++index;
			}

			if ( write->dstBinding == index
				|| itSampled == m_pass.sampled.end() )
			{
				m_descriptorBindings.push_back( { write->dstBinding
					, write->descriptorType
					, 1u
					, shaderStage
					, nullptr } );
				m_descriptorWrites.push_back( write );
				index = write->dstBinding + 1u;
			}
		}

		while ( itSampled != m_pass.sampled.end() )
		{
			m_descriptorBindings.push_back( { index
				, imageDescriptor
				, 1u
				, shaderStage
				, nullptr } );
			m_descriptorWrites.push_back( WriteDescriptorSet{ index
				, 0u
				, imageDescriptor
				, VkDescriptorImageInfo{ imageSampler
					, m_graph.getImageView( *itSampled )
					, itSampled->initialLayout } } );
			++index;
			++itSampled;
		}
	}

	void RunnablePass::doCreateDescriptorSetLayout()
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
		checkVkResult( res, "DescriptorSetLayout creation" );
		crgRegisterObject( m_context, m_pass.name, m_descriptorSetLayout );
	}

	void RunnablePass::doCreatePipelineLayout()
	{
		VkPipelineLayoutCreateInfo createInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO
			, nullptr
			, 0u
			, 1u                                           // setLayoutCount
			, &m_descriptorSetLayout                       // pSetLayouts
			, uint32_t( m_baseConfig.pushRanges.size() )   // pushConstantRangeCount
			, ( m_baseConfig.pushRanges.empty()            // pPushConstantRanges
				? nullptr
				: m_baseConfig.pushRanges.data() ) };
		auto res = m_context.vkCreatePipelineLayout( m_context.device
			, &createInfo
			, m_context.allocator
			, &m_pipelineLayout );
		checkVkResult( res, "PipeliineLayout creation" );
		crgRegisterObject( m_context, m_pass.name, m_pipelineLayout );
	}

	void RunnablePass::doCreateDescriptorPool()
	{
		assert( m_descriptorSetLayout );
		VkDescriptorSetLayoutBindingArray bindings;
		uint32_t index = 0u;

		for ( auto & binding : m_pass.sampled )
		{
			bindings.push_back( { index++
				, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE
				, 1u
				, VK_SHADER_STAGE_FRAGMENT_BIT
				, nullptr } );
		}

		auto sizes = convert( bindings, 1u );
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
		checkVkResult( res, "DescriptorPool creation" );
		crgRegisterObject( m_context, m_pass.name, m_descriptorSetPool );
	}

	void RunnablePass::doCreateDescriptorSet()
	{
		VkDescriptorSetAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO
			, nullptr
			, m_descriptorSetPool
			, 1u
			, &m_descriptorSetLayout };
		auto res = m_context.vkAllocateDescriptorSets( m_context.device
			, &allocateInfo
			, &m_descriptorSet );
		checkVkResult( res, "DescriptorSet allocation" );
		crgRegisterObject( m_context, m_pass.name, m_descriptorSet );

		for ( auto & write : m_descriptorWrites )
		{
			write.update( m_descriptorSet );
		}

		auto descriptorWrites = makeVkArray< VkWriteDescriptorSet >( m_descriptorWrites );
		m_context.vkUpdateDescriptorSets( m_context.device
			, uint32_t( descriptorWrites.size() )
			, descriptorWrites.data()
			, 0u
			, nullptr );
	}

	void RunnablePass::doCreateCommandPool()
	{
		VkCommandPoolCreateInfo createInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO
			, nullptr
			, 0u
			, };
		auto res = m_context.vkCreateCommandPool( m_context.device
			, &createInfo
			, m_context.allocator
			, &m_commandPool );
		checkVkResult( res, "CommandPool creation" );
		crgRegisterObject( m_context, m_pass.name, m_commandPool );
	}

	void RunnablePass::doCreateCommandBuffer()
	{
		VkCommandBufferAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO
			, nullptr
			, m_commandPool
			, VK_COMMAND_BUFFER_LEVEL_PRIMARY
			, 1u };
		auto res = m_context.vkAllocateCommandBuffers( m_context.device
			, &allocateInfo
			, &m_commandBuffer );
		checkVkResult( res, "CommandBuffer allocation" );
		crgRegisterObject( m_context, m_pass.name, m_commandBuffer );
	}

	void RunnablePass::doCreateSemaphore()
	{
		VkSemaphoreCreateInfo createInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
			, nullptr
			, 0u };
		auto res = m_context.vkCreateSemaphore( m_context.device
			, &createInfo
			, m_context.allocator
			, &m_semaphore );
		checkVkResult( res, "Semaphore creation" );
		crgRegisterObject( m_context, m_pass.name, m_semaphore );
	}

	void RunnablePass::doRecordInto( VkCommandBuffer commandBuffer )const
	{
	}
}
