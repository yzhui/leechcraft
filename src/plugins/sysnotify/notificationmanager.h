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

#include <memory>
#include <QObject>
#include <QDBusInterface>
#include <QPointer>
#include <interfaces/structures.h>

class QDBusPendingCallWatcher;

namespace LeechCraft
{
namespace Sysnotify
{
	class NotificationManager : public QObject
	{
		Q_OBJECT

		std::unique_ptr<QDBusInterface> Connection_;

		struct CapCheckData
		{
			Entity Entity_;
		};
		QMap<QDBusPendingCallWatcher*, CapCheckData> Watcher2CapCheck_;

		struct ActionData
		{
			Entity E_;
			QObject_ptr Handler_;
			QStringList Actions_;
		};
		QMap<QDBusPendingCallWatcher*, ActionData> Watcher2AD_;
		QMap<uint, ActionData> CallID2AD_;

		bool IgnoreTimeoutCloses_ = false;
	public:
		NotificationManager (QObject* = 0);

		bool CouldNotify (const Entity&) const;
		void HandleNotification (const Entity&);
	private:
		void DoNotify (const Entity&, bool);
	private slots:
		void handleGotServerInfo (QDBusPendingCallWatcher*);
		void handleNotificationCallFinished (QDBusPendingCallWatcher*);
		void handleCapCheckCallFinished (QDBusPendingCallWatcher*);
		void handleActionInvoked (uint, QString);
		void handleNotificationClosed (uint, uint);
	};
}
}
