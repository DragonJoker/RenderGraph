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
	*	Basic buffer data, from which buffers will be created.
	*/
	struct BufferData
	{
		std::string name;
		BufferCreateInfo info;
		uint32_t maxPages{ 1u };
		uint32_t allocatedPages{};

		explicit BufferData( std::string pname = {}
			, BufferCreateFlags pflags = {}
			, DeviceSize psize = {}
			, uint32_t pmaxPages = 1u
			, BufferUsageFlags pusage = {}
			, MemoryPropertyFlags pmemory = MemoryPropertyFlags::eDeviceLocal )
			: name{ std::move( pname ) }
			, info{ pflags, psize, pusage, pmemory }
			, maxPages{ pmaxPages }
		{
		}

		explicit BufferData( std::string pname = {}
			, BufferCreateFlags pflags = {}
			, DeviceSize psize = {}
			, BufferUsageFlags pusage = {}
			, MemoryPropertyFlags pmemory = MemoryPropertyFlags::eDeviceLocal )
			: BufferData{ std::move( pname ), pflags, psize, 1u, pusage, pmemory }
		{
		}

	private:
		friend bool operator==( BufferData const & lhs, BufferData const & rhs ) = default;
	};
}
