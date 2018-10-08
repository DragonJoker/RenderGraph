/*
See LICENSE file in root folder
*/
#ifndef ___Utils_BINARY_WRITER_H___
#define ___Utils_BINARY_WRITER_H___

#include "Writer.hpp"
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
	class BinaryWriter
		: public Writer< T, FileType::eBinary >
	{
	protected:
		using Type = typename Writer< T, FileType::eBinary >::Type;

	public:
		/**
		 *\~english
		 *\brief		Constructor
		 *\~french
		 *\brief		Constructeur
		 */
		BinaryWriter() = default;
		/**
		 *\~english
		 *\brief		Destructor
		 *\~french
		 *\brief		Destructeur
		 */
		virtual ~BinaryWriter() = default;
	};
}

#endif
