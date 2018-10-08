/*
See LICENSE file in root folder
*/
#ifndef ___Utils_BLOCK_TIMER___
#define ___Utils_BLOCK_TIMER___

#include "Utils/Miscellaneous/PreciseTimer.hpp"
#include "Utils/Miscellaneous/Utils.hpp"

namespace utils
{
	/*!
	\author		Sylvain DOREMUS
	\version	0.6.5.0
	\date		24/04/2012
	\~english
	\brief		Helper class, used to time a block's execution time
	\remark		Call the macro CASTOR_TIME() at the beginning of a block to have a console output when leaving that block
	\~french
	\brief		Classe permettant de mesurer le temps d'exécution d'un bloc
	\remark		Appelez la macro CASTOR_TIME() au début d'un bloc pour avoir une entrée dans la console en sortie du bloc
	*/
	class BlockTimer
	{
	public:
		/**
		 *\~english
		 *\brief		Constructor
		 *\param[in]	p_szFunction	Caller function
		 *\param[in]	p_szFile		Function file
		 *\param[in]	p_uiLine		Function line
		 *\~french
		 *\brief		Constructeur
		 *\param[in]	p_szFunction	La fonction appelante
		 *\param[in]	p_szFile		Le fichier oà se trouve la fonction
		 *\param[in]	p_uiLine		La ligne dans la fonction
		 */
		BlockTimer( char const * p_szFunction, char const * p_szFile, uint32_t p_uiLine );
		/**
		 *\~english
		 *\brief		Destructor
		 *\~french
		 *\brief		Destructeur
		 */
		~BlockTimer();

	private:
		String m_strFile;
		String m_strFunction;
		uint32_t const m_uiLine;
		PreciseTimer m_timer;
	};
}

#	define CASTOR_TIME() utils::BlockTimer timer##__LINE__( __FUNCTION__, __FILE__, __LINE__ )

#endif
