/*
See LICENSE file in root folder
*/
#ifndef ___Utils_BoundingSphere_H___
#define ___Utils_BoundingSphere_H___

#include "BoundingContainer.hpp"

namespace utils
{
	/*!
	\author		Sylvain DOREMUS
	\date		14/02/2010
	\~english
	\brief		Sphere container class.
	\~french
	\brief		Classe de conteneur sphérique.
	*/
	class BoundingSphere
		: public BoundingContainer3D
	{
	public:
		BoundingSphere( BoundingSphere const & rhs ) = default;
		BoundingSphere( BoundingSphere && rhs ) = default;
		BoundingSphere & operator=( BoundingSphere const & rhs ) = default;
		BoundingSphere & operator=( BoundingSphere && rhs ) = default;
		/**
		 *\~english
		 *\brief		Default constructor.
		 *\~french
		 *\brief		Constructeur par défaut.
		 */
		BoundingSphere();
		/**
		 *\~english
		 *\brief		Specified constructor.
		 *\param[in]	center	The center of the sphere.
		 *\param[in]	radius	The radius of the sphere.
		 *\~french
		 *\brief		Constructeur spécifié.
		 *\param[in]	center	Le centre de la sphère.
		 *\param[in]	radius	Le rayon de la sphère.
		 */
		BoundingSphere( Point3r const & center, real radius );
		/**
		 *\~english
		 *\brief		Constructor from a BoundingBox.
		 *\param[in]	box	The BoundingBox.
		 *\~french
		 *\brief		Constructeur à partir d'une CubeBox.
		 *\param[in]	box	La BoundingBox.
		 */
		explicit BoundingSphere( BoundingBox const & box );
		/**
		 *\~english
		 *\brief		Reinitialises the sphere box.
		 *\param[in]	center	The center.
		 *\param[in]	radius	The radius.
		 *\~french
		 *\brief		Réinitialise la sphere box.
		 *\param[in]	center	Le centre.
		 *\param[in]	radius	Le rayon.
		 */
		void load( Point3r const & center, real radius );
		/**
		 *\~english
		 *\brief		Reinitialises the sphere box from a BoundingBox.
		 *\param[in]	box	The BoundingBox.
		 *\~french
		 *\brief		Réinitialise à partir d'une CubeBox.
		 *\param[in]	box	La BoundingBox.
		 */
		void load( BoundingBox const & box );
		/**
		 *\~english
		 *\brief		Tests if a vertex is within the container, id est inside it but not on it's limits.
		 *\param[in]	point	The vertex to test.
		 *\return		\p true if the vertex is within the container, false if not.
		 *\~french
		 *\brief		Teste si un point est contenu dans le conteneur (mais pas à la limite).
		 *\param[in]	point	Le point à tester.
		 *\return		\p true si le point est dans le conteneur.
		 */
		bool isWithin( Point3r const & point )const override;
		/**
		 *\~english
		 *\brief		Tests if a vertex is on the limits of this container, and not within.
		 *\param[in]	point	The vertex to test.
		 *\return		\p true if the vertex is on the limits of the container, false if not.
		 *\~french
		 *\brief		Teste si un point est sur la limite du conteneur, et pas dedans.
		 *\param[in]	point	Le point à tester.
		 *\return		\p true si le point est sur la limite.
		 */
		bool isOnLimits( Point3r const & point )const override;
		/**
		 *\~english
		 *\return		The radius.
		 *\~french
		 *\return		Le rayon.
		 */
		inline real getRadius()const
		{
			return m_radius;
		}

	private:
		//!\~english	The radius of the sphere.
		//!\~french		Le rayon de la sphère.
		real m_radius;
	};
}

#endif
