/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2012  Georg Rudoy
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **********************************************************************/

#ifndef COREINSTANCEOBJECT_H
#define COREINSTANCEOBJECT_H
#include <QObject>
#include "interfaces/iinfo.h"
#include "interfaces/ihavesettings.h"
#include "interfaces/ihavetabs.h"
#include "interfaces/ipluginready.h"
#include "interfaces/ihaveshortcuts.h"

class IShortcutProxy;

namespace LeechCraft
{
	class SettingsTab;
	class CorePlugin2Manager;
	class ShortcutManager;

	namespace Util
	{
		class ShortcutManager;
	}

	class CoreInstanceObject : public QObject
							 , public IInfo
							 , public IHaveSettings
							 , public IHaveTabs
							 , public IHaveShortcuts
							 , public IPluginReady
	{
		Q_OBJECT
		Q_INTERFACES (IInfo IHaveSettings IHaveTabs IHaveShortcuts IPluginReady)

		Util::XmlSettingsDialog_ptr XmlSettingsDialog_;
		TabClasses_t Classes_;

		SettingsTab *SettingsTab_;

		CorePlugin2Manager *CorePlugin2Manager_;

		ShortcutManager *ShortcutManager_;

		Util::ShortcutManager *CoreShortcutManager_;
	public:
		CoreInstanceObject (QObject* = 0);

		// IInfo
		void Init (ICoreProxy_ptr);
		void SecondInit ();
		void Release ();
		QByteArray GetUniqueID () const;
		QString GetName () const;
		QString GetInfo () const;
		QIcon GetIcon () const;

		// IHaveSettings
		Util::XmlSettingsDialog_ptr GetSettingsDialog () const;

		// IHaveTabs
		TabClasses_t GetTabClasses () const;
		void TabOpenRequested (const QByteArray&);

		// IHaveShortcuts
		QMap<QString, ActionInfo> GetActionInfo () const;
		void SetShortcut (const QString& id, const QKeySequences_t& sequences);

		// IPluginReady
		QSet<QByteArray> GetExpectedPluginClasses () const;
		void AddPlugin (QObject*);

		CorePlugin2Manager* GetCorePluginManager () const;

		SettingsTab* GetSettingsTab () const;

		IShortcutProxy* GetShortcutProxy () const;
		ShortcutManager* GetShortcutManager () const;
		Util::ShortcutManager* GetCoreShortcutManager () const;
	private:
		void BuildNewTabModel ();
	private slots:
		void handleSettingsButton (const QString&);
		void updateIconSet ();
		void updateColorTheme ();
#ifdef STRICT_LICENSING
		void notifyLicensing ();
#endif
	signals:
		void addNewTab (const QString&, QWidget*);
		void removeTab (QWidget*);
		void changeTabName (QWidget*, const QString&);
		void changeTabIcon (QWidget*, const QIcon&);
		void statusBarChanged (QWidget*, const QString&);
		void raiseTab (QWidget*);

		void gotEntity (const LeechCraft::Entity&);
	};
}

#endif
