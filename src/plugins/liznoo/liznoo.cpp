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

#include "liznoo.h"
#include <cmath>
#include <limits>
#include <QIcon>
#include <QAction>
#include <QTimer>
#include <interfaces/core/icoreproxy.h>
#include <interfaces/core/ientitymanager.h>
#include <interfaces/entitytesthandleresult.h>
#include <util/util.h>
#include <util/xpc/util.h>
#include <xmlsettingsdialog/xmlsettingsdialog.h>
#include "xmlsettingsmanager.h"
#include "batteryhistorydialog.h"
#include "platform/screen/screenplatformlayer.h"

#if defined(Q_OS_LINUX)
	#include "platform/platformupower.h"
	#include "platform/screen/screenplatformfreedesktop.h"
#elif defined(Q_OS_WIN32)
	#include "platform/platformwinapi.h"
#elif defined(Q_OS_FREEBSD)
	#include "platform/platformfreebsd.h"
	#include "platform/screen/screenplatformfreedesktop.h"
#elif defined(Q_OS_MAC)
	#include "platform/platformmac.h"
#else
	#pragma message ("Unsupported system")
#endif

namespace LeechCraft
{
namespace Liznoo
{
	const int HistSize = 300;

	void Plugin::Init (ICoreProxy_ptr proxy)
	{
		Proxy_ = proxy;
		qRegisterMetaType<BatteryInfo> ("Liznoo::BatteryInfo");

		Util::InstallTranslator ("liznoo");

		XSD_.reset (new Util::XmlSettingsDialog);
		XSD_->RegisterObject (XmlSettingsManager::Instance (), "liznoosettings.xml");

#if defined(Q_OS_LINUX)
		PL_ = new PlatformUPower (Proxy_, this);
		SPL_ = new ScreenPlatformFreedesktop (this);
#elif defined(Q_OS_WIN32)
		PL_ = new PlatformWinAPI (Proxy_, this);
#elif defined(Q_OS_FREEBSD)
		PL_ = new PlatformFreeBSD (Proxy_, this);
		SPL_ = new ScreenPlatformFreedesktop (this);
#elif defined(Q_OS_MAC)
		PL_ = new PlatformMac (Proxy_, this);
#else
		PL_ = 0;
#endif

		connect (PL_,
				SIGNAL (started ()),
				this,
				SLOT (handlePlatformStarted ()));

		Suspend_ = new QAction (tr ("Suspend"), this);
		connect (Suspend_,
				SIGNAL (triggered ()),
				this,
				SLOT (handleSuspendRequested ()));
		Suspend_->setProperty ("ActionIcon", "system-suspend");

		Hibernate_ = new QAction (tr ("Hibernate"), this);
		connect (Hibernate_,
				SIGNAL (triggered ()),
				this,
				SLOT (handleHibernateRequested ()));
		Hibernate_->setProperty ("ActionIcon", "system-suspend-hibernate");

		connect (XSD_.get (),
				SIGNAL (pushButtonClicked (QString)),
				this,
				SLOT (handlePushButton (QString)));
	}

	void Plugin::SecondInit ()
	{
	}

	QByteArray Plugin::GetUniqueID () const
	{
		return "org.LeechCraft.Liznoo";
	}

	void Plugin::Release ()
	{
		if (PL_)
			PL_->Stop ();
	}

	QString Plugin::GetName () const
	{
		return "Liznoo";
	}

	QString Plugin::GetInfo () const
	{
		return tr ("UPower/WinAPI-based power manager.");
	}

	QIcon Plugin::GetIcon () const
	{
		static QIcon icon ("lcicons:/liznoo/resources/images/liznoo.svg");
		return icon;
	}

	Util::XmlSettingsDialog_ptr Plugin::GetSettingsDialog () const
	{
		return XSD_;
	}

	EntityTestHandleResult Plugin::CouldHandle (const Entity& entity) const
	{
		return entity.Mime_ == "x-leechcraft/power-management" ?
				EntityTestHandleResult (EntityTestHandleResult::PIdeal) :
				EntityTestHandleResult ();
	}

	void Plugin::Handle (Entity entity)
	{
		const auto& context = entity.Entity_.toString ();
		if (context == "ScreensaverProhibition")
		{
			if (!SPL_)
			{
				qWarning () << Q_FUNC_INFO
						<< "screen platform layer unavailable, screensaver prohibiton won't work";
				return;
			}

			const auto enable = entity.Additional_ ["Enable"].toBool ();
			const auto& id = entity.Additional_ ["ContextID"].toString ();

			SPL_->ProhibitScreensaver (enable, id);
		}
	}

	QList<QAction*> Plugin::GetActions (ActionsEmbedPlace place) const
	{
		QList<QAction*> result;
		if (place == ActionsEmbedPlace::LCTray)
			result << Battery2Action_.values ();
		return result;
	}

	QMap<QString, QList<QAction*>> Plugin::GetMenuActions () const
	{
		QMap<QString, QList<QAction*>> result;
		result ["System"] << Suspend_;
		result ["System"] << Hibernate_;
		return result;
	}

	namespace
	{
		QString GetBattIconName (BatteryInfo info)
		{
			const bool isCharging = info.TimeToFull_ && !info.TimeToEmpty_;

			QString name = "battery-";
			if (isCharging)
				name += "charging-";

			if (info.Percentage_ < 15)
				name += "low";
			else if (info.Percentage_ < 30)
				name += "caution";
			else if (info.Percentage_ < 50)
				name += "040";
			else if (info.Percentage_ < 70)
				name += "060";
			else if (info.Percentage_ < 90)
				name += "080";
			else if (isCharging)
				name.chop (1);
			else
				name += "100";

			return name;
		}
	}

	void Plugin::UpdateAction (const BatteryInfo& info)
	{
		QAction *act = Battery2Action_ [info.ID_];

		const bool isDischarging = info.TimeToEmpty_ && !info.TimeToFull_;
		const bool isCharging = info.TimeToFull_ && !info.TimeToEmpty_;

		QString text = QString::number (info.Percentage_) + '%';
		if (isCharging)
			text += " " + tr ("(charging)");
		else if (isDischarging)
			text += " " + tr ("(discharging)");
		act->setText (text);

		QString tooltip = QString ("%1: %2<br />")
				.arg (tr ("Battery"))
				.arg (text);
		if (isCharging)
			tooltip += QString ("%1 until charged")
					.arg (Util::MakeTimeFromLong (info.TimeToFull_));
		else if (isDischarging)
			tooltip += QString ("%1 until discharged")
					.arg (Util::MakeTimeFromLong (info.TimeToEmpty_));
		if (isCharging || isDischarging)
			tooltip += "<br /><br />";

		tooltip += tr ("Battery technology: %1")
				.arg (info.Technology_);
		tooltip += "<br />";
		auto isNotNull = [] (double val) { return std::fabs (val) > std::numeric_limits<double>::epsilon (); };
		if (isNotNull (info.EnergyRate_))
		{
			tooltip += tr ("Energy rate: %1 W")
					.arg (std::abs (info.EnergyRate_));
			tooltip += "<br />";
		}
		if (isNotNull (info.Energy_))
		{
			tooltip += tr ("Remaining energy: %1 Wh")
					.arg (info.Energy_);
			tooltip += "<br />";
		}
		if (isNotNull (info.EnergyFull_))
		{
			tooltip += tr ("Full energy capacity: %1 Wh")
					.arg (info.EnergyFull_);
			tooltip += "<br />";
		}

		act->setToolTip (tooltip);
		act->setProperty ("ActionIcon", GetBattIconName (info));
	}

	void Plugin::CheckNotifications (const BatteryInfo& info)
	{
		auto check = [&info, this] (std::function<bool (const BatteryInfo&)> f) -> bool
		{
			if (!Battery2LastInfo_.contains (info.ID_))
				return f (info);

			return f (info) && !f (Battery2LastInfo_ [info.ID_]);
		};

		auto checkPerc = [] (const BatteryInfo& b, const QByteArray& prop)
			{ return b.Percentage_ <= XmlSettingsManager::Instance ()->property (prop).toInt (); };

		const bool isExtremeLow = check ([checkPerc] (const BatteryInfo& b)
				{ return checkPerc (b, "NotifyOnExtremeLowPower"); });
		const bool isLow = check ([checkPerc] (const BatteryInfo& b)
				{ return checkPerc (b, "NotifyOnLowPower"); });

		const auto iem = Proxy_->GetEntityManager ();
		if (isExtremeLow || isLow)
			iem->HandleEntity (Util::MakeNotification ("Liznoo",
						tr ("Battery charge level is below %1.")
							.arg (info.Percentage_),
						isLow ? PWarning_ : PCritical_));

		if (XmlSettingsManager::Instance ()->property ("NotifyOnPowerTransitions").toBool ())
		{
			const bool startedCharging = check ([] (const BatteryInfo& b)
					{ return b.TimeToFull_ && !b.TimeToEmpty_; });
			const bool startedDischarging = check ([] (const BatteryInfo& b)
					{ return !b.TimeToFull_ && b.TimeToEmpty_; });

			if (startedCharging)
				iem->HandleEntity (Util::MakeNotification ("Liznoo",
							tr ("The device started charging."),
							PInfo_));
			else if (startedDischarging)
				iem->HandleEntity (Util::MakeNotification ("Liznoo",
							tr ("The device started discharging."),
							PWarning_));
		}
	}

	void Plugin::handleBatteryInfo (BatteryInfo info)
	{
		if (!Battery2Action_.contains (info.ID_))
		{
			QAction *act = new QAction (tr ("Battery status"), this);
			act->setProperty ("WatchActionIconChange", true);
			act->setProperty ("Liznoo/BatteryID", info.ID_);

			act->setProperty ("Action/Class", GetUniqueID () + "/BatteryAction");
			act->setProperty ("Action/ID", GetUniqueID () + "/" + info.ID_);

			emit gotActions (QList<QAction*> () << act, ActionsEmbedPlace::LCTray);
			Battery2Action_ [info.ID_] = act;

			connect (act,
					SIGNAL (triggered ()),
					this,
					SLOT (handleHistoryTriggered ()));
		}

		UpdateAction (info);
		CheckNotifications (info);

		Battery2LastInfo_ [info.ID_] = info;
	}

	void Plugin::handleUpdateHistory ()
	{
		for (const QString& id : Battery2LastInfo_.keys ())
		{
			auto& hist = Battery2History_ [id];
			hist << BatteryHistory (Battery2LastInfo_ [id]);
			if (hist.size () > HistSize)
				hist.removeFirst ();
		}

		for (const QString& id : Battery2Dialog_.keys ())
			Battery2Dialog_ [id]->UpdateHistory (Battery2History_ [id], Battery2LastInfo_ [id]);
	}

	void Plugin::handleHistoryTriggered ()
	{
		const QString& id = sender ()->
				property ("Liznoo/BatteryID").toString ();
		if (!Battery2History_.contains (id) ||
				Battery2Dialog_.contains (id))
		{
			auto dia = static_cast<BatteryHistoryDialog*> (Battery2Dialog_.value (id));
			if (dia)
				dia->close ();
			return;
		}

		auto dialog = new BatteryHistoryDialog (HistSize);
		dialog->UpdateHistory (Battery2History_ [id], Battery2LastInfo_ [id]);
		dialog->setAttribute (Qt::WA_DeleteOnClose);
		Battery2Dialog_ [id] = dialog;
		connect (dialog,
				SIGNAL (destroyed (QObject*)),
				this,
				SLOT (handleBatteryDialogDestroyed ()));
		dialog->show ();
		dialog->activateWindow ();
		dialog->raise ();
	}

	void Plugin::handleBatteryDialogDestroyed ()
	{
		auto dia = static_cast<BatteryHistoryDialog*> (sender ());
		Battery2Dialog_.remove (Battery2Dialog_.key (dia));
	}

	void Plugin::handlePlatformStarted ()
	{
		connect (PL_,
				SIGNAL (batteryInfoUpdated (Liznoo::BatteryInfo)),
				this,
				SLOT (handleBatteryInfo (Liznoo::BatteryInfo)));

		QTimer *timer = new QTimer (this);
		connect (timer,
				SIGNAL (timeout ()),
				this,
				SLOT (handleUpdateHistory ()));
		timer->start (3000);
	}

	void Plugin::handleSuspendRequested ()
	{
		PL_->ChangeState (PlatformLayer::PowerState::Suspend);
	}

	void Plugin::handleHibernateRequested ()
	{
		PL_->ChangeState (PlatformLayer::PowerState::Hibernate);
	}

	void Plugin::handlePushButton (const QString& button)
	{
		if (!PL_)
		{
			qWarning () << Q_FUNC_INFO
					<< "platform backend unavailable";
			return;
		}

		if (button == "TestSleep")
			PL_->emitGonnaSleep (1000);
		else if (button == "TestWake")
			PL_->emitWokeUp ();
	}
}
}

LC_EXPORT_PLUGIN (leechcraft_liznoo, LeechCraft::Liznoo::Plugin);
