/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "RenderGraph/FrameGraphPrerequisites.hpp"

namespace crg
{
	struct WriteDescriptorSet
	{
		WriteDescriptorSet( uint32_t dstBinding
			, uint32_t dstArrayElement
			, uint32_t descriptorCount
			, VkDescriptorType descriptorType )
			: vk{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, nullptr, dstBinding, dstArrayElement, descriptorCount, descriptorType, nullptr, nullptr, nullptr }
			, needsUpdate{ true }
		{
		}

		WriteDescriptorSet( uint32_t dstBinding
			, uint32_t dstArrayElement
			, VkDescriptorType descriptorType
			, VkDescriptorImageInfo imageInfos )
			: imageInfo{ 1u, imageInfos }
			, vk{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, nullptr, dstBinding, dstArrayElement, uint32_t( this->imageInfo.size() ), descriptorType, nullptr, nullptr, nullptr }
			, needsUpdate{ true }
		{
		}

		WriteDescriptorSet( uint32_t dstBinding
			, uint32_t dstArrayElement
			, VkDescriptorType descriptorType
			, VkDescriptorImageInfoArray imageInfos )
			: imageInfo{ std::move( imageInfos ) }
			, vk{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, nullptr, dstBinding, dstArrayElement, uint32_t( this->imageInfo.size() ), descriptorType, nullptr, nullptr, nullptr }
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
		VkDescriptorBufferInfoArray bufferViewInfo;
		VkBufferViewArray texelBufferView;

	private:
		mutable VkWriteDescriptorSet vk;
		bool needsUpdate;
	};
}
