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

		explicit BufferData( std::string name = {}
			, BufferCreateFlags flags = {}
			, DeviceSize size = {}
			, BufferUsageFlags usage = {}
			, MemoryPropertyFlags memory = MemoryPropertyFlags::eDeviceLocal )
			: name{ std::move( name ) }
			, info{ flags, size, usage, memory }
		{
		}

	private:
		friend bool operator==( BufferData const & lhs, BufferData const & rhs ) = default;
	};
}
