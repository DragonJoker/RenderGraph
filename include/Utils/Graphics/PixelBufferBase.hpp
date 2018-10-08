/*
See LICENSE file in root folder
*/
#ifndef ___Utils_PIXEL_BUFFER_BASE_H___
#define ___Utils_PIXEL_BUFFER_BASE_H___

#include "Pixel.hpp"
#include <Miscellaneous/Extent2D.hpp>
#include <Miscellaneous/Offset2D.hpp>

namespace utils
{
	/*!
	\author		Sylvain DOREMUS
	\version	0.6.1.0
	\date		29/08/2011
	\~english
	\brief		Pixel buffer base definition
	\remark		It has 2 dimensions
	\~french
	\brief		Définition de la classe de base d'un buffer de Pixel
	\remark		Il a 2 dimensions
	*/
	class PxBufferBase
	{
	public:
		typedef std::vector< uint8_t > px_array;
		typedef px_array::iterator pixel_data;
		typedef px_array::const_iterator const_pixel_data;

	public:
		/**
		 *\~english
		 *\brief		Constructor.
		 *\param[in]	size	Buffer dimensions.
		 *\param[in]	format	Pixels format.
		 *\~french
		 *\brief		Constructeur.
		 *\param[in]	size	Dimensions du buffer.
		 *\param[in]	format	Format des pixels du buffer.
		 */
		PxBufferBase( Size const & size, ashes::Format format );
		/**
		 *\~english
		 *\brief		Copy Constructor
		 *\param[in]	pixelBuffer	The PxBufferBase object to copy
		 *\~french
		 *\brief		Constructeur par copie
		 *\param[in]	pixelBuffer	L'objet PxBufferBase à copier
		 */
		PxBufferBase( PxBufferBase const & pixelBuffer );
		/**
		 *\~english
		 *\brief		Destructor
		 *\~french
		 *\brief		Destructeur
		 */
		virtual ~PxBufferBase();
		/**
		 *\~english
		 *\brief		Copy assignment operator
		 *\param[in]	pixelBuffer	The PxBufferBase object to copy
		 *\return		A reference to this PxBufferBase object
		 *\~french
		 *\brief		Opérateur d'affectation par copie
		 *\param[in]	pixelBuffer	L'objet PxBufferBase à copier
		 *\return		Une référence sur cet objet PxBufferBase
		 */
		PxBufferBase & operator=( PxBufferBase const & pixelBuffer );
		/**
		 *\brief		Deletes the data buffer
		 *\~french
		 *\brief		Détruit le tampon de données
		 */
		void clear();
		/**
		 *\~english
		 *\brief		Initialises the data buffer to the given one
		 *\remarks		Conversions are made if needed
		 *\param[in]	buffer	Data buffer
		 *\param[in]	format	Data buffer's pixels format
		 *\~french
		 *\brief		Initialise le buffer de données à celui donné
		 *\remarks		Des conversions sont faites si besoin est
		 *\param[in]	buffer	Buffer de données
		 *\param[in]	format	Format des pixels du buffer de données
		 */
		void initialise( uint8_t const * buffer, ashes::Format format );
		/**
		 *\~english
		 *\brief		Initialises the data buffer at the given size
		 *\remarks		Conversions are made if needed
		 *\param[in]	size	Buffer dimensions
		 *\~french
		 *\brief		Initialise le buffer de données à la taille donnée
		 *\remarks		Des conversions sont faites si besoin est
		 *\param[in]	size	Les dimensions du buffer
		 */
		void initialise( Size const & size );
		/**
		 *\~english
		 *\brief		Makes a vertical swap of pixels
		 *\~french
		 *\brief		Effectue un échange vertical des pixels
		 */
		void flip();
		/**
		 *\~english
		 *\brief		Swaps this buffer's data with the given one's
		 *\param[in]	pixelBuffer	The buffer to swap
		 *\~french
		 *\brief		Echange les données de ce buffer avec celles du buffer donné
		 *\param[in]	pixelBuffer	Le buffer à échanger
		 */
		void swap( PxBufferBase & pixelBuffer );
		/**
		 *\~english
		 *\brief		Converts and assigns a data buffer to this buffer
		 *\param[in]	buffer		Data buffer
		 *\param[in]	format	Data buffer's pixels format
		 *\return
		 *\~french
		 *\brief		Convertit et assigne les données du buffer donné à ce buffer
		 *\param[in]	buffer		Buffer de données
		 *\param[in]	format	Format des pixels du buffer de données
		 *\return
		 */
		virtual void assign( std::vector< uint8_t > const & buffer, ashes::Format format ) = 0;
		/**
		 *\~english
		 *\brief		Retrieves the pointer on constant datas
		 *\return		The pointer
		 *\~french
		 *\brief		Récupère le pointeur sur les données constantes
		 *\return		Les données
		 */
		virtual uint8_t const * constPtr()const = 0;
		/**
		 *\~english
		 *\brief		Retrieves the pointer on datas
		 *\return		The pointer
		 *\~french
		 *\brief		Récupère le pointeur sur les données
		 *\return		Les données
		 */
		virtual uint8_t * ptr() = 0;
		/**
		 *\~english
		 *\brief		Retrieves the total size of the buffer
		 *\return		count() * (size of a pixel)
		 *\~french
		 *\brief		Récupère la taille totale du buffer
		 *\return		count() * (size of a pixel)
		 */
		virtual uint32_t size()const = 0;
		/**
		 *\~english
		 *\brief		Creates a new buffer with same values as this one
		 *\return		The created buffer
		 *\~french
		 *\brief		Crée un nouveau buffer avec les mêmes valeurs
		 *\return		Le buffer créé
		 */
		virtual std::shared_ptr< PxBufferBase > clone()const = 0;
		/**
		 *\~english
		 *\brief		Retrieves the pixel data at given position
		 *\param[in]	x, y	The pixel position
		 *\return		The pixel data
		 *\~french
		 *\brief		Récupère les données du pixel à la position donnée
		 *\param[in]	x, y	The pixel position
		 *\return		Les données du pixel
		 */
		virtual pixel_data getAt( uint32_t x, uint32_t y ) = 0;
		/**
		 *\~english
		 *\brief		Retrieves the pixel data at given position
		 *\param[in]	x, y	The pixel position
		 *\return		The constant pixel data
		 *\~french
		 *\brief		Récupère les données du pixel à la position donnée
		 *\param[in]	x, y	The pixel position
		 *\return		Les données constantes du pixel
		 */
		virtual const_pixel_data getAt( uint32_t x, uint32_t y )const = 0;
		/**
		 *\~english
		 *\brief		Makes a horizontal swap of pixels
		 *\~french
		 *\brief		Effectue un échange horizontal des pixels
		 */
		virtual void mirror() = 0;
		/**
		 *\~english
		 *\brief		Retrieves the pixels format
		 *\return		The pixels format
		 *\~french
		 *\brief		Récupère le format ds pixels
		 *\return		Le format des pixels
		 */
		inline ashes::Format format()const
		{
			return m_pixelFormat;
		}
		/**
		 *\~english
		 *\brief		Retrieves the buffer width
		 *\return		The buffer width
		 *\~french
		 *\brief		Récupère la largeur du buffer
		 *\return		La largeur du buffer
		 */
		inline uint32_t getWidth()const
		{
			return m_size.width;
		}
		/**
		 *\~english
		 *\brief		Retrieves the buffer height
		 *\return		The buffer height
		 *\~french
		 *\brief		Récupère la hauteur du buffer
		 *\return		La hauteur du buffer
		 */
		inline uint32_t getHeight()const
		{
			return m_size.height;
		}
		/**
		 *\~english
		 *\brief		Retrieves the buffer's dimensions
		 *\return		The buffer's dimensions
		 *\~french
		 *\brief		Récupère les dimensions du buffer
		 *\return		Les dimensions du buffer
		 */
		inline Size const & dimensions()const
		{
			return m_size;
		}
		/**
		 *\~english
		 *\brief		Retrieves the pixels count
		 *\return		width * height
		 *\~french
		 *\brief		Récupère le compte des pixels
		 *\return		largeur * hauteur
		 */
		inline uint32_t count()const
		{
			return getWidth() * getHeight();
		}
		/**
		 *\~english
		 *\brief		Retrieves the pixel data at given position
		 *\param[in]	p_position	The pixel position
		 *\return		The pixel data
		 *\~french
		 *\brief		Récupère les données du pixel à la position donnée
		 *\param[in]	p_position	The pixel position
		 *\return		Les données du pixel
		 */
		inline pixel_data getAt( Position const & p_position )
		{
			return getAt( p_position.x, p_position.y );
		}
		/**
		 *\~english
		 *\brief		Retrieves the pixel data at given position
		 *\param[in]	p_position	The pixel position
		 *\return		The pixel constant data
		 *\~french
		 *\brief		Récupère les données du pixel à la position donnée
		 *\param[in]	p_position	The pixel position
		 *\return		Les données constantes du pixel
		 */
		inline const_pixel_data getAt( Position const & p_position )const
		{
			return getAt( p_position.x, p_position.y );
		}
		/**
		 *\~english
		 *\brief		Creates a buffer with the given data
		 *\param[in]	size			Buffer dimensions
		 *\param[in]	p_wantedFormat	Pixels format
		 *\param[in]	buffer		Data buffer
		 *\param[in]	p_bufferFormat	Data buffer's pixels format
		 *\return		The created buffer
		 *\~french
		 *\brief		Crée un buffer avec les données voulues
		 *\param[in]	size			Dimensions du buffer
		 *\param[in]	p_wantedFormat	Format des pixels du buffer
		 *\param[in]	buffer		Buffer de données
		 *\param[in]	p_bufferFormat	Format des pixels du buffer de données
		 *\return		Le buffer créé
		 */
		static PxBufferBaseSPtr create( Size const & size
			, ashes::Format p_wantedFormat
			, uint8_t const * buffer = nullptr
			, ashes::Format p_bufferFormat = ashes::Format::eR8G8B8A8_UNORM );

	private:
		ashes::Format m_pixelFormat;

	protected:
		//!\~english Buffer dimensions	\~french Dimensions du buffer
		Size m_size;
		//!\~english Buffer data	\~french données du buffer
		px_array m_buffer;
	};
}

#endif
