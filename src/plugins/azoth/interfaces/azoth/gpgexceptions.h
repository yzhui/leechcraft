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

#pragma once

#include <stdexcept>
#include <boost/variant.hpp>

namespace LeechCraft
{
namespace Azoth
{
namespace GPGExceptions
{
	class General : public std::runtime_error
	{
		int Code_;
		QString Message_;
	public:
		General (const QString& context, int code, const QString& msg)
		: std::runtime_error
		{
			context.toStdString () + std::to_string (code) + ": " + msg.toStdString ()
		}
		, Code_ { code }
		, Message_ { msg }
		{
		}

		General (int code, const QString& msg)
		: General { "Azoth GPG error", code, msg }
		{
		}

		int GetCode () const
		{
			return Code_;
		}

		const QString& GetMessage () const
		{
			return Message_;
		}
	};

	class NullPubkey : public General
	{
	public:
		NullPubkey ()
		: General { "Azoth GPG: null pubkey", -1, {} }
		{
		}
	};

	class Encryption : public General
	{
	public:
		Encryption (int code, const QString& msg)
		: General { "Azoth GPG encryption error", code, msg }
		{
		}
	};

	using AnyException_t = boost::variant<Encryption, NullPubkey, General>;
}
}
}
