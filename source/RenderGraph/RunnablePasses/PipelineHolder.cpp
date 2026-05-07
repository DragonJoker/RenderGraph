/*
See LICENSE file in root folder.
*/
#include "RenderGraph/RunnablePasses/PipelineHolder.hpp"

#include "RenderGraph/GraphContext.hpp"
#include "RenderGraph/Hash.hpp"
#include "RenderGraph/RunnableGraph.hpp"
#include "RenderGraph/RunnablePasses/RenderPass.hpp"

#include <cassert>

namespace crg
{
	namespace pphdr
	{
		enum class DescriptorType : uint8_t
		{
			eStorageImage,
			eCombinedImageSampler,
			eUniformTexelBuffer,
			eStorageTexelBuffer,
			eUniformBuffer,
			eStorageBuffer,
		};

		static DescriptorType getDescriptorType( BufferAttachment const & attach )
		{
			if ( attach.isUniformView() )
				return DescriptorType::eUniformTexelBuffer;
			if ( attach.isStorageView() )
				return DescriptorType::eStorageTexelBuffer;
			if ( attach.isUniform() )
				return DescriptorType::eUniformBuffer;
			return DescriptorType::eStorageBuffer;
		}

		static DescriptorType getDescriptorType( ImageAttachment const & attach )
		{
			if ( attach.isStorageView() )
				return DescriptorType::eStorageImage;
			return DescriptorType::eCombinedImageSampler;
		}

		static DescriptorType getDescriptorType( Attachment const & attach )
		{
			if ( attach.isImage() )
				return getDescriptorType( attach.imageAttach );
			return getDescriptorType( attach.bufferAttach );
		}

		static bool isDescriptor( Attachment const & attach )
		{
			return attach.isStorageImageView() || attach.isSampledImageView()
				|| attach.isUniformBuffer() || attach.isStorageBuffer()
				|| attach.isUniformBufferView() || attach.isStorageBufferView();
		}

		static void createDescriptorWrites( std::map< uint32_t, FramePass::SampledAttachment > const & attaches
			, uint32_t index
			, RunnableGraph & graph
			, WriteDescriptorSetArray & writes )
		{
			for ( auto & [binding, attach] : attaches )
				writes.push_back( graph.getDescriptorWrite( *attach.attach, attach.sampler, binding, index ) );
		}
		
		static void createDescriptorWrites( std::map< uint32_t, Attachment const * > const & attaches
			, uint32_t index
			, RunnableGraph & graph
			, WriteDescriptorSetArray & writes )
		{
			for ( auto & [binding, attach] : attaches )
			{
				if ( isDescriptor( *attach ) )
					writes.push_back( graph.getDescriptorWrite( *attach, binding, index ) );
			}
		}

		static void createDescriptorBindings( std::map< uint32_t, FramePass::SampledAttachment > const & attaches
			, VkShaderStageFlags shaderStage
			, RunnableGraph const & graph
			, VkDescriptorSetLayoutBindingArray & descriptorBindings )
		{
			for ( auto & [binding, attach] : attaches )
			{
				descriptorBindings.push_back( { binding
					, graph.getDescriptorType( *attach.attach )
					, 1u, shaderStage, nullptr } );
			}
		}

		static uint32_t getDescriptorTotalCount( Attachment const & attach )
		{
			return attach.isBuffer() ? attach.buffer().data->buffer.data->maxPages : 1u;
		}

		static uint32_t getDescriptorAllocatedCount( Attachment const & attach )
		{
			return attach.isBuffer() ? attach.buffer().data->buffer.data->allocatedPages : 1u;
		}

		static void createDescriptorBindings( std::map< uint32_t, Attachment const * > const & attaches
			, VkShaderStageFlags shaderStage
			, RunnableGraph const & graph
			, VkDescriptorSetLayoutBindingArray & descriptorBindings )
		{
			for ( auto & [binding, attach] : attaches )
			{
				if ( isDescriptor( *attach ) )
					descriptorBindings.push_back( { binding
						, graph.getDescriptorType( *attach )
						, getDescriptorTotalCount( *attach ), shaderStage, nullptr } );
			}
		}

		static uint32_t getDescriptorId( Attachment const & attach
			, uint32_t index )
		{
			if ( attach.isImage() )
				return attach.view( index ).id;
			return attach.buffer( index ).id;
		}

		static_assert( sizeof( size_t ) >= sizeof( uint64_t ) );

		static size_t getDescriptorHash( uint32_t binding
			, Attachment const & attach
			, uint32_t index )
		{
			return ( ( uint64_t( getDescriptorId( attach, index ) ) & 0xFFFFFFFFull ) << 32u ) // 32 bits for resource ID
				| ( ( uint64_t( getDescriptorType( attach ) ) & 0xFFull ) << 24u ) // 8 bits for descriptor type
				| ( ( uint64_t( getDescriptorAllocatedCount( attach ) ) & 0xFFull ) << 16u ) // 8 bits for descriptor count
				| ( ( uint64_t( binding ) & 0xFFFFull ) << 0u ); // 16 bits for descriptor binding
		}

		static size_t getDescriptorsHash( std::map< uint32_t, FramePass::SampledAttachment > const & attaches
			, uint32_t index )
		{
			size_t result{};
			for ( auto & [binding, attach] : attaches )
				result = hashCombine( result, getDescriptorHash( binding, *attach.attach, index ) );
			return result;
		}

		static size_t getDescriptorsHash( std::map< uint32_t, Attachment const * > const & attaches
			, uint32_t index )
		{
			size_t result{};
			for ( auto & [binding, attach] : attaches )
			{
				if ( isDescriptor( *attach ) )
					result = hashCombine( result, getDescriptorHash( binding, *attach, index ) );
			}
			return result;
		}

		static size_t makeDescriptorSetHash( crg::FramePass const & pass
			, uint32_t index )
		{
			size_t result{};
			result = hashCombine( result, pphdr::getDescriptorsHash( pass.getUniforms(), index ) );
			result = hashCombine( result, pphdr::getDescriptorsHash( pass.getSampled(), index ) );
			result = hashCombine( result, pphdr::getDescriptorsHash( pass.getInputs(), index ) );
			result = hashCombine( result, pphdr::getDescriptorsHash( pass.getInouts(), index ) );
			result = hashCombine( result, pphdr::getDescriptorsHash( pass.getOutputs(), index ) );
			return result;
		}

		static bool hasPagedBuffers( std::map< uint32_t, Attachment const * > const & attaches )
		{
			return std::any_of( attaches.begin(), attaches.end()
				, []( std::map< uint32_t, Attachment const * >::value_type const & lookup )
				{
					bool result = lookup.second->isBuffer();
					if ( result )
						result = lookup.second->buffer().data->buffer.data->maxPages > 1u;
					return result;
				} );
		}

		static bool hasPagedBuffers( crg::FramePass const & pass )
		{
			return pphdr::hasPagedBuffers( pass.getUniforms() )
				|| pphdr::hasPagedBuffers( pass.getInputs() )
				|| pphdr::hasPagedBuffers( pass.getInouts() )
				|| pphdr::hasPagedBuffers( pass.getOutputs() );
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
		, m_baseConfig{ config.m_programs ? *config.m_programs : defaultV< std::vector< VkPipelineShaderStageCreateInfoArray > >
			, config.m_programCreator ? *config.m_programCreator : defaultV< ProgramCreator >
			, config.m_layouts ? *config.m_layouts : defaultV< std::vector< VkDescriptorSetLayout > >
			, config.m_pushConstants ? *config.m_pushConstants : defaultV< std::vector< VkPushConstantRange > > }
		, m_bindingPoint{ bindingPoint }
	{
		if ( m_baseConfig.m_programCreator.create )
		{
			m_pipelines.resize( m_baseConfig.m_programCreator.maxCount, VkPipeline{} );
			m_baseConfig.m_programs.resize( m_baseConfig.m_programCreator.maxCount );
		}
		else
		{
			m_pipelines.resize( m_baseConfig.m_programs.size(), VkPipeline{} );
		}

		m_descriptorSets.resize( maxPassCount );
	}

	PipelineHolder::~PipelineHolder()noexcept
	{
		cleanup();
	}

	void PipelineHolder::initialise()
	{
		if ( !m_pipelineLayout )
		{
			doFillDescriptorBindings();
			doCreateDescriptorSetLayout();
			doCreatePipelineLayout();
			doCreateDescriptorPool();
		}
	}

	void PipelineHolder::cleanup()noexcept
	{
		m_descriptorBindings.clear();

		if ( m_descriptorSetPool )
		{
			bool hasPagedBuffers = pphdr::hasPagedBuffers( m_pass );
			for ( auto & descriptorSet : m_descriptorSets )
			{
				if ( descriptorSet.set )
				{
					crgUnregisterObject( m_context, descriptorSet.set );
					if ( hasPagedBuffers )
						m_context.vkFreeDescriptorSets( m_context.device, m_descriptorSetPool, 1u, &descriptorSet.set );
					descriptorSet.writes.clear();
					descriptorSet.set = {};
				}
			}

			crgUnregisterObject( m_context, m_descriptorSetPool );
			m_context.vkDestroyDescriptorPool( m_context.device
				, m_descriptorSetPool
				, m_context.allocator );
			m_descriptorSetPool = {};
		}

		for ( auto & pipeline : m_pipelines )
		{
			if ( pipeline != VkPipeline{} )
			{
				crgUnregisterObject( m_context, pipeline );
				m_context.vkDestroyPipeline( m_context.device
					, pipeline
					, m_context.allocator );
				pipeline = {};
			}
		}

		if ( m_pipelineLayout )
		{
			crgUnregisterObject( m_context, m_pipelineLayout );
			m_context.vkDestroyPipelineLayout( m_context.device
				, m_pipelineLayout
				, m_context.allocator );
			m_pipelineLayout = {};
		}

		if ( m_descriptorSetLayout )
		{
			crgUnregisterObject( m_context, m_descriptorSetLayout );
			m_context.vkDestroyDescriptorSetLayout( m_context.device
				, m_descriptorSetLayout
				, m_context.allocator );
			m_descriptorSetLayout = {};
		}
	}

	VkPipelineShaderStageCreateInfoArray const & PipelineHolder::getProgram( uint32_t index )
	{
		if ( m_baseConfig.m_programCreator.create )
		{
			assert( m_baseConfig.m_programCreator.maxCount > index );
			m_baseConfig.m_programs[index] = m_baseConfig.m_programCreator.create( index );
		}

		if ( m_baseConfig.m_programs.size() == 1u )
		{
			return m_baseConfig.m_programs[0];
		}

		assert( m_baseConfig.m_programs.size() > index );
		return m_baseConfig.m_programs[index];
	}

	VkPipeline & PipelineHolder::getPipeline( uint32_t index )
	{
		if ( m_baseConfig.m_programs.size() == 1u )
		{
			assert( m_pipelines.size() == 1u );
			return m_pipelines[0];
		}

		assert( m_pipelines.size() > index );
		return m_pipelines[index];
	}

	void PipelineHolder::createPipeline( uint32_t index
		, std::string const & name
		, VkGraphicsPipelineCreateInfo const & createInfo )
	{
		if ( m_context.vkCreateGraphicsPipelines )
		{
			auto & pipeline = getPipeline( index );
			auto res = m_context.vkCreateGraphicsPipelines( m_context.device
				, m_context.cache
				, 1u
				, &createInfo
				, m_context.allocator
				, &pipeline );
			crg::checkVkResult( res, name + " - Pipeline creation" );
			crgRegisterObject( m_context, name, pipeline );
		}
	}

	void PipelineHolder::createPipeline( uint32_t index
		, VkGraphicsPipelineCreateInfo const & createInfo )
	{
		createPipeline( index
			, m_pass.getGroupName()
			, createInfo );
	}

	void PipelineHolder::createPipeline( uint32_t index
		, std::string const & name
		, VkComputePipelineCreateInfo const & createInfo )
	{
		if ( m_context.vkCreateComputePipelines )
		{
			auto & pipeline = getPipeline( index );
			auto res = m_context.vkCreateComputePipelines( m_context.device
				, m_context.cache
				, 1u
				, &createInfo
				, m_context.allocator
				, &pipeline );
			checkVkResult( res, name + " - Pipeline creation" );
			crgRegisterObject( m_context, name, pipeline );
		}
	}

	void PipelineHolder::createPipeline( uint32_t index
		, VkComputePipelineCreateInfo const & createInfo )
	{
		createPipeline( index
			, m_pass.getGroupName()
			, createInfo );
	}

	void PipelineHolder::recordInto( RecordContext const & context
		, VkCommandBuffer commandBuffer
		, uint32_t index )
	{
		createDescriptorSet( index );
		auto & pipeline = getPipeline( index );
		context->vkCmdBindPipeline( commandBuffer, m_bindingPoint, pipeline );
		context->vkCmdBindDescriptorSets( commandBuffer, m_bindingPoint, m_pipelineLayout, 0u, 1u, &m_descriptorSets[index].set, 0u, nullptr );
	}

	void PipelineHolder::resetPipelineLayout( std::vector< VkDescriptorSetLayout > const & layouts
		, std::vector< VkPushConstantRange > const & ranges
		, VkPipelineShaderStageCreateInfoArray const & config )
	{
		bool hadPipelineLayout{};
		if ( m_pipelineLayout )
		{
			hadPipelineLayout = true;
			auto pipelineLayout = m_pipelineLayout;
			m_context.delQueue.push( [pipelineLayout]( GraphContext & context )
				{
					crgUnregisterObject( context, pipelineLayout );
					context.vkDestroyPipelineLayout( context.device
						, pipelineLayout
						, context.allocator );
				} );
			m_pipelineLayout = {};
		}

		m_baseConfig.layouts( layouts );
		m_baseConfig.pushConstants( ranges );

		if ( hadPipelineLayout )
			doCreatePipelineLayout();

		auto count = uint32_t( m_pipelines.size() );
		for ( uint32_t i = 0; i < count; ++i )
			resetPipeline( config, i );
	}

	void PipelineHolder::resetPipeline( VkPipelineShaderStageCreateInfoArray config
		, uint32_t index )
	{
		assert( m_pipelines.size() == 1u || index < m_pipelines.size() );

		if ( m_pipelines.size() == 1u )
		{
			index = 0u;
		}

		if ( m_pipelines[index] )
		{
			auto pipeline = m_pipelines[index];
			m_context.delQueue.push( [pipeline]( GraphContext & context )
				{
					crgUnregisterObject( context, pipeline );
					context.vkDestroyPipeline( context.device
						, pipeline
						, context.allocator );
				} );
			m_pipelines[index] = {};
		}

		if ( !config.empty() )
		{
			assert( m_baseConfig.m_programs.size() > index );
			m_baseConfig.m_programs[index] = std::move( config );
		}
	}

	void PipelineHolder::createDescriptorSet( uint32_t index )
	{
		auto & descriptorSet = m_descriptorSets[index];
		auto hash = pphdr::makeDescriptorSetHash( m_pass, index );

		if ( descriptorSet.set != VkDescriptorSet{}
			&& descriptorSet.hash == hash )
		{
			return;
		}

		if ( descriptorSet.set != VkDescriptorSet{} )
		{
			auto ds = descriptorSet.set;
			auto pool = m_descriptorSetPool;
			m_context.delQueue.push( [ds, pool]( GraphContext & context )
				{
					context.vkFreeDescriptorSets( context.device, pool, 1u, &ds );
				} );
			descriptorSet.writes.clear();
			descriptorSet.set = {};
		}

		descriptorSet.hash = hash;

		pphdr::createDescriptorWrites( m_pass.getUniforms(), index, m_graph, descriptorSet.writes );
		pphdr::createDescriptorWrites( m_pass.getSampled(), index, m_graph, descriptorSet.writes );
		pphdr::createDescriptorWrites( m_pass.getInputs(), index, m_graph, descriptorSet.writes );
		pphdr::createDescriptorWrites( m_pass.getInouts(), index, m_graph, descriptorSet.writes );
		pphdr::createDescriptorWrites( m_pass.getOutputs(), index, m_graph, descriptorSet.writes );

		VkDescriptorSetAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO
			, nullptr
			, m_descriptorSetPool
			, 1u
			, &m_descriptorSetLayout };
		auto res = m_context.vkAllocateDescriptorSets( m_context.device
			, &allocateInfo
			, &descriptorSet.set );
		checkVkResult( res, m_pass.getGroupName() + " - DescriptorSet allocation" );
		crgRegisterObject( m_context, m_pass.getGroupName(), descriptorSet.set );

		for ( auto const & write : descriptorSet.writes )
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
		m_descriptorBindings.clear();
		auto shaderStage = VkShaderStageFlags( ( VK_PIPELINE_BIND_POINT_COMPUTE == m_bindingPoint )
			? VK_SHADER_STAGE_COMPUTE_BIT
			: ( VK_SHADER_STAGE_VERTEX_BIT
				| VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT
				| VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT
				| VK_SHADER_STAGE_GEOMETRY_BIT
				| VK_SHADER_STAGE_FRAGMENT_BIT ) );

		pphdr::createDescriptorBindings( m_pass.getUniforms(), shaderStage, m_graph, m_descriptorBindings );
		pphdr::createDescriptorBindings( m_pass.getSampled(), shaderStage, m_graph, m_descriptorBindings );
		pphdr::createDescriptorBindings( m_pass.getInputs(), shaderStage, m_graph, m_descriptorBindings );
		pphdr::createDescriptorBindings( m_pass.getInouts(), shaderStage, m_graph, m_descriptorBindings );
		pphdr::createDescriptorBindings( m_pass.getOutputs(), shaderStage, m_graph, m_descriptorBindings );
	}

	void PipelineHolder::doCreateDescriptorSetLayout()
	{
		if ( m_context.vkCreateDescriptorSetLayout )
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
			checkVkResult( res, m_pass.getGroupName() + " - DescriptorSetLayout creation" );
			crgRegisterObject( m_context, m_pass.getGroupName(), m_descriptorSetLayout );
		}
	}

	void PipelineHolder::doCreatePipelineLayout()
	{
		if ( m_context.vkCreatePipelineLayout )
		{
			std::vector< VkDescriptorSetLayout > layouts;
			layouts.push_back( m_descriptorSetLayout );
			layouts.insert( layouts.end()
				, m_baseConfig.m_layouts.begin()
				, m_baseConfig.m_layouts.end() );
			VkPipelineLayoutCreateInfo createInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO
				, nullptr
				, 0u
				, uint32_t( layouts.size() )
				, layouts.data()
				, uint32_t( m_baseConfig.m_pushConstants.size() )
				, m_baseConfig.m_pushConstants.data() };
			auto res = m_context.vkCreatePipelineLayout( m_context.device
				, &createInfo
				, m_context.allocator
				, &m_pipelineLayout );
			checkVkResult( res, m_pass.getGroupName() + " - PipeliineLayout creation" );
			crgRegisterObject( m_context, m_pass.getGroupName(), m_pipelineLayout );
		}
	}

	void PipelineHolder::doCreateDescriptorPool()
	{
		if ( m_context.vkCreateDescriptorPool )
		{
			assert( m_descriptorSetLayout );
			VkDescriptorPoolCreateFlags flags{};
			if ( pphdr::hasPagedBuffers( m_pass ) )
				flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
			// x2 to account for descriptor set changes (the deallocation is deferred)
			auto maxSets = uint32_t( m_descriptorSets.size() * 2u );
			auto sizes = getBindingsSizes( m_descriptorBindings, maxSets );
			VkDescriptorPoolCreateInfo createInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO
				, nullptr
				, flags
				, maxSets
				, uint32_t( sizes.size() )
				, sizes.data() };
			auto res = m_context.vkCreateDescriptorPool( m_context.device
				, &createInfo
				, m_context.allocator
				, &m_descriptorSetPool );
			checkVkResult( res, m_pass.getGroupName() + " - DescriptorPool creation" );
			crgRegisterObject( m_context, m_pass.getGroupName(), m_descriptorSetPool );
		}
	}
}
