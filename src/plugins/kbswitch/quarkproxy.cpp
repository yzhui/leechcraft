/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2010-2012  Oleg Linkin
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

#include "quarkproxy.h"
#include <QCursor>
#include <QMenu>
#include <QtDebug>
#include "kbctl.h"

namespace LeechCraft
{
namespace KBSwitch
{
	QuarkProxy::QuarkProxy (QObject *parent)
	: QObject (parent)
	{
		connect (&KBCtl::Instance (),
				SIGNAL (groupChanged (int)),
				this,
				SLOT (handleGroupChanged (int)));
		handleGroupChanged (KBCtl::Instance ().GetCurrentGroup ());
	}

	QString QuarkProxy::GetCurrentLangCode () const
	{
		return CurrentLangCode_;
	}

	void QuarkProxy::setNextLanguage ()
	{
		KBCtl::Instance ().EnableNextGroup ();
	}

	void QuarkProxy::contextMenuRequested ()
	{
		QMenu menu;

		auto& kbctl = KBCtl::Instance ();
		const auto& enabled = kbctl.GetEnabledGroups ();
		const auto curGrpIdx = kbctl.GetCurrentGroup ();

		for (int i = 0; i < enabled.size (); ++i)
		{
			const auto& actionName = QString ("%1 (%2)")
					.arg (kbctl.GetLayoutDesc (i))
					.arg (enabled.at (i));
			const auto act = menu.addAction (actionName,
					this,
					SLOT (handleGroupSelectAction ()));
			act->setCheckable (true);
			if (curGrpIdx == i)
				act->setChecked (true);
			act->setProperty ("KBSwitch/GrpIdx", i);
		}

		menu.exec (QCursor::pos ());
	}

	void QuarkProxy::handleGroupSelectAction ()
	{
		const auto grpIdx = sender ()->property ("KBSwitch/GrpIdx").toInt ();
		KBCtl::Instance ().EnableGroup (grpIdx);
	}

	void QuarkProxy::handleGroupChanged (int group)
	{
		CurrentLangCode_ = KBCtl::Instance ().GetLayoutName (group);
		emit currentLangCodeChanged ();
	}
}
}
