/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "RenderGraph/FrameGraphPrerequisites.hpp"

namespace crg
{
	template< typename TypeT >
	struct Id
	{
		friend class Attachment;
		friend class FrameGraph;
		friend class ImageViewData;
		friend class ImageData;

		uint32_t id;
		TypeT const * data;

		Id( uint32_t id = 0u
			, TypeT const * data = nullptr )
			: id{ id }
			, data{ data }
		{
		}
	};

	template< typename TypeT >
	inline bool operator<( Id< TypeT > const & lhs, Id< TypeT > const & rhs )
	{
		return lhs.id < rhs.id;
	}

	template< typename TypeT >
	inline bool operator>( Id< TypeT > const & lhs, Id< TypeT > const & rhs )
	{
		return lhs.id > rhs.id;
	}

	template< typename TypeT >
	inline bool operator==( Id< TypeT > const & lhs, Id< TypeT > const & rhs )
	{
		return lhs.id == rhs.id;
	}

	template< typename TypeT >
	inline bool operator!=( Id< TypeT > const & lhs, Id< TypeT > const & rhs )
	{
		return lhs.id != rhs.id;
	}
}
