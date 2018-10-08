/*
See LICENSE file in root folder
*/
#ifndef ___Utils_IMAGE_CACHE_H___
#define ___Utils_IMAGE_CACHE_H___

#include "Utils/Design/Collection.hpp"

namespace utils
{
	/*!
	\author		Sylvain DOREMUS
	\version	0.8.0
	\date		15/10/2015
	\~english
	\brief		Image manager.
	\remark		Holds the loaded images, and helps their loading.
	\~french
	\brief		Gestionnaire d'images.
	\remark		Détient les images, et simplifie leur création.
	*/
	class ImageCache
		: private Collection< Image, String >
	{
	public:
		/**
		 *\~english
		 *\brief		Constructor.
		 *\~french
		 *\brief		Constructeur.
		 */
		ImageCache();
		/**
		 *\~english
		 *\brief		Destructor.
		 *\~french
		 *\brief		Destructeur.
		 */
		~ImageCache();
		/**
		 *\~english
		 *\brief		Creates an image.
		 *\remarks		If the image already exists, it is returned.
		 *\param[in]	p_name		The font name.
		 *\param[in]	p_path		The full access path to the image file.
		 *\return		The created (or retrieved) image.
		 *\~french
		 *\brief		Crée une image.
		 *\remarks		Si la police existe déjà, elle est retournée.
		 *\param[in]	p_name		Le nom de la police.
		 *\param[in]	p_path		Le chemin complet d'accès au fichier de l'image.
		 *\return		L'image créée (ou récupérée).
		 */
		ImageSPtr add( String const & name
			, Path const & path );
		/**
		 *\~english
		 *\brief		Creates the image with given params.
		 *\remarks		If the image already exists, it is returned.
		 *\param[in]	name	The image name.
		 *\param[in]	size	The wanted image dimensions.
		 *\param[in]	format	The wanted image pixel format.
		 *\~french
		 *\brief		Crée l'image avec les paramètres donnés.
		 *\param[in]	name	Le nom de l'image.
		 *\param[in]	size	Les dimensions voulues pour l'image.
		 *\param[in]	format	Le format de pixel voulu pour l'image.
		 */
		ImageSPtr add( String const & name
			, Size const & size, ashes::Format format );

	public:
		using Collection< Image, String >::begin;
		using Collection< Image, String >::end;
		using Collection< Image, String >::lock;
		using Collection< Image, String >::unlock;
		using Collection< Image, String >::clear;
		using Collection< Image, String >::find;
		using Collection< Image, String >::has;
	};
}

#endif

