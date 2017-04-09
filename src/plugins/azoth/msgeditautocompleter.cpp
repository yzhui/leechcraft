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

#include "msgeditautocompleter.h"
#include <QTextEdit>
#include <interfaces/core/icoreproxy.h>
#include <interfaces/core/ipluginsmanager.h>
#include "interfaces/azoth/iprovidecommands.h"
#include "core.h"
#include "util.h"
#include "xmlsettingsmanager.h"
#include "corecommandsmanager.h"

namespace LeechCraft
{
namespace Azoth
{
	MsgEditAutocompleter::MsgEditAutocompleter (const QString& entryId,
			QTextEdit *edit, QObject *parent)
	: QObject { parent }
	, EntryId_ { entryId }
	, MsgEdit_ { edit }
	{
	}

	QStringList MsgEditAutocompleter::GetPossibleCompletions (const QString& firstPart, int pos) const
	{
		auto result = GetNickCompletions (pos) + GetCommandCompletions (pos);
		result.erase (std::remove_if (result.begin (), result.end (),
					[&firstPart] (const QString& completion)
					{
						return !completion.startsWith (firstPart, Qt::CaseInsensitive);
					}),
				result.end ());
		return result;
	}

	QStringList MsgEditAutocompleter::GetCommandCompletions (int) const
	{
		const auto entryObj = Core::Instance ().GetEntry (EntryId_);
		const auto entry = qobject_cast<ICLEntry*> (entryObj);
		if (!entry)
			return {};

		QStringList result;

		auto cmdProvs = Core::Instance ().GetProxy ()->
				GetPluginsManager ()->GetAllCastableTo<IProvideCommands*> ();
		cmdProvs << Core::Instance ().GetCoreCommandsManager ();
		for (const auto prov : cmdProvs)
			for (const auto& cmd : prov->GetStaticCommands (entry))
				result += cmd.Names_;

		return result;
	}

	QStringList MsgEditAutocompleter::GetNickCompletions (int pos) const
	{
		const auto entryObj = Core::Instance ().GetEntry (EntryId_);
		const auto entry = qobject_cast<IMUCEntry*> (entryObj);
		if (!entry)
			return {};

		auto nicks = GetMucParticipants (EntryId_);
		nicks.removeAll (entry->GetNick ());
		std::sort (nicks.begin (), nicks.end (),
				[] (const QString& left, const QString& right)
				{
					return QString::localeAwareCompare (left.toLower (), right.toLower ()) < 0;
				});

		QString postNick;
		if (pos == -1)
			postNick = XmlSettingsManager::Instance ().property ("PostAddressText").toString ();
		postNick += " ";

		for (auto& nick : nicks)
			nick += postNick;

		return nicks;
	}

	struct NickReplacement
	{
		int Length_;
		QString NewText_;
	};

	NickReplacement MsgEditAutocompleter::GetNickFromState (const QStringList& completions)
	{
		if (AvailableNickList_.isEmpty ())
		{
			AvailableNickList_ = completions;
			if (AvailableNickList_.isEmpty ())
				return {};

			return { NickFirstPart_.size (), AvailableNickList_ [CurrentNickIndex_] };
		}

		auto newAvailableNick = completions;

		int lastNickLen = -1;
		if ((newAvailableNick != AvailableNickList_) && (!newAvailableNick.isEmpty ()))
		{
			int newIndex = newAvailableNick.indexOf (AvailableNickList_ [CurrentNickIndex_ - 1]);
			lastNickLen = AvailableNickList_ [CurrentNickIndex_ - 1].length ();

			while (newIndex == -1 && CurrentNickIndex_ > 0)
				newIndex = newAvailableNick.indexOf (AvailableNickList_ [--CurrentNickIndex_]);

			CurrentNickIndex_ = (newIndex == -1 ? 0 : newIndex);
			AvailableNickList_ = newAvailableNick;
		}

		if (CurrentNickIndex_ < AvailableNickList_.count () && CurrentNickIndex_)
			return { AvailableNickList_ [CurrentNickIndex_ - 1].size (), AvailableNickList_ [CurrentNickIndex_] };
		else if (CurrentNickIndex_)
		{
			CurrentNickIndex_ = 0;
			return { AvailableNickList_.last ().size (), AvailableNickList_ [CurrentNickIndex_] };
		}
		else
			return { lastNickLen, AvailableNickList_ [CurrentNickIndex_] };
	}

	void MsgEditAutocompleter::complete ()
	{
		auto cursor = MsgEdit_->textCursor ();

		int cursorPosition = cursor.position ();
		int pos = -1;

		auto text = MsgEdit_->toPlainText ();

		if (AvailableNickList_.isEmpty ())
		{
			pos = text.lastIndexOf (' ', cursorPosition ? cursorPosition - 1 : 0);
			LastSpacePosition_ = pos;
		}
		else
			pos = LastSpacePosition_;

		if (NickFirstPart_.isNull ())
			NickFirstPart_ = cursorPosition ?
					text.mid (pos + 1, cursorPosition - pos - 1) :
					"";

		const auto& completions = GetPossibleCompletions (NickFirstPart_, pos);
		const auto& replacement = GetNickFromState (completions);
		if (replacement.NewText_.isEmpty ())
			return;

		text.replace (pos + 1, replacement.Length_, replacement.NewText_);
		++CurrentNickIndex_;

		MsgEdit_->setPlainText (text);
		cursor.setPosition (pos + 1 + replacement.NewText_.size ());
		MsgEdit_->setTextCursor (cursor);
	}

	void MsgEditAutocompleter::resetState ()
	{
		NickFirstPart_.clear ();
		if (!AvailableNickList_.isEmpty ())
		{
			AvailableNickList_.clear ();
			CurrentNickIndex_ = 0;
		}
	}
}
}
