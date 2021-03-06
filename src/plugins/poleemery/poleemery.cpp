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

#include "poleemery.h"
#include <QIcon>
#include <util/sll/prelude.h>
#include <xmlsettingsdialog/xmlsettingsdialog.h>
#include "operationstab.h"
#include "accountstab.h"
#include "graphstab.h"
#include "xmlsettingsmanager.h"
#include "core.h"
#include "currenciesmanager.h"

namespace LeechCraft
{
namespace Poleemery
{
	void Plugin::Init (ICoreProxy_ptr proxy)
	{
		XSD_.reset (new Util::XmlSettingsDialog);
		XSD_->RegisterObject (&XmlSettingsManager::Instance (), "poleemerysettings.xml");

		XSD_->SetDataSource ("CurrenciesView",
				Core::Instance ().GetCurrenciesManager ()->GetSettingsModel ());

		Core::Instance ().SetCoreProxy (proxy);

		TabClasses_.append ({
				{
					GetUniqueID () + "/Operations",
					tr ("Finances operations"),
					tr ("All operations on personal finances."),
					QIcon (),
					2,
					TFOpenableByRequest
				},
				[this] (const TabClassInfo& tc) { MakeTab<OperationsTab> (tc); }
			});
		TabClasses_.append ({
				{
					GetUniqueID () + "/Accounts",
					tr ("Finances accounts"),
					tr ("Finances accounts management tab."),
					QIcon (),
					1,
					TFOpenableByRequest
				},
				[this] (const TabClassInfo& tc) { MakeTab<AccountsTab> (tc); }
			});
		TabClasses_.append ({
				{
					GetUniqueID () + "/Graphs",
					tr ("Spending graphs"),
					tr ("Tab with various graphs helping to analyze spendings."),
					QIcon (),
					1,
					TFOpenableByRequest
				},
				[this] (const TabClassInfo& tc) { MakeTab<GraphsTab> (tc); }
			});
	}

	void Plugin::SecondInit ()
	{
	}

	QByteArray Plugin::GetUniqueID () const
	{
		return "org.LeechCraft.Poleemery";
	}

	void Plugin::Release ()
	{
	}

	QString Plugin::GetName () const
	{
		return "Poleemery";
	}

	QString Plugin::GetInfo () const
	{
		return tr ("The personal finances manager.");
	}

	QIcon Plugin::GetIcon () const
	{
		return QIcon ();
	}

	TabClasses_t Plugin::GetTabClasses () const
	{
		return Util::Map (TabClasses_, Util::Fst);
	}

	void Plugin::TabOpenRequested (const QByteArray& tc)
	{
		const auto pos = std::find_if (TabClasses_.begin (), TabClasses_.end (),
				[&tc] (const auto& pair) { return pair.first.TabClass_ == tc; });
		if (pos == TabClasses_.end ())
		{
			qWarning () << Q_FUNC_INFO
					<< "unknown tab class"
					<< tc;
			return;
		}

		pos->second (pos->first);
	}

	Util::XmlSettingsDialog_ptr Plugin::GetSettingsDialog () const
	{
		return XSD_;
	}

	template<typename T>
	void Plugin::MakeTab (const TabClassInfo& tc)
	{
		const auto tab = new T { tc, this };

		connect (tab,
				SIGNAL (removeTab (QWidget*)),
				this,
				SIGNAL (removeTab (QWidget*)));
		emit addNewTab (tc.VisibleName_, tab);
		emit changeTabIcon (tab, tc.Icon_);
		emit raiseTab (tab);
	}
}
}

LC_EXPORT_PLUGIN (leechcraft_poleemery, LeechCraft::Poleemery::Plugin);
