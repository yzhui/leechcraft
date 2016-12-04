/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Boost Software License - Version 1.0 - August 17th, 2003
 *
 * Permission is hereby granted, free of charge, to any person or organization
 * obtaining a copy of the software and accompanying documentation covered by
 * this license (the "Software") to use, reproduce, display, distribute,
 * execute, and transmit the Software, and to prepare derivative works of the
 * Software, and to permit third-parties to whom the Software is furnished to
 * do so, all subject to the following:
 *
 * The copyright notices in the Software and this entire statement, including
 * the above license grant, this restriction and the following disclaimer,
 * must be included in all copies of the Software, in whole or in part, and
 * all derivative works of the Software, unless such copies or derivative
 * works are solely in the form of machine-executable object code generated by
 * a source language processor.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 **********************************************************************/

#include "debugmessagehandler.h"
#include <memory>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <cstring>
#include <boost/optional.hpp>

#if defined (_GNU_SOURCE) || defined (Q_OS_OSX)
#include <execinfo.h>
#include <cxxabi.h>
#endif

#include <unistd.h>

#include <QMutex>
#include <QMutexLocker>
#include <QThread>
#include <QDateTime>
#include <QDir>
#include <QProcess>
#include <util/sll/monad.h>

QMutex G_DbgMutex;
uint Counter = 0;

namespace
{
	QString GetFilename (QtMsgType type)
	{
		switch (type)
		{
		case QtDebugMsg:
			return "debug.log";
#if QT_VERSION >= 0x050500
		case QtInfoMsg:
			return "info.log";
#endif
		case QtWarningMsg:
			return "warning.log";
		case QtCriticalMsg:
			return "critical.log";
		case QtFatalMsg:
			return "fatal.log";
		}

		return "unknown.log";
	}

	bool SupportsColors ()
	{
		static const auto supportsColors = isatty (fileno (stdout));
		return supportsColors;
	}

	std::string GetColorCode (QtMsgType type)
	{
		if (!SupportsColors ())
			return {};

		switch (type)
		{
		case QtDebugMsg:
#if QT_VERSION >= 0x050500
		case QtInfoMsg:
#endif
			return "\x1b[32m";
		case QtWarningMsg:
			return "\x1b[33m";
		case QtCriticalMsg:
			return "\x1b[31m";
		case QtFatalMsg:
			return "\x1b[35m";
		}

		return {};
	}

	std::shared_ptr<std::ostream> GetOstream (QtMsgType type, DebugHandler::DebugWriteFlags flags)
	{
		if (flags & DebugHandler::DWFNoFileLog)
		{
			auto& stream = type == QtDebugMsg ? std::cout : std::cerr;

			stream << GetColorCode (type);
			switch (type)
			{
			case QtDebugMsg:
				stream << "[DBG] ";
				break;
#if QT_VERSION >= 0x050500
			case QtInfoMsg:
				stream << "[INF] ";
				break;
#endif
			case QtWarningMsg:
				stream << "[WRN] ";
				break;
			case QtCriticalMsg:
				stream << "[CRT] ";
				break;
			case QtFatalMsg:
				stream << "[FTL] (yay, really `faster than light`!) ";
				break;
			}
			if (SupportsColors ())
				stream << "\x1b[0m";

			return { &stream, [] (std::ostream*) {} };
		}

		const QString name = QDir::homePath () + "/.leechcraft/" + GetFilename (type);

		auto ostr = std::make_shared<std::ofstream> ();
		ostr->open (QDir::toNativeSeparators (name).toStdString (), std::ios::app);
		return ostr;
	}

#if defined (_GNU_SOURCE)
	struct AddrInfo
	{
		std::string ObjectPath_;
		std::string SourcePath_;
		std::string Symbol_;
	};

	using StrRange_t = std::pair<const char*, const char*>;

	boost::optional<StrRange_t> FindStrRange (const char *str, char open, char close)
	{
		const auto strEnd = str + std::strlen (str);
		const auto parensCount = std::count_if (str, strEnd,
				[=] (char c) { return c == open || c == close; });
		if (parensCount != 2)
			return {};

		const auto openParen = std::find (str, strEnd, open);
		const auto closeParen = std::find (openParen, strEnd, close);
		if (openParen == strEnd || closeParen == strEnd)
			return {};

		return { { openParen + 1, closeParen } };
	}

	boost::optional<AddrInfo> QueryAddr2Line (const std::string& execName,
			const std::string& addr, bool textSection)
	{
		QProcess proc;
		QStringList params { "-Cfe", QString::fromStdString (execName), QString::fromStdString (addr) };
		if (textSection)
			params.prepend ("-j.text");

		proc.start ("addr2line", params);
		if (!proc.waitForFinished (500))
			return {};

		if (proc.exitStatus () != QProcess::NormalExit ||
			proc.exitCode ())
			return {};

		const auto& out = proc.readAllStandardOutput ().trimmed ().split ('\n');
		if (out.size () != 2)
			return {};

		return { { execName, out [1].constData (), out [0].constData () } };
	}

	boost::optional<AddrInfo> QueryAddr2LineExecutable (const char *str,
			const std::string& execName)
	{
		using LeechCraft::Util::operator>>;

		return FindStrRange (str, '[', ']') >>
				[&] (const auto& bracketRange)
				{
					return QueryAddr2Line (execName,
							{ bracketRange.first, bracketRange.second },
							false);
				};
	}

	boost::optional<AddrInfo> QueryAddr2LineLibrary (const std::string& execName,
			const std::string& addr)
	{
		return QueryAddr2Line (execName, addr, true);
	}

	boost::optional<std::string> GetDemangled (const char *str)
	{
		int status = -1;
		const auto demangled = abi::__cxa_demangle (str, nullptr, 0, &status);

		if (!demangled)
			return {};

		const std::string demangledStr { demangled };
		free (demangled);
		return demangledStr;
	}

	boost::optional<AddrInfo> GetAddrInfo (const char *str)
	{
		using LeechCraft::Util::operator>>;

		return FindStrRange (str, '(', ')') >>
				[str] (const auto& pair)
				{
					const std::string binaryName { str, pair.first - 1 };

					const auto plusPos = std::find (pair.first, pair.second, '+');

					if (plusPos == pair.second)
						return QueryAddr2LineExecutable (str, binaryName);

					if (plusPos == pair.first)
						return QueryAddr2LineLibrary (binaryName, { plusPos, pair.second });

					return GetDemangled (pair.first) >>
							[&] (const std::string& value)
							{
								return boost::optional<AddrInfo> { { binaryName, {}, value } };
							};
				};
	}
#elif defined (Q_OS_OSX)
	boost::optional<AddrInfo> GetAddrInfo (const char *str)
	{
		return {};
	}
#endif

	void PrintBacktrace (const std::shared_ptr<std::ostream>& ostr)
	{
#if defined (_GNU_SOURCE) || defined (Q_OS_OSX)
		const int maxSize = 100;
		void *callstack [maxSize];
		size_t size = backtrace (callstack, maxSize);
		char **strings = backtrace_symbols (callstack, size);

		*ostr << "Backtrace of " << size << " frames:" << std::endl;

		for (size_t i = 0; i < size; ++i)
		{
			*ostr << i << "\t";

			if (const auto info = GetAddrInfo (strings [i]))
				*ostr << info->ObjectPath_
						<< ": "
						<< info->Symbol_
						<< " ["
						<< info->SourcePath_
						<< "]"
						<< std::endl;
			else
				*ostr << strings [i] << std::endl;
		}

		std::free (strings);
#endif
	}
}

namespace DebugHandler
{
	void Write (QtMsgType type, const char *message, DebugWriteFlags flags)
	{
#if !defined (Q_OS_WIN32)
		if (!strcmp (message, "QPixmap::handle(): Pixmap is not an X11 class pixmap") ||
				strstr (message, ": Painter not active"))
			return;
#endif
#if defined (Q_OS_WIN32)
		if (!strcmp (message, "QObject::startTimer: QTimer can only be used with threads started with QThread"))
			return;
#endif

		QMutexLocker locker { &G_DbgMutex };

		const auto& ostr = GetOstream (type, flags);
		*ostr << "["
				<< QDateTime::currentDateTime ().toString ("dd.MM.yyyy HH:mm:ss.zzz").toStdString ()
				<< "] ["
				<< QThread::currentThread ()
				<< "] ["
				<< std::setfill ('0')
				<< std::setw (3)
				<< Counter++
				<< "] "
				<< message
				<< std::endl;

		if (type != QtDebugMsg && (flags & DWFBacktrace))
			PrintBacktrace (ostr);
	}
}
