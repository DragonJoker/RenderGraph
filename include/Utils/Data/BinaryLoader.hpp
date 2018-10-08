/*
See LICENSE file in root folder
*/
#ifndef ___Utils_BINARY_LOADER_H___
#define ___Utils_BINARY_LOADER_H___

#include "Loader.hpp"
#include "BinaryFile.hpp"

namespace utils
{
	/*!
	\author		Sylvain DOREMUS
	\version	0.6.1.0
	\date		19/10/2011
	\~english
	\brief		Partial utils::Loader specialisation for binary files
	\~french
	\brief		Spécialisation partielle de utils::Loader, pour les fichiers binaires
	*/
	template< class T >
	class BinaryLoader
		: public Loader< T, FileType::eBinary >
	{
	protected:
		using Type = typename Loader< T, FileType::eBinary >::Type;

	public:
		/**
		 *\~english
		 *\brief		Constructor
		 *\~french
		 *\brief		Constructeur
		 */
		BinaryLoader() = default;
		/**
		 *\~english
		 *\brief		Destructor
		 *\~french
		 *\brief		Destructeur
		 */
		virtual ~BinaryLoader() = default;
	};
}

#endif
