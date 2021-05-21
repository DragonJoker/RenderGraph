/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#include "RenderGraph/RenderQuad.hpp"

#include "RenderGraph/Attachment.hpp"
#include "RenderGraph/GraphContext.hpp"
#include "RenderGraph/ImageData.hpp"
#include "RenderGraph/RunnableGraph.hpp"

#include <array>

namespace crg
{
	RenderQuad::RenderQuad( FramePass const & pass
		, GraphContext const & context
		, RunnableGraph & graph
		, rq::Config config )
		: RenderPass{ pass
			, context
			, graph }
		, m_config{ std::move( config.program ? *config.program : defaultV< VkPipelineShaderStageCreateInfoArray > )
			, std::move( config.texcoordConfig ? *config.texcoordConfig : defaultV< rq::Texcoord > )
			, std::move( config.renderSize ? *config.renderSize : defaultV< VkExtent2D > )
			, std::move( config.renderPosition ? *config.renderPosition : defaultV< VkOffset2D > ) }
		, m_useTexCoord{ config.texcoordConfig }
	{
	}

	RenderQuad::~RenderQuad()
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

	void RenderQuad::doSubInitialise()
	{
		m_vertexBuffer = &m_graph.createQuadVertexBuffer( m_useTexCoord
			, m_config.texcoordConfig.invertU
			, m_config.texcoordConfig.invertV );
		doFillDescriptorBindings();
		doCreateDescriptorSetLayout();
		doCreatePipelineLayout();
		doCreateDescriptorPool();
		doCreateDescriptorSet();
		doCreatePipeline();
	}

	void RenderQuad::doSubRecordInto( VkCommandBuffer commandBuffer )const
	{
		VkDeviceSize offset{};
		m_context.vkCmdBindPipeline( commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline );
		m_context.vkCmdBindDescriptorSets( commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0u, 1u, &m_descriptorSet, 0u, nullptr );
		m_context.vkCmdBindVertexBuffers( commandBuffer, 0u, 1u, &m_vertexBuffer->buffer, &offset );
		m_context.vkCmdDraw( commandBuffer, 4u, 1u, 0u, 0u );
	}

	void RenderQuad::doFillDescriptorBindings()
	{
		WriteDescriptorSetArray descriptorWrites;
		VkShaderStageFlags shaderStage = VK_SHADER_STAGE_FRAGMENT_BIT;
		auto imageDescriptor = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

		for ( auto & sampled : m_pass.sampled )
		{
			m_descriptorBindings.push_back( { sampled.binding
				, imageDescriptor
				, 1u
				, shaderStage
				, nullptr } );
			m_descriptorWrites.push_back( WriteDescriptorSet{ sampled.binding
				, 0u
				, imageDescriptor
				, VkDescriptorImageInfo{ m_graph.createSampler( sampled.filter )
				, m_graph.getImageView( sampled )
				, ( sampled.initialLayout != VK_IMAGE_LAYOUT_UNDEFINED
					? sampled.initialLayout
					: sampled.getImageLayout( m_context.separateDepthStencilLayouts ) ) } } );
		}

		for ( auto & storage : m_pass.storage )
		{
			m_descriptorBindings.push_back( { storage.binding
				, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
				, 1u
				, shaderStage
				, nullptr } );
			m_descriptorWrites.push_back( WriteDescriptorSet{ storage.binding
				, 0u
				, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
				, VkDescriptorImageInfo{ VK_NULL_HANDLE
				, m_graph.getImageView( storage )
				, ( storage.initialLayout != VK_IMAGE_LAYOUT_UNDEFINED
					? storage.initialLayout
					: storage.getImageLayout( m_context.separateDepthStencilLayouts ) ) } } );
		}

		shaderStage = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

		for ( auto & uniform : m_pass.buffers )
		{
			m_descriptorBindings.push_back( { uniform->dstBinding
				, uniform->descriptorType
				, uniform->descriptorCount
				, shaderStage
				, nullptr } );
			m_descriptorWrites.push_back( uniform );
		}

		for ( auto & uniform : m_pass.bufferViews )
		{
			m_descriptorBindings.push_back( { uniform->dstBinding
				, uniform->descriptorType
				, uniform->descriptorCount
				, shaderStage
				, nullptr } );
			m_descriptorWrites.push_back( uniform );
		}
	}

	void RenderQuad::doCreateDescriptorSetLayout()
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

	void RenderQuad::doCreatePipelineLayout()
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
		checkVkResult( res, "PipeliineLayout creation" );
		crgRegisterObject( m_context, m_pass.name, m_pipelineLayout );
	}

	void RenderQuad::doCreateDescriptorPool()
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

		auto sizes = getBindingsSizes( bindings, 1u );
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

	void RenderQuad::doCreateDescriptorSet()
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

	void RenderQuad::doCreatePipeline()
	{
		VkVertexInputAttributeDescriptionArray vertexAttribs;
		VkVertexInputBindingDescriptionArray vertexBindings;
		VkViewportArray viewports;
		VkScissorArray scissors;
		auto vpState = doCreateViewportState( viewports, scissors );
		auto cbState = doCreateBlendState();
		VkPipelineInputAssemblyStateCreateInfo iaState{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO
			, nullptr
			, 0u
			, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP
			, VK_FALSE };
		VkPipelineMultisampleStateCreateInfo msState{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO
			, nullptr
			, 0u
			, VK_SAMPLE_COUNT_1_BIT
			, VK_FALSE
			, 0.0f
			, nullptr
			, VK_FALSE
			, VK_FALSE };
		VkPipelineRasterizationStateCreateInfo rsState{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO
			, nullptr
			, 0u
			, VK_FALSE
			, VK_FALSE
			, VK_POLYGON_MODE_FILL
			, VK_CULL_MODE_NONE
			, VK_FRONT_FACE_COUNTER_CLOCKWISE
			, VK_FALSE
			, 0.0f
			, 0.0f
			, 0.0f
			, 0.0f };
		VkPipelineDepthStencilStateCreateInfo dsState{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO
			, nullptr
			, 0u
			, VK_FALSE
			, VK_FALSE };
		VkGraphicsPipelineCreateInfo createInfo{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO
			, nullptr
			, 0u
			, uint32_t( m_config.program.size() )
			, m_config.program.data()
			, &m_vertexBuffer->inputState
			, &iaState
			, nullptr
			, &vpState
			, &rsState
			, &msState
			, &dsState
			, &cbState
			, nullptr
			, m_pipelineLayout
			, m_renderPass
			, 0u
			, VK_NULL_HANDLE
			, 0u };
		auto res = m_context.vkCreateGraphicsPipelines( m_context.device
			, m_context.cache
			, 1u
			, &createInfo
			, m_context.allocator
			, &m_pipeline );
		checkVkResult( res, "Pipeline creation" );
		crgRegisterObject( m_context, m_pass.name, m_pipeline );
	}

	VkPipelineViewportStateCreateInfo RenderQuad::doCreateViewportState( VkViewportArray & viewports
		, VkScissorArray & scissors )
	{
		viewports.push_back( { float( m_config.renderPosition.x )
			, float( m_config.renderPosition.y )
			, float( m_config.renderSize.width )
			, float( m_config.renderSize.height ) } );
		scissors.push_back( { m_config.renderPosition.x
			, m_config.renderPosition.y
			, m_config.renderSize.width
			, m_config.renderSize.height } );
		return { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO
			, nullptr
			, 0u
			, uint32_t( viewports.size())
			, viewports.data()
			, uint32_t( scissors.size())
			, scissors.data() };
	}
}
