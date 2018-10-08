/*
See LICENSE file in root folder
*/
#ifndef ___Utils_UTILS_H___
#define ___Utils_UTILS_H___

#include "Utils/UtilsPrerequisites.hpp"
#include "Utils/Miscellaneous/StringUtils.hpp"
#include "Utils/Design/Templates.hpp"

#include <cstdarg>

namespace utils
{
	Point3r operator *( Matrix4x4r const & p_mtx, Point3r const & p_pt );
	Point3r operator *( Point3r const & p_pt, Matrix4x4r const & p_mtx );
	Point4r operator *( Matrix4x4r const & p_mtx, Point4r const & p_pt );
	Point4r operator *( Point4r const & p_pt, Matrix4x4r const & p_mtx );
	uint32_t getNext2Pow( uint32_t p_uiDim );
}

#endif
