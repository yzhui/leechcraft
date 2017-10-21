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

#include "recentlyopenedmanager.h"
#include <QSettings>
#include <QCoreApplication>
#include <QFileInfo>
#include <QMenu>
#include <QMessageBox>
#include <QtDebug>
#include "xmlsettingsmanager.h"
#include "core.h"

namespace LeechCraft
{
namespace Monocle
{
	RecentlyOpenedManager::RecentlyOpenedManager (QObject *parent)
	: QObject (parent)
	{
		QSettings settings (QCoreApplication::organizationName (),
				QCoreApplication::applicationName () + "_Monocle");
		OpenedDocs_ = settings.value ("RecentlyOpened").toStringList ();
	}

	QMenu* RecentlyOpenedManager::CreateOpenMenu (QWidget *docTab)
	{
		if (const auto menu = Menus_ [docTab])
			return menu;

		auto result = new QMenu (tr ("Recently opened"), docTab);
		UpdateMenu (result);
		Menus_ [docTab] = result;
		connect (docTab,
				&QObject::destroyed,
				this,
				[this, docTab] { Menus_.remove (docTab); });
		return result;
	}

	void RecentlyOpenedManager::RecordOpened (const QString& path)
	{
		if (OpenedDocs_.value (0) == path)
			return;

		if (OpenedDocs_.contains (path))
			OpenedDocs_.removeAll (path);
		OpenedDocs_.prepend (path);

		const int listLength = XmlSettingsManager::Instance ()
				.property ("RecentlyOpenedListSize").toInt ();
		if (OpenedDocs_.size () > listLength)
			OpenedDocs_.erase (OpenedDocs_.begin () + listLength, OpenedDocs_.end ());

		QSettings settings (QCoreApplication::organizationName (),
				QCoreApplication::applicationName () + "_Monocle");
		settings.setValue ("RecentlyOpened", OpenedDocs_);

		for (const auto& menu : Menus_.values ())
			UpdateMenu (menu);
	}

	void RecentlyOpenedManager::UpdateMenu (QMenu *menu)
	{
		menu->clear ();
		for (const auto& path : OpenedDocs_)
		{
			auto act = menu->addAction (QFileInfo (path).fileName ());
			act->setProperty ("Path", path);
			act->setToolTip (path);
		}
	}
}
}
