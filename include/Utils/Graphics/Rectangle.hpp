/*
See LICENSE file in root folder
*/
#ifndef ___Rectangle___
#define ___Rectangle___

#include "Utils/UtilsPrerequisites.hpp"

namespace utils
{
	//!
	/*!
	\author		Sylvain DOREMUS
	\version	0.6.1.0
	\date		03/01/2011
	\~english
	\brief		Rectangle class
	\remark		Rectangle class, inherits from Point, holds the intersection functions and specific accessors
	\~french
	\brief		Classe représentant un rectangle
	\remark		Dérive de Point, gère les intersections entre rectangles et les accesseurs spécifiques
	*/
	struct Rectangle
	{
		Position position;
		Size size;
	};
	/**
	*\~english
	*\brief		Test if the givent point is onto or into this rectangle
	*\param[in]	p_point	The point to test
	*\return	\p Intersection::eIn if onto or into, \p Intersection::eOut if not
	*\~french
	*\brief		Teste si le point donné est sur ou dans ce rectangle
	*\param[in]	p_point	Le point à tester
	*\return	\p Intersection::eIn si sur ou dedans, \p Intersection::eOut sinon
	*/
	Intersection getIntersection( Rectangle const & lhs, Position const & rhs );
	/**
	*\~english
	*\brief		Test if the givent rectangle intersects this rectangle
	*\param[in]	p_rcRect	The rectangle to test
	*\return	The intersection type between the 2 rectangles
	*\~french
	*\brief		Teste si le rectangle donné intersecte ce rectangle
	*\param[in]	p_rcRect	Le rectangle à tester
	*\return	Le type d'intersection entre les 2 rectangles
	*/
	Intersection getIntersection( Rectangle const & lhs, Rectangle const & rhs );
	/**
	 *\~english
	 *name Comparison operators.
	 *\~french
	 *name Opérateurs de comparaison.
	**/
	/**@{*/
	inline bool operator==( Rectangle const & lhs, Rectangle const & rhs )
	{
		return lhs.position.x == rhs.position.x
			&& lhs.position.y == rhs.position.y
			&& lhs.size.width == rhs.size.width
			&& lhs.size.height == rhs.size.height;
	}

	inline bool operator!=( Rectangle const & lhs, Rectangle const & rhs )
	{
		return !( lhs == rhs );
	}
	/**@}*/
}

#endif
