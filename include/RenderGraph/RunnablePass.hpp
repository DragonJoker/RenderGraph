/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "RenderGraph/FramePass.hpp"

#include <optional>

namespace crg
{
	struct SemaphoreWait
	{
		VkSemaphore semaphore;
		VkPipelineStageFlags dstStageMask;
	};
	struct WriteDescriptorSet
	{
		WriteDescriptorSet( uint32_t dstBinding
			, uint32_t dstArrayElement
			, uint32_t descriptorCount
			, VkDescriptorType descriptorType )
			: vk{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, VK_NULL_HANDLE, dstBinding, dstArrayElement, descriptorCount, descriptorType }
			, needsUpdate{ true }
		{
		}

		WriteDescriptorSet( uint32_t dstBinding
			, uint32_t dstArrayElement
			, VkDescriptorType descriptorType
			, VkDescriptorImageInfo imageInfos )
			: imageInfo{ 1u, imageInfos }
			, vk{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, VK_NULL_HANDLE, dstBinding, dstArrayElement, uint32_t( this->imageInfo.size() ), descriptorType }
			, needsUpdate{ true }
		{
		}

		WriteDescriptorSet( uint32_t dstBinding
			, uint32_t dstArrayElement
			, VkDescriptorType descriptorType
			, VkDescriptorImageInfoArray imageInfos )
			: imageInfo{ std::move( imageInfos ) }
			, vk{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, VK_NULL_HANDLE, dstBinding, dstArrayElement, uint32_t( this->imageInfo.size() ), descriptorType }
			, needsUpdate{ true }
		{
		}

		WriteDescriptorSet( VkDescriptorSet set
			, uint32_t dstBinding
			, uint32_t dstArrayElement
			, uint32_t descriptorCount
			, VkDescriptorType descriptorType
			, VkDescriptorImageInfo const * imageInfo
			, VkDescriptorBufferInfo const * bufferInfo
			, VkBufferView const * texelBufferView )
			: vk{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, set, dstBinding, dstArrayElement, descriptorCount, descriptorType, imageInfo, bufferInfo, texelBufferView }
			, needsUpdate{ false }
		{
		}

		void update( VkDescriptorSet descriptorSet )const
		{
			if ( needsUpdate )
			{
				vk.dstSet = descriptorSet;
				vk.pImageInfo = imageInfo.data();
				vk.pBufferInfo = bufferInfo.data();
				vk.pTexelBufferView = texelBufferView.data();
			}
		}

		operator VkWriteDescriptorSet const & ()const
		{
			return vk;
		}

		inline VkWriteDescriptorSet const * operator->()const
		{
			return &vk;
		}

		inline VkWriteDescriptorSet * operator->()
		{
			return &vk;
		}

		VkDescriptorImageInfoArray imageInfo;
		VkDescriptorBufferInfoArray bufferInfo;
		VkBufferViewArray texelBufferView;

	private:
		mutable VkWriteDescriptorSet vk;
		bool needsUpdate;
	};

	template< typename TypeT >
	struct DefaultValueGetterT;

	template<>
	struct DefaultValueGetterT< VkPipelineShaderStageCreateInfoArray >
	{
		static VkPipelineShaderStageCreateInfoArray get()
		{
			return VkPipelineShaderStageCreateInfoArray{};
		}
	};

	template<>
	struct DefaultValueGetterT< VkPushConstantRangeArray >
	{
		static VkPushConstantRangeArray get()
		{
			return VkPushConstantRangeArray{};
		}
	};

	template<>
	struct DefaultValueGetterT< WriteDescriptorSetArray >
	{
		static WriteDescriptorSetArray get()
		{
			return WriteDescriptorSetArray{};
		}
	};

	template<>
	struct DefaultValueGetterT< VkPipelineDepthStencilStateCreateInfo >
	{
		static VkPipelineDepthStencilStateCreateInfo get()
		{
			return VkPipelineDepthStencilStateCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO
				, nullptr
				, 0u
				, VK_FALSE
				, VK_FALSE };
		}
	};

	template< typename TypeT >
	static inline TypeT const & defaultV = DefaultValueGetterT< TypeT >::get();

	template< typename TypeT >
	struct RawTyperT
	{
		using Type = TypeT;
	};

	template< typename TypeT >
	using RawTypeT = typename RawTyperT< TypeT >::Type;

	namespace rp
	{
		template< template< typename ValueT > typename WrapperT >
		struct ConfigT
		{
			WrapperT< VkPipelineShaderStageCreateInfoArray > program;
			WrapperT< VkPushConstantRangeArray > pushRanges;
			WrapperT< VkPipelineDepthStencilStateCreateInfo > dsState;
			WrapperT< WriteDescriptorSetArray > descriptorWrites;
		};

		using Config = ConfigT< std::optional >;
		using ConfigData = ConfigT< RawTypeT >;
	}

	class RunnablePass
	{
	public:
		RunnablePass( FramePass const & pass
			, GraphContext const & context
			, RunnableGraph & graph
			, rp::Config config
			, VkPipelineBindPoint bindingPoint );
		virtual ~RunnablePass();
		/**
		*\brief
		*	Initialises the descriptor set.
		*/
		void initialise();
		/**
		*\brief
		*	Records the render pass into the given command buffer.
		*\param[in,out] commandBuffer
		*	The command buffer.
		*/
		void record()const;
		/**
		*\brief
		*	Records the render pass into the given command buffer.
		*\param[in,out] commandBuffer
		*	The command buffer.
		*/
		virtual void recordInto( VkCommandBuffer commandBuffer )const;
		/**
		*\brief
		*	Submits this pass' command buffer to the givent queue.
		*\param[in] toWait
		*	The semaphore to wait for.
		*\param[out] queue
		*	The queue to submit to.
		*\return
		*	This pass' semaphore.
		*/
		SemaphoreWait run( SemaphoreWait toWait
			, VkQueue queue )const;

	private:
		void doFillDescriptorBindings();
		void doCreateDescriptorSetLayout();
		void doCreatePipelineLayout();
		void doCreateDescriptorPool();
		void doCreateDescriptorSet();
		void doCreateCommandPool();
		void doCreateCommandBuffer();
		void doCreateSemaphore();

		virtual void doInitialise() = 0;
		virtual void doRecordInto( VkCommandBuffer commandBuffer )const;

	protected:
		rp::ConfigData m_baseConfig;
		FramePass const & m_pass;
		GraphContext const & m_context;
		RunnableGraph & m_graph;
		VkPipelineBindPoint m_bindingPoint;
		WriteDescriptorSetArray m_descriptorWrites;
		VkDescriptorSetLayoutBindingArray m_descriptorBindings;
		VkDescriptorSetLayout m_descriptorSetLayout{ VK_NULL_HANDLE };
		VkPipelineLayout m_pipelineLayout{ VK_NULL_HANDLE };
		VkPipeline m_pipeline{ VK_NULL_HANDLE };
		VkDescriptorPool m_descriptorSetPool{ VK_NULL_HANDLE };
		VkDescriptorSet m_descriptorSet{ VK_NULL_HANDLE };
		VkCommandPool m_commandPool{ VK_NULL_HANDLE };
		VkCommandBuffer m_commandBuffer{ VK_NULL_HANDLE };
		VkSemaphore m_semaphore{ VK_NULL_HANDLE };
	};

	template< typename BuilderT >
	class RunnablePassBuilderT
	{
	public:
		RunnablePassBuilderT() = default;

		BuilderT & program( VkPipelineShaderStageCreateInfoArray config )
		{
			m_config.program = std::move( config );
			return static_cast< BuilderT & >( *this );
		}

		BuilderT & pushRanges( VkPushConstantRangeArray config )
		{
			m_config.pushRanges = std::move( config );
			return static_cast< BuilderT & >( *this );
		}

		BuilderT & dsState( VkPipelineDepthStencilStateCreateInfo config )
		{
			m_config.dsState = std::move( config );
			return static_cast< BuilderT & >( *this );
		}

		BuilderT & descriptorWrites( WriteDescriptorSetArray config )
		{
			m_config.descriptorWrites = std::move( config );
			return static_cast< BuilderT & >( *this );
		}

		BuilderT & descriptorWrite( VkBuffer const & buffer
			, VkDescriptorType descriptorType
			, uint32_t dstBinding
			, uint32_t byteOffset
			, uint32_t byteRange
			, uint32_t dstArrayElement = 0u )
		{
			if ( !m_config.descriptorWrites )
			{
				m_config.descriptorWrites = WriteDescriptorSetArray{};
			}

			auto result = WriteDescriptorSet{ dstBinding
				, dstArrayElement
				, 1u
				, descriptorType };
			result.bufferInfo.push_back( { buffer
				, byteOffset
				, byteRange } );
			m_config.descriptorWrites.value().push_back( result );
			return static_cast< BuilderT & >( *this );
		}

		BuilderT & descriptorWrite( VkBuffer const & buffer
			, VkBufferView const & view
			, VkDescriptorType descriptorType
			, uint32_t dstBinding
			, uint32_t byteOffset
			, uint32_t byteRange
			, uint32_t dstArrayElement = 0u )
		{
			if ( !m_config.descriptorWrites )
			{
				m_config.descriptorWrites = WriteDescriptorSetArray{};
			}

			auto result = WriteDescriptorSet{ dstBinding
				, dstArrayElement
				, 1u
				, descriptorType };
			result.bufferInfo.push_back( { buffer
				, byteOffset
				, byteRange } );
			result.texelBufferView.push_back( view );
			m_config.descriptorWrites.value().push_back( result );
			return static_cast< BuilderT & >( *this );
		}

	protected:
		rp::Config m_config;
	};

	class RunnablePassBuilder
		: public RunnablePassBuilderT< RunnablePassBuilder >
	{
	};
}
