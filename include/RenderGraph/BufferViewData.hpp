/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "Id.hpp"

namespace crg
{
	/**
	*\brief
	*	Basic buffer view data, from which views will be created.
	*/
	struct BufferViewData
	{
		std::string name;
		BufferId buffer;
		BufferViewCreateInfo info;
		BufferViewIdArray source{};

		explicit BufferViewData( std::string name = {}
			, BufferId buffer = BufferId{}
			, BufferSubresourceRange subresourceRange = {}
			, PixelFormat format = PixelFormat::eUNDEFINED )
			: name{ std::move( name ) }
			, buffer{ std::move( buffer ) }
			, info{ format, subresourceRange }
		{
		}

	private:
		friend bool operator==( BufferViewData const & lhs, BufferViewData const & rhs )
		{
			return lhs.buffer == rhs.buffer
				&& lhs.info == rhs.info;
		}
	};

	struct VertexBuffer
	{
		explicit VertexBuffer( BufferViewId pbuffer = BufferViewId{}
			, VkVertexInputAttributeDescriptionArray pvertexAttribs = {}
			, VkVertexInputBindingDescriptionArray pvertexBindings = {} )
			: buffer{ std::move( pbuffer ) }
			, vertexAttribs{ std::move( pvertexAttribs ) }
			, vertexBindings{ std::move( pvertexBindings ) }
		{
			doUpdateState();
		}

		VertexBuffer( VertexBuffer const & rhs )
			: buffer{ rhs.buffer }
			, vertexAttribs{ rhs.vertexAttribs }
			, vertexBindings{ rhs.vertexBindings }
		{
			doUpdateState();
		}

		VertexBuffer( VertexBuffer && rhs )noexcept
			: buffer{ std::move( rhs.buffer ) }
			, vertexAttribs{ std::move( rhs.vertexAttribs ) }
			, vertexBindings{ std::move( rhs.vertexBindings ) }
		{
			doUpdateState();
		}

		VertexBuffer & operator=( VertexBuffer const & rhs )
		{
			buffer = rhs.buffer;
			vertexAttribs = rhs.vertexAttribs;
			vertexBindings = rhs.vertexBindings;
			doUpdateState();

			return *this;
		}

		VertexBuffer & operator=( VertexBuffer && rhs )noexcept
		{
			buffer = std::move( rhs.buffer );
			vertexAttribs = std::move( rhs.vertexAttribs );
			vertexBindings = std::move( rhs.vertexBindings );
			doUpdateState();

			return *this;
		}

		BufferViewId buffer;
		VkVertexInputAttributeDescriptionArray vertexAttribs;
		VkVertexInputBindingDescriptionArray vertexBindings;
		VkPipelineVertexInputStateCreateInfo inputState{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, nullptr, {}, {}, {}, {}, {} };

	private:
		void doUpdateState()
		{
			inputState.vertexAttributeDescriptionCount = uint32_t( vertexAttribs.size() );
			inputState.pVertexAttributeDescriptions = vertexAttribs.data();
			inputState.vertexBindingDescriptionCount = uint32_t( vertexBindings.size() );
			inputState.pVertexBindingDescriptions = vertexBindings.data();
		}
	};

	struct IndexBuffer
	{
		explicit IndexBuffer( BufferViewId pbuffer = BufferViewId{} )
			: buffer{ std::move( pbuffer ) }
		{
		}

		BufferViewId buffer;

	private:
		friend bool operator==( IndexBuffer const & lhs, IndexBuffer const & rhs ) = default;
	};

	struct IndirectBuffer
	{
		explicit IndirectBuffer( BufferViewId pbuffer
			, uint32_t pstride )
			: buffer{ std::move( pbuffer ) }
			, stride{ pstride }
		{
		}

		BufferViewId buffer;
		uint32_t stride;

	private:
		friend bool operator==( IndirectBuffer const & lhs, IndirectBuffer const & rhs ) = default;
	};

	template<>
	struct DefaultValueGetterT< VertexBuffer >
	{
		static VertexBuffer get()
		{
			VertexBuffer const result{};
			return result;
		}
	};

	template<>
	struct DefaultValueGetterT< IndexBuffer >
	{
		static IndexBuffer get()
		{
			IndexBuffer const result{ BufferViewId{} };
			return result;
		}
	};

	template<>
	struct DefaultValueGetterT< IndirectBuffer >
	{
		static IndirectBuffer get()
		{
			IndirectBuffer const result{ BufferViewId{}, 0u };
			return result;
		}
	};
}
