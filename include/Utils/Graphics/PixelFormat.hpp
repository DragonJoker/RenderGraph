/*
See LICENSE file in root folder
*/
#ifndef ___Utils_PixelFormat___
#define ___Utils_PixelFormat___

#include "Utils/UtilsPrerequisites.hpp"

#include "Utils/Exception/Exception.hpp"

#include <vector>
#include <algorithm>
#include <numeric>

namespace utils
{
	/*!
	\~english
	\brief Pixel PixelComponents enumeration
	\~french
	\brief Enumération des composantes de Pixel
	*/
	enum class PixelComponent
		: uint8_t
	{
		//!\~english Red	\~french Rouge
		eRed,
		//!\~english Green	\~french Vert
		eGreen,
		//!\~english Blue	\~french Bleu
		eBlue,
		//!\~english Alpha	\~french Alpha
		eAlpha,
		//!\~english Depth	\~french Profondeur
		eDepth,
		//!\~english Stencil	\~french Stencil
		eStencil,
		Utils_ScopedEnumBounds( eRed )
	};
	/*!
	\author 	Sylvain DOREMUS
	\date 		27/05/2013
	\~english
	\brief		Holds colour/depth/stencil PixelComponents helper functions
	\~french
	\brief		Contient des fonctions d'aides à la conversion de composantes couleur/profondeur/stencil
	*/
	template< ashes::Format PF > struct PixelComponents;
	/**
	 *\~english
	 *\brief		Helper struct that holds pixel size, toString and converion functions
	 *\~french
	 *\brief		Structure d'aide contenant la taille d'un pixel ainsi que sa fonction toString et les fonction de conversion vers d'autres formats
	 */
	template< ashes::Format format > struct PixelDefinitions;
	/**
	 *\~english
	 *\brief		Helper struct to tell if a pixel format represents a colour
	 *\~french
	 *\brief		Structure d'aide permettant de dire si un format de pixels représente une couleur
	 */
	template< ashes::Format PF > struct IsColourFormat : public std::true_type {};

	template<> struct IsColourFormat< ashes::Format::eD16_UNORM > : public std::false_type {};
	template<> struct IsColourFormat< ashes::Format::eD24_UNORM_S8_UINT > : public std::false_type {};
	template<> struct IsColourFormat< ashes::Format::eD32_SFLOAT > : public std::false_type {};
	template<> struct IsColourFormat< ashes::Format::eD32_SFLOAT_S8_UINT > : public std::false_type {};
	template<> struct IsColourFormat< ashes::Format::eS8_UINT > : public std::false_type {};
	/**
	 *\~english
	 *\brief		Helper struct to tell if a pixel format represents a depth pixel
	 *\~french
	 *\brief		Structure d'aide permettant de dire si un format de pixels représente un pixel de profondeur
	 */
	template< ashes::Format PF > struct IsDepthFormat : public std::false_type {};

	template<> struct IsDepthFormat< ashes::Format::eD16_UNORM > : public std::true_type {};
	template<> struct IsDepthFormat< ashes::Format::eD24_UNORM_S8_UINT > : public std::true_type {};
	template<> struct IsDepthFormat< ashes::Format::eD32_SFLOAT > : public std::true_type {};
	template<> struct IsDepthFormat< ashes::Format::eD32_SFLOAT_S8_UINT > : public std::true_type {};
	/**
	 *\~english
	 *\brief		Helper struct to tell if a pixel format represents a stencil pixel
	 *\~french
	 *\brief		Structure d'aide permettant de dire si un format de pixels représente un pixel de stencil
	 */
	template< ashes::Format PF > struct IsStencilFormat : public std::false_type {};

	template<> struct IsStencilFormat< ashes::Format::eD24_UNORM_S8_UINT > : public std::true_type {};
	template<> struct IsStencilFormat< ashes::Format::eD32_SFLOAT_S8_UINT > : public std::true_type {};
	template<> struct IsStencilFormat< ashes::Format::eS8_UINT > : public std::true_type {};
	/**
	 *\~english
	 *\brief		Helper struct to tell if a pixel format represents a depth or stencil pixel
	 *\~french
	 *\brief		Structure d'aide permettant de dire si un format de pixels représente un pixel de profondeur ou de stencil
	 */
	template< ashes::Format PF > struct IsDepthStencilFormat : public std::false_type {};

	template<> struct IsDepthStencilFormat< ashes::Format::eD16_UNORM > : public std::true_type {};
	template<> struct IsDepthStencilFormat< ashes::Format::eD24_UNORM_S8_UINT > : public std::true_type {};
	template<> struct IsDepthStencilFormat< ashes::Format::eD32_SFLOAT > : public std::true_type {};
	template<> struct IsDepthStencilFormat< ashes::Format::eD32_SFLOAT_S8_UINT > : public std::true_type {};
	template<> struct IsDepthStencilFormat< ashes::Format::eS8_UINT > : public std::true_type {};

	namespace PF
	{
		/**
		 *\~english
		 *\brief		Function to retrieve Pixel size without templates
		 *\param[in]	format	The pixel format
		 *\~french
		 *\brief		Fonction de récuperation de la taille d'un pixel sans templates
		 *\param[in]	format	Le format de pixels
		 */
		uint8_t getBytesPerPixel( ashes::Format format );
		/**
		 *\~english
		 *\brief		Function to retrieve the number of components of a pixel format.
		 *\param[in]	format	The pixel format
		 *\~french
		 *\brief		Fonction de récuperation du nombre de composantes d'un format de pixel.
		 *\param[in]	format	Le format de pixels
		 */
		uint8_t getComponentsCount( ashes::Format format );
		/**
		 *\~english
		 *\brief		Function to retrieve pixel format from a name
		 *\param[in]	name	The pixel format name
		 *\~french
		 *\brief		Fonction de récuperation d'un format de pixel par son nom
		 *\param[in]	name	Le nom du format de pixels
		 */
		ashes::Format getFormatByName( String const & name );
		/**
		 *\~english
		 *\brief		Function to retrieve pixel format name
		 *\param[in]	format	The pixel format
		 *\~french
		 *\brief		Fonction de récuperation du nom d'un format de pixel
		 *\param[in]	format	Le format de pixels
		 */
		String getFormatName( ashes::Format format );
		/**
		 *\~english
		 *\brief		Function to retrieve pixel colour PixelComponents in float
		 *\param[in]	pixel		The pixel
		 *\param[in]	component	The PixelComponents
		 *\return		The PixelComponents value
		 *\~french
		 *\brief		Fonction de récupération d'une composante couleur d'un pixel, en flottant
		 *\param[in]	pixel		Le pixel
		 *\param[in]	component	La composante
		 *\return		La valeur de la composante
		 */
		template< ashes::Format PF >
		float getFloatComponent( Pixel< PF > const & pixel, PixelComponent component );
		/**
		 *\~english
		 *\brief		Function to define pixel colour PixelComponents in float
		 *\param[in]	pixel		The pixel
		 *\param[in]	component	The PixelComponents
		 *\param[in]	value		The PixelComponents value
		 *\~french
		 *\brief		Fonction de définition d'une composante couleur d'un pixel, en flottant
		 *\param[in]	pixel		Le pixel
		 *\param[in]	component	La composante
		 *\param[in]	value		La valeur de la composante
		 */
		template< ashes::Format PF >
		void setFloatComponent( Pixel< PF > & pixel, PixelComponent component, float value );
		/**
		 *\~english
		 *\brief		Function to retrieve pixel colour PixelComponents in byte
		 *\param[in]	pixel		The pixel
		 *\param[in]	component	The PixelComponents
		 *\return		The PixelComponents value
		 *\~french
		 *\brief		Fonction de récupération d'une composante couleur d'un pixel, en octet
		 *\param[in]	pixel		Le pixel
		 *\param[in]	component	La composante
		 *\return		La valeur de la composante
		 */
		template< ashes::Format PF >
		uint8_t getByteComponent( Pixel< PF > const & pixel, PixelComponent component );
		/**
		 *\~english
		 *\brief		Function to define pixel colour PixelComponents in byte
		 *\param[in]	pixel		The pixel
		 *\param[in]	component	The PixelComponents
		 *\param[in]	value		The PixelComponents value
		 *\~french
		 *\brief		Fonction de définition d'une composante couleur d'un pixel, en octet
		 *\param[in]	pixel		Le pixel
		 *\param[in]	component	La composante
		 *\param[in]	value		La valeur de la composante
		 */
		template< ashes::Format PF >
		void setByteComponent( Pixel< PF > & pixel, PixelComponent component, uint8_t value );
		/**
		 *\~english
		 *\brief		Function to retrieve pixel colour PixelComponents in uint16_t
		 *\param[in]	pixel		The pixel
		 *\param[in]	component	The PixelComponents
		 *\return		The PixelComponents value
		 *\~french
		 *\brief		Fonction de récupération d'une composante couleur d'un pixel, en uint16_t
		 *\param[in]	pixel		Le pixel
		 *\param[in]	component	La composante
		 *\return		La valeur de la composante
		 */
		template< ashes::Format PF >
		uint16_t getUInt16Component( Pixel< PF > const & pixel, PixelComponent component );
		/**
		 *\~english
		 *\brief		Function to define pixel colour PixelComponents in uint16_t
		 *\param[in]	pixel		The pixel
		 *\param[in]	component	The PixelComponents
		 *\param[in]	value		The PixelComponents value
		 *\~french
		 *\brief		Fonction de définition d'une composante couleur d'un pixel, en uint16_t
		 *\param[in]	pixel		Le pixel
		 *\param[in]	component	La composante
		 *\param[in]	value		La valeur de la composante
		 */
		template< ashes::Format PF >
		void setUInt16Component( Pixel< PF > & pixel, PixelComponent component, uint16_t value );
		/**
		 *\~english
		 *\brief		Function to retrieve pixel colour PixelComponents in uint32_t, with 24 relevant bits
		 *\param[in]	pixel		The pixel
		 *\param[in]	component	The PixelComponents
		 *\return		The PixelComponents value
		 *\~french
		 *\brief		Fonction de récupération d'une composante couleur d'un pixel, en uint32_t, avec 24 bits utiles
		 *\param[in]	pixel		Le pixel
		 *\param[in]	component	La composante
		 *\return		La valeur de la composante
		 */
		template< ashes::Format PF >
		uint32_t getUInt24Component( Pixel< PF > const & pixel, PixelComponent component );
		/**
		 *\~english
		 *\brief		Function to define pixel colour PixelComponents in uint32_t, with 24 relevant bits
		 *\param[in]	pixel		The pixel
		 *\param[in]	component	The PixelComponents
		 *\param[in]	value		The PixelComponents value
		 *\~french
		 *\brief		Fonction de définition d'une composante couleur d'un pixel, en uint32_t, avec 24 bits utiles
		 *\param[in]	pixel		Le pixel
		 *\param[in]	component	La composante
		 *\param[in]	value		La valeur de la composante
		 */
		template< ashes::Format PF >
		void setUInt24Component( Pixel< PF > & pixel, PixelComponent component, uint32_t value );
		/**
		 *\~english
		 *\brief		Function to retrieve pixel colour PixelComponents in uint32_t
		 *\param[in]	pixel		The pixel
		 *\param[in]	component	The PixelComponents
		 *\return		The PixelComponents value
		 *\~french
		 *\brief		Fonction de récupération d'une composante couleur d'un pixel, en uint32_t
		 *\param[in]	pixel		Le pixel
		 *\param[in]	component	La composante
		 *\return		La valeur de la composante
		 */
		template< ashes::Format PF >
		uint32_t getUInt32Component( Pixel< PF > const & pixel, PixelComponent component );
		/**
		 *\~english
		 *\brief		Function to define pixel colour PixelComponents in uint32_t
		 *\param[in]	pixel		The pixel
		 *\param[in]	component	The PixelComponents
		 *\param[in]	value		The PixelComponents value
		 *\~french
		 *\brief		Fonction de définition d'une composante couleur d'un pixel, en uint32_t
		 *\param[in]	pixel		Le pixel
		 *\param[in]	component	La composante
		 *\param[in]	value		La valeur de la composante
		 */
		template< ashes::Format PF >
		void setUInt32Component( Pixel< PF > & pixel, PixelComponent component, uint32_t value );
		/**
		 *\~english
		 *\brief		Function to retrieve pixel colour PixelComponents in float
		 *\param[in]	pixel		The pixel
		 *\return		The PixelComponents value
		 *\~french
		 *\brief		Fonction de récupération de la composante rouge d'un pixel, en flottant
		 *\param[in]	pixel		Le pixel
		 *\return		La valeur de la composante
		 */
		template< ashes::Format PF >
		float getFloatRed( Pixel< PF > const & pixel )
		{
			return getFloatComponent( pixel, PixelComponent::eRed );
		}
		/**
		 *\~english
		 *\brief		Function to define pixel colour PixelComponents in float
		 *\param[in]	pixel		The pixel
		 *\param[in]	value		The PixelComponents value
		 *\~french
		 *\brief		Fonction de définition de la composante rouge d'un pixel, en flottant
		 *\param[in]	pixel		Le pixel
		 *\param[in]	value		La valeur de la composante
		 */
		template< ashes::Format PF >
		void setFloatRed( Pixel< PF > const & pixel, float value )
		{
			return setFloatComponent( pixel, PixelComponent::eRed, value );
		}
		/**
		 *\~english
		 *\brief		Function to retrieve pixel red PixelComponents in byte
		 *\param[in]	pixel		The pixel
		 *\return		The PixelComponents value
		 *\~french
		 *\brief		Fonction de récupération de la composante rouge d'un pixel, en octet
		 *\param[in]	pixel		Le pixel
		 *\return		La valeur de la composante
		 */
		template< ashes::Format PF >
		uint8_t getByteRed( Pixel< PF > const & pixel )
		{
			return getByteComponent( pixel, PixelComponent::eRed );
		}
		/**
		 *\~english
		 *\brief		Function to define pixel red PixelComponents in byte
		 *\param[in]	pixel		The pixel
		 *\param[in]	value		The PixelComponents value
		 *\~french
		 *\brief		Fonction de définition de la composante rouge d'un pixel, en octet
		 *\param[in]	pixel		Le pixel
		 *\param[in]	value		La valeur de la composante
		 */
		template< ashes::Format PF >
		void setByteRed( Pixel< PF > & pixel, uint8_t value )
		{
			return setByteComponent( pixel, PixelComponent::eRed, value );
		}
		/**
		 *\~english
		 *\brief		Function to retrieve pixel green PixelComponents in float
		 *\param[in]	pixel		The pixel
		 *\return		The PixelComponents value
		 *\~french
		 *\brief		Fonction de récupération de la composante verte d'un pixel, en flottant
		 *\param[in]	pixel		Le pixel
		 *\return		La valeur de la composante
		 */
		template< ashes::Format PF >
		float getFloatGreen( Pixel< PF > const & pixel )
		{
			return getFloatComponent( pixel, PixelComponent::eGreen );
		}
		/**
		 *\~english
		 *\brief		Function to define pixel green PixelComponents in float
		 *\param[in]	pixel		The pixel
		 *\param[in]	value		The PixelComponents value
		 *\~french
		 *\brief		Fonction de définition de la composante verte d'un pixel, en flottant
		 *\param[in]	pixel		Le pixel
		 *\param[in]	value		La valeur de la composante
		 */
		template< ashes::Format PF >
		void setFloatGreen( Pixel< PF > & pixel, float value )
		{
			return setFloatComponent( pixel, PixelComponent::eGreen, value );
		}
		/**
		 *\~english
		 *\brief		Function to retrieve pixel green PixelComponents in byte
		 *\param[in]	pixel		The pixel
		 *\return		The PixelComponents value
		 *\~french
		 *\brief		Fonction de récupération de la composante verte d'un pixel, en octet
		 *\param[in]	pixel		Le pixel
		 *\return		La valeur de la composante
		 */
		template< ashes::Format PF >
		uint8_t getByteGreen( Pixel< PF > const & pixel )
		{
			return getByteComponent( pixel, PixelComponent::eGreen );
		}
		/**
		 *\~english
		 *\brief		Function to define pixel green PixelComponents in byte
		 *\param[in]	pixel		The pixel
		 *\param[in]	value		The PixelComponents value
		 *\~french
		 *\brief		Fonction de définition de la composante verte d'un pixel, en octet
		 *\param[in]	pixel		Le pixel
		 *\param[in]	value		La valeur de la composante
		 */
		template< ashes::Format PF >
		void setByteGreen( Pixel< PF > & pixel, uint8_t value )
		{
			return setByteComponent( pixel, PixelComponent::eGreen, value );
		}
		/**
		 *\~english
		 *\brief		Function to retrieve pixel blue PixelComponents in float
		 *\param[in]	pixel		The pixel
		 *\return		The PixelComponents value
		 *\~french
		 *\brief		Fonction de récupération de la composante bleue d'un pixel, en flottant
		 *\param[in]	pixel		Le pixel
		 *\return		La valeur de la composante
		 */
		template< ashes::Format PF >
		float getFloatBlue( Pixel< PF > const & pixel )
		{
			return getFloatComponent( pixel, PixelComponent::eBlue );
		}
		/**
		 *\~english
		 *\brief		Function to define pixel blue PixelComponents in float
		 *\param[in]	pixel		The pixel
		 *\param[in]	value		The PixelComponents value
		 *\~french
		 *\brief		Fonction de définition de la composante bleue d'un pixel, en flottant
		 *\param[in]	pixel		Le pixel
		 *\param[in]	value		La valeur de la composante
		 */
		template< ashes::Format PF >
		void setFloatBlue( Pixel< PF > & pixel, float value )
		{
			return setFloatComponent( pixel, PixelComponent::eBlue, value );
		}
		/**
		 *\~english
		 *\brief		Function to retrieve pixel blue PixelComponents in byte
		 *\param[in]	pixel		The pixel
		 *\return		The PixelComponents value
		 *\~french
		 *\brief		Fonction de récupération de la composante bleue d'un pixel, en octet
		 *\param[in]	pixel		Le pixel
		 *\return		La valeur de la composante
		 */
		template< ashes::Format PF >
		uint8_t getByteBlue( Pixel< PF > const & pixel )
		{
			return getByteComponent( pixel, PixelComponent::eBlue );
		}
		/**
		 *\~english
		 *\brief		Function to define pixel blue PixelComponents in byte
		 *\param[in]	pixel		The pixel
		 *\param[in]	value		The PixelComponents value
		 *\~french
		 *\brief		Fonction de définition de la composante bleue d'un pixel, en octet
		 *\param[in]	pixel		Le pixel
		 *\param[in]	value		La valeur de la composante
		 */
		template< ashes::Format PF >
		void setByteBlue( Pixel< PF > & pixel, uint8_t value )
		{
			return setByteComponent( pixel, PixelComponent::eBlue, value );
		}
		/**
		 *\~english
		 *\brief		Function to retrieve pixel alpha PixelComponents in float
		 *\param[in]	pixel		The pixel
		 *\return		The PixelComponents value
		 *\~french
		 *\brief		Fonction de récupération de la composante alpha d'un pixel, en flottant
		 *\param[in]	pixel		Le pixel
		 *\return		La valeur de la composante
		 */
		template< ashes::Format PF >
		float getFloatAlpha( Pixel< PF > const & pixel )
		{
			return getFloatComponent( pixel, PixelComponent::eAlpha );
		}
		/**
		 *\~english
		 *\brief		Function to define pixel alpha PixelComponents in float
		 *\param[in]	pixel		The pixel
		 *\param[in]	value		The PixelComponents value
		 *\~french
		 *\brief		Fonction de définition de la composante alpha d'un pixel, en flottant
		 *\param[in]	pixel		Le pixel
		 *\param[in]	value		La valeur de la composante
		 */
		template< ashes::Format PF >
		void setFloatAlpha( Pixel< PF > & pixel, float value )
		{
			return setFloatComponent( pixel, PixelComponent::eAlpha, value );
		}
		/**
		 *\~english
		 *\brief		Function to retrieve pixel alpha PixelComponents in byte
		 *\param[in]	pixel		The pixel
		 *\return		The PixelComponents value
		 *\~french
		 *\brief		Fonction de récupération de la composante alpha d'un pixel, en octet
		 *\param[in]	pixel		Le pixel
		 *\return		La valeur de la composante
		 */
		template< ashes::Format PF >
		uint8_t getByteAlpha( Pixel< PF > const & pixel )
		{
			return getByteComponent( pixel, PixelComponent::eAlpha );
		}
		/**
		 *\~english
		 *\brief		Function to define pixel alpha PixelComponents in byte
		 *\param[in]	pixel		The pixel
		 *\param[in]	value		The PixelComponents value
		 *\~french
		 *\brief		Fonction de définition de la composante alpha d'un pixel, en octet
		 *\param[in]	pixel		Le pixel
		 *\param[in]	value		La valeur de la composante
		 */
		template< ashes::Format PF >
		void setByteAlpha( Pixel< PF > & pixel, uint8_t value )
		{
			return setByteComponent( pixel, PixelComponent::eAlpha, value );
		}
		/**
		 *\~english
		 *\brief		Function to retrieve pixel depth PixelComponents in float
		 *\param[in]	pixel		The pixel
		 *\return		The PixelComponents value
		 *\~french
		 *\brief		Fonction de récupération de la composante profondeur d'un pixel, en flottant
		 *\param[in]	pixel		Le pixel
		 *\return		La valeur de la composante
		 */
		template< ashes::Format PF >
		float getFloatDepth( Pixel< PF > const & pixel )
		{
			return getFloatComponent( pixel, PixelComponent::eDepth );
		}
		/**
		 *\~english
		 *\brief		Function to define pixel depth PixelComponents in float
		 *\param[in]	pixel		The pixel
		 *\param[in]	value		The PixelComponents value
		 *\~french
		 *\brief		Fonction de définition de la composante profondeur d'un pixel, en flottant
		 *\param[in]	pixel		Le pixel
		 *\param[in]	value		La valeur de la composante
		 */
		template< ashes::Format PF >
		void setFloatDepth( Pixel< PF > & pixel, float value )
		{
			return setFloatComponent( pixel, PixelComponent::eDepth, value );
		}
		/**
		 *\~english
		 *\brief		Function to retrieve pixel depth PixelComponents in byte
		 *\param[in]	pixel		The pixel
		 *\return		The PixelComponents value
		 *\~french
		 *\brief		Fonction de récupération de la composante profondeur d'un pixel, en octet
		 *\param[in]	pixel		Le pixel
		 *\return		La valeur de la composante
		 */
		template< ashes::Format PF >
		uint8_t getByteDepth( Pixel< PF > const & pixel )
		{
			return getByteComponent( pixel, PixelComponent::eDepth );
		}
		/**
		 *\~english
		 *\brief		Function to define pixel depth PixelComponents in byte
		 *\param[in]	pixel		The pixel
		 *\param[in]	value		The PixelComponents value
		 *\~french
		 *\brief		Fonction de définition de la composante profondeur d'un pixel, en octet
		 *\param[in]	pixel		Le pixel
		 *\param[in]	value		La valeur de la composante
		 */
		template< ashes::Format PF >
		void setByteDepth( Pixel< PF > & pixel, uint8_t value )
		{
			return setByteComponent( pixel, PixelComponent::eDepth, value );
		}
		/**
		 *\~english
		 *\brief		Function to retrieve pixel depth PixelComponents in uint16_t
		 *\param[in]	pixel		The pixel
		 *\return		The PixelComponents value
		 *\~french
		 *\brief		Fonction de récupération de la composante profondeur d'un pixel, en uint16_t
		 *\param[in]	pixel		Le pixel
		 *\return		La valeur de la composante
		 */
		template< ashes::Format PF >
		uint16_t getUInt16Depth( Pixel< PF > const & pixel )
		{
			return getUInt16Component( pixel, PixelComponent::eDepth );
		}
		/**
		 *\~english
		 *\brief		Function to define pixel depth PixelComponents in uint16_t
		 *\param[in]	pixel		The pixel
		 *\param[in]	value		The PixelComponents value
		 *\~french
		 *\brief		Fonction de définition de la composante profondeur d'un pixel, en uint16_t
		 *\param[in]	pixel		Le pixel
		 *\param[in]	value		La valeur de la composante
		 */
		template< ashes::Format PF >
		void setUInt16Depth( Pixel< PF > & pixel, uint16_t value )
		{
			return setUInt16Component( pixel, PixelComponent::eDepth, value );
		}
		/**
		 *\~english
		 *\brief		Function to retrieve pixel depth PixelComponents in uint32_t, with 24 relevant bits
		 *\param[in]	pixel		The pixel
		 *\return		The PixelComponents value
		 *\~french
		 *\brief		Fonction de récupération de la composante profondeur d'un pixel, en uint32_t, avec 24 bits utiles
		 *\param[in]	pixel		Le pixel
		 *\return		La valeur de la composante
		 */
		template< ashes::Format PF >
		uint32_t getUInt24Depth( Pixel< PF > const & pixel )
		{
			return getUInt24Component( pixel, PixelComponent::eDepth );
		}
		/**
		 *\~english
		 *\brief		Function to define pixel depth PixelComponents in uint32_t, with 24 relevant bits
		 *\param[in]	pixel		The pixel
		 *\param[in]	value		The PixelComponents value
		 *\~french
		 *\brief		Fonction de définition de la composante profondeur d'un pixel, en uint32_t, avec 24 bits utiles
		 *\param[in]	pixel		Le pixel
		 *\param[in]	value		La valeur de la composante
		 */
		template< ashes::Format PF >
		void setUInt24Depth( Pixel< PF > & pixel, uint32_t value )
		{
			return setUInt24Component( pixel, PixelComponent::eDepth, value );
		}
		/**
		 *\~english
		 *\brief		Function to retrieve pixel depth PixelComponents in uint32_t
		 *\param[in]	pixel		The pixel
		 *\return		The PixelComponents value
		 *\~french
		 *\brief		Fonction de récupération de la composante profondeur d'un pixel, en uint32_t
		 *\param[in]	pixel		Le pixel
		 *\return		La valeur de la composante
		 */
		template< ashes::Format PF >
		uint32_t getUInt32Depth( Pixel< PF > const & pixel )
		{
			return getUInt32Component( pixel, PixelComponent::eDepth );
		}
		/**
		 *\~english
		 *\brief		Function to define pixel depth PixelComponents in uint32_t
		 *\param[in]	pixel		The pixel
		 *\param[in]	value		The PixelComponents value
		 *\~french
		 *\brief		Fonction de définition de la composante profondeur d'un pixel, en uint32_t
		 *\param[in]	pixel		Le pixel
		 *\param[in]	value		La valeur de la composante
		 */
		template< ashes::Format PF >
		void setUInt32Depth( Pixel< PF > & pixel, uint32_t value )
		{
			return setUInt32Component( pixel, PixelComponent::eDepth, value );
		}
		/**
		 *\~english
		 *\brief		Function to retrieve pixel depth stencil in float
		 *\param[in]	pixel		The pixel
		 *\return		The PixelComponents value
		 *\~french
		 *\brief		Fonction de récupération de la composante stencil d'un pixel, en flottant
		 *\param[in]	pixel		Le pixel
		 *\return		La valeur de la composante
		 */
		template< ashes::Format PF >
		float getFloatStencil( Pixel< PF > const & pixel )
		{
			return getFloatComponent( pixel, PixelComponent::eStencil );
		}
		/**
		 *\~english
		 *\brief		Function to define pixel depth stencil in float
		 *\param[in]	pixel		The pixel
		 *\param[in]	value		The PixelComponents value
		 *\~french
		 *\brief		Fonction de définition de la composante stencil d'un pixel, en flottant
		 *\param[in]	pixel		Le pixel
		 *\param[in]	value		La valeur de la composante
		 */
		template< ashes::Format PF >
		void setFloatStencil( Pixel< PF > & pixel, float value )
		{
			return setFloatComponent( pixel, PixelComponent::eStencil, value );
		}
		/**
		 *\~english
		 *\brief		Function to retrieve pixel depth stencil in byte
		 *\param[in]	pixel		The pixel
		 *\return		The PixelComponents value
		 *\~french
		 *\brief		Fonction de récupération de la composante stencil d'un pixel, en octet
		 *\param[in]	pixel		Le pixel
		 *\return		La valeur de la composante
		 */
		template< ashes::Format PF >
		uint8_t getByteStencil( Pixel< PF > const & pixel )
		{
			return getByteComponent( pixel, PixelComponent::eStencil );
		}
		/**
		 *\~english
		 *\brief		Function to define pixel depth stencil in byte
		 *\param[in]	pixel		The pixel
		 *\param[in]	value		The PixelComponents value
		 *\~french
		 *\brief		Fonction de définition de la composante stencil d'un pixel, en octet
		 *\param[in]	pixel		Le pixel
		 *\param[in]	value		La valeur de la composante
		 */
		template< ashes::Format PF >
		void setByteStencil( Pixel< PF > & pixel, uint8_t value )
		{
			return setByteComponent( pixel, PixelComponent::eStencil, value );
		}
		/**
		 *\~english
		 *\brief		Function to perform convertion without templates
		 *\param[in]		srcFormat	The source format
		 *\param[in,out]	srcBuffer	The source pixel
		 *\param[in]		dstFormat	The destination format
		 *\param[in,out]	dstBuffer	The destination pixel
		 *\~french
		 *\brief		Fonction de conversion sans templates
		 *\param[in]		srcFormat	Le format de la source
		 *\param[in,out]	srcBuffer	Le pixel source
		 *\param[in]		dstFormat	Le format de la destination
		 *\param[in,out]	dstBuffer	Le pixel destination
		 */
		void convertPixel( ashes::Format srcFormat
			, uint8_t const *& srcBuffer
			, ashes::Format dstFormat
			, uint8_t *& dstBuffer );
		/**
		 *\~english
		 *\brief		Function to perform convertion without templates
		 *\param[in]	srcFormat	The source format
		 *\param[in]	srcBuffer	The source buffer
		 *\param[in]	srcSize		The source size
		 *\param[in]	dstFormat	The destination format
		 *\param[in]	dstBuffer	The destination buffer
		 *\param[in]	dstSize		The destination size
		 *\~french
		 *\brief		Fonction de conversion sans templates
		 *\param[in]	srcFormat	Le format de la source
		 *\param[in]	srcBuffer	Le buffer source
		 *\param[in]	srcSize		La taille de la source
		 *\param[in]	dstFormat	Le format de la destination
		 *\param[in]	dstBuffer	Le buffer destination
		 *\param[in]	dstSize		La taille de la destination
		 */
		void convertBuffer( ashes::Format srcFormat
			, uint8_t const * srcBuffer
			, uint32_t srcSize
			, ashes::Format dstFormat
			, uint8_t * dstBuffer
			, uint32_t dstSize );
		/**
		 *\~english
		 *\brief		Extracts alpha values from a source buffer holding alpha and puts it in a destination buffer
		 *\remarks		Destination buffer will be a L8 pixel format buffer and alpha channel from source buffer will be removed
		 *\param[in]	srcBuffer	The source buffer
		 *\return		The destination buffer, \p nullptr if source didn't have alpha
		 *\~french
		 *\brief		Extrait les valeurs alpha d'un tampon source pour les mettre dans un tampon à part
		 *\remarks		Le tampon contenant les valeurs alpha sera au format L8 et le canal alpha du tampon source sera supprimé
		 *\param[in]	srcBuffer	Le tampon source
		 *\return		Le tampon alpha, \p nullptr si la source n'avait pas d'alpha
		 */
		PxBufferBaseSPtr extractAlpha( PxBufferBaseSPtr & srcBuffer );
		/**
		 *\see			ashes::Format
		 *\~english
		 *\brief		Checks the alpha PixelComponents support for given pixel format
		 *\return		\p false if format is depth, stencil or one without alpha
		 *\~french
		 *\brief		Vérifie si le format donné possède une composante alpha
		 *\return		\p false si le format est depth, stencil ou un format sans alpha
		 */
		bool hasAlpha( ashes::Format format );
		/**
		 *\see			ashes::Format
		 *\~english
		 *\brief		Checks if the given pixel format is a compressed one
		 *\return		The value
		 *\~french
		 *\brief		Vérifie si le format donné est un format compressé
		 *\return		La valeur
		 */
		bool isCompressed( ashes::Format format );
		/**
		 *\~english
		 *\brief		Retrieves the pixel format without alpha that is near from the one given
		 *\param[in]	p_ePixelFmt	The pixel format
		 *\return		The given pixel format if none found
		 *\~french
		 *\brief		Récupère le format de pixel sans alpha le plus proche de celui donné
		 *\param[in]	p_ePixelFmt	Le format de pixel
		 *\return		Le format de pixels donné si non trouvé
		 */
		ashes::Format getPFWithoutAlpha( ashes::Format format );
		/**
		 *\~english
		 *\brief			Reduces a buffer to it's alpha channel, stored in a L8 format buffer
		 *\param[in,out]	srcBuffer	The buffer to reduce
		 *\~english
		 *\brief			Réduit un tampon à son canal alpha stocké dans un tampon au format L8
		 *\param[in,out]	srcBuffer	Le tampon à réduire
		 */
		void reduceToAlpha( PxBufferBaseSPtr & srcBuffer );
	}
}

#include "PixelFormat.inl"

#endif
