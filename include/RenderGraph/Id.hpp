/*
This file belongs to RenderGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "RenderGraph/RenderGraphPrerequisites.hpp"

namespace crg
{
	template< typename TypeT >
	struct Id
	{
		uint32_t id;
		TypeT const * data;
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
	inline bool operator==( std::vector< Id< TypeT > > const & lhs
		, std::vector< Id< TypeT > > const & rhs )
	{
		auto result = lhs.size() == rhs.size();

		if ( result )
		{
			for ( size_t i = 0u; result && i < lhs.size(); ++i )
			{
				result = lhs[i] == rhs[i];
			}
		}

		return result;
	}
}
