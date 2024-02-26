/*
See LICENSE file in root folder
*/
#ifndef ___CRG_Log_H___
#define ___CRG_Log_H___

#include "FrameGraphPrerequisites.hpp"

#pragma warning( push )
#pragma warning( disable: 4365 )
#pragma warning( disable: 5262 )
#include <functional>
#include <memory>
#include <sstream>
#include <string>
#pragma warning( pop )

namespace crg
{
	class Logger
	{
	public:
		using LogCallback = std::function< void( std::string const & msg, bool newLine ) >;

	public:
		/**
		*\brief
		*	Logs a trace message.
		*/
		CRG_API static void logTrace( std::string const & message, bool newLine = true );
		/**
		*\brief
		*	Logs a debug message.
		*/
		CRG_API static void logDebug( std::string const & message, bool newLine = true );
		/**
		*\brief
		*	Logs an info message.
		*/
		CRG_API static void logInfo( std::string const & message, bool newLine = true );
		/**
		*\brief
		*	Logs a warning message.
		*/
		CRG_API static void logWarning( std::string const & message, bool newLine = true );
		/**
		*\brief
		*	Logs an error message.
		*/
		CRG_API static void logError( std::string const & message, bool newLine = true );
		/**
		*\brief
		*	Sets the trace callback.
		*/
		CRG_API static void setTraceCallback( LogCallback callback );
		/**
		*\brief
		*	Sets the debug callback.
		*/
		CRG_API static void setDebugCallback( LogCallback callback );
		/**
		*\brief
		*	Sets the info callback.
		*/
		CRG_API static void setInfoCallback( LogCallback callback );
		/**
		*\brief
		*	Sets the warning callback.
		*/
		CRG_API static void setWarningCallback( LogCallback callback );
		/**
		*\brief
		*	Sets the error callback.
		*/
		CRG_API static void setErrorCallback( LogCallback callback );

	private:
		Logger();

		static Logger & doGetInstance();

	private:
		LogCallback m_trace;
		LogCallback m_debug;
		LogCallback m_info;
		LogCallback m_warning;
		LogCallback m_error;
	};
}

#endif
