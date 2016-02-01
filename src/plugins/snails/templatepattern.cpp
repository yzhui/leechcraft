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

#include "templatepattern.h"
#include <QHash>
#include <interfaces/itexteditor.h>
#include "message.h"
#include "account.h"

namespace LeechCraft
{
namespace Snails
{
	namespace
	{
		template<typename F, typename = std::result_of_t<F (const Message*)>>
		PatternFunction_t Wrap (const F& f)
		{
			return [f] (const Account*, const Message *msg, ContentType, const QString&)
					{
						if (!msg)
							return QString {};

						return f (msg);
					};
		}
	}

	QList<TemplatePattern> GetKnownPatterns ()
	{
		static const QList<TemplatePattern> patterns
		{
			{
				"ODATE",
				Wrap ([] (const Message *msg) { return msg->GetDate ().date ().toString (Qt::DefaultLocaleLongDate); })
			},
			{
				"OTIME",
				Wrap ([] (const Message *msg) { return msg->GetDate ().time ().toString (Qt::DefaultLocaleLongDate); })
			},
			{
				"ODATETIME",
				Wrap ([] (const Message *msg) { return msg->GetDate ().toString (Qt::DefaultLocaleShortDate); })
			},
			{
				"ONAME",
				Wrap ([] (const Message *msg) { return msg->GetAddress (Message::Address::From).first; })
			},
			{
				"OEMAIL",
				Wrap ([] (const Message *msg) { return msg->GetAddress (Message::Address::From).second; })
			},
			{
				"ONAMEOREMAIL",
				Wrap ([] (const Message *msg)
						{
							const auto& addr = msg->GetAddress (Message::Address::ReplyTo);
							return addr.first.isEmpty () ? addr.second : addr.first;
						})
			},
			{
				"QUOTE",
				[] (const Account*, const Message*, ContentType, const QString& body) { return body; }
			},
			{
				"SIGNATURE",
				[] (const Account *acc, const Message*, ContentType type, const QString&)
				{
					switch (type)
					{
					case ContentType::PlainText:
						return "-- \n  " + acc->GetUserName () + "\n";
					case ContentType::HTML:
						return "-- <br/>&nbsp;&nbsp;" + acc->GetUserName () + "<br/>";
					}
				}
			},
			{
				"CURSOR",
				[] (const Account*, const Message*, ContentType type, const QString&)
				{
					switch (type)
					{
					case ContentType::PlainText:
						return "${CURSOR}";
					case ContentType::HTML:
						return "<div id='place_cursor_here'>&nbsp;</div>";
					}
				}
			},
		};

		return patterns;
	}

	QHash<QString, PatternFunction_t> GetKnownPatternsHash ()
	{
		static const auto hash = []
				{
					QHash<QString, PatternFunction_t> result;
					for (const auto& pattern : GetKnownPatterns ())
						result [pattern.PatternText_] = pattern.Substitute_;
					return result;
				} ();
		return hash;
	}
}
}
