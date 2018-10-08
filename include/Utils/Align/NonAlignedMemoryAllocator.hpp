/*
See LICENSE file in root folder
*/
#ifndef ___Utils_NON_ALIGNED_MEMORY_ALLOCATOR_H___
#define ___Utils_NON_ALIGNED_MEMORY_ALLOCATOR_H___

#include "Utils/UtilsPrerequisites.hpp"

namespace utils
{
	/*!
	\author		Sylvain DOREMUS
	\version	0.8.0
	\date		14/01/2016
	\~english
	\brief		Memory allocator.
	\remarks	Allocates non aligned memory.
	\~french
	\brief		Allocateur de mémoire.
	\remarks	Alloue de la mémoire non alignée.
	*/
	class NonAlignedMemoryAllocator
	{
	public:
		/**
		 *\~english
		 *\brief		Allocates memory.
		 *\return		The allocated space memory address.
		 *\~french
		 *\brief		Alloue de la mémoire.
		 *\return		L'adresse mémoire de l'espace alloué.
		 */
		static uint8_t * allocate( size_t p_size )
		{
			return new uint8_t[p_size];;
		}
		/**
		 *\~english
		 *\brief		Frees the given memory.
		 *\param[in]	p_space	The memory to free.
		 *\~french
		 *\brief		Libère la mémoire donnée.
		 *\param[in]	p_space	La mémoire à libérer.
		 */
		static void deallocate( uint8_t * p_space )noexcept
		{
			delete [] p_space;
		}
	};
}

#endif
