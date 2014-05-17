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

#include "coreproxywrapper.h"
#include <interfaces/core/irootwindowsmanager.h>
#include <interfaces/core/iiconthememanager.h>
#include "shortcutproxywrapper.h"
#include "pluginsmanagerwrapper.h"
#include "tagsmanagerwrapper.h"

namespace LeechCraft
{
namespace Qrosp
{
	CoreProxyWrapper::CoreProxyWrapper (ICoreProxy_ptr proxy)
	: Proxy_ (proxy)
	{
	}

	QNetworkAccessManager* CoreProxyWrapper::GetNetworkAccessManager () const
	{
		return Proxy_->GetNetworkAccessManager ();
	}

	QObject* CoreProxyWrapper::GetShortcutProxy () const
	{
		return new ShortcutProxyWrapper (Proxy_->GetShortcutProxy ());
	}

	QModelIndex CoreProxyWrapper::MapToSource (const QModelIndex& index) const
	{
		return Proxy_->MapToSource (index);
	}

	QIcon CoreProxyWrapper::GetIcon (const QString& on, const QString& off) const
	{
		return Proxy_->GetIconThemeManager ()->GetIcon (on, off);
	}

	QMainWindow* CoreProxyWrapper::GetMainWindow () const
	{
		return Proxy_->GetRootWindowsManager ()->GetMainWindow (0);
	}

	ICoreTabWidget* CoreProxyWrapper::GetTabWidget () const
	{
		return Proxy_->GetRootWindowsManager ()->GetTabWidget (0);
	}

	QObject* CoreProxyWrapper::GetTagsManager () const
	{
		return new TagsManagerWrapper (Proxy_->GetTagsManager ());
	}

	QStringList CoreProxyWrapper::GetSearchCategories () const
	{
		return Proxy_->GetSearchCategories ();
	}

	int CoreProxyWrapper::GetID ()
	{
		return Proxy_->GetID ();
	}

	void CoreProxyWrapper::FreeID (int id)
	{
		Proxy_->FreeID (id);
	}

	QObject* CoreProxyWrapper::GetPluginsManager () const
	{
		return new PluginsManagerWrapper (Proxy_->GetPluginsManager ());
	}
}
}
