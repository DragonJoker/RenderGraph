#include "Utils/Graphics/Rectangle.hpp"

#include "Utils/Math/Point.hpp"

namespace utils
{
	Intersection getIntersection( Rectangle const & lhs, Position const & rhs )
	{
		Intersection result = Intersection::eOut;

		if ( ( rhs.x >= lhs.position.x )
			&& ( rhs.y >= lhs.position.y )
			&& ( rhs.x <= int32_t( lhs.position.x + lhs.size.width ) )
			&& ( rhs.y <= int32_t( lhs.position.y + lhs.size.height ) ) )
		{
			result = Intersection::eIn;
		}

		return result;
	}

	Intersection getIntersection( Rectangle const & lhs, Rectangle const & rhs )
	{
		// Calcul du rectangle d'intersection
		Point2i start( std::max( lhs.position.x, rhs.position.x ), std::max( lhs.position.y, rhs.position.y ) );
		Point2i end( std::min( lhs.position.x + lhs.size.width, rhs.position.x + rhs.size.width ), std::min( lhs.position.y + lhs.size.height, rhs.position.y + rhs.size.height ) );
		Rectangle rcOverlap{ { start[0], start[1] }, { uint32_t( end[0] ), uint32_t( end[1] ) } };
		Intersection result = Intersection::eIntersect;

		if ( ( start[0] > int32_t( lhs.position.x + lhs.size.width ) )
			|| ( start[1] > int32_t( lhs.position.y + lhs.size.height ) ) )
		{
			result = Intersection::eOut;
		}
		else if ( ( rcOverlap == lhs ) || ( rcOverlap == rhs ) )
		{
			result = Intersection::eIn;
		}

		return result;
	}
}
