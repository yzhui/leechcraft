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

#include "kopeteimportpage.h"
#include <QDir>
#include <QStandardItemModel>
#include <util/sll/prelude.h>
#include "kopeteimportthread.h"

namespace LeechCraft
{
namespace NewLife
{
namespace Importers
{
	KopeteImportPage::KopeteImportPage (const ICoreProxy_ptr& proxy, QWidget *parent)
	: Common::IMImportPage { parent }
	, Proxy_ { proxy }
	{
	}

	void KopeteImportPage::FindAccounts ()
	{
		QDir dir = QDir::home ();
		for (const auto& str : { ".kde4", "share", "apps", "kopete", "logs" })
			if (!dir.cd (str))
			{
				qWarning () << Q_FUNC_INFO
						<< "unable to cd into"
						<< str;
				return;
			}

		for (QString proto : { "JabberProtocol", "ICQProtocol" })
			if (dir.exists (proto))
			{
				proto.chop (QByteArray { "Protocol" }.size ());
				ScanProto (dir.filePath (proto), proto);
			}

		Ui_.AccountsTree_->expandAll ();
	}

	void KopeteImportPage::SendImportAcc (QStandardItem*)
	{
	}

	void KopeteImportPage::SendImportHist (QStandardItem *accItem)
	{
		const QVariantMap& data = accItem->data (Roles::AccountData).toMap ();
		const QString& path = data ["LogsPath"].toString ();

		QDir dir (path);
		const auto& paths = Util::Map (dir.entryList (QDir::Files),
				[&dir] (const QString& file) { return dir.absoluteFilePath (file); });

		const auto thread = new KopeteImportThread (Proxy_, data ["Protocol"].toString (), paths);
		thread->start (QThread::LowestPriority);
	}

	void KopeteImportPage::ScanProto (const QString& path, const QString& proto)
	{
		QStandardItem *protoItem = new QStandardItem (proto);
		AccountsModel_->appendRow (protoItem);

		static const QMap<QString, QString> kopeteProto2LC
		{
			{ "Jabber", "xmpp" },
			{ "ICQ", "oscar" },
			{ "IRC", "irc" }
		};

		if (!kopeteProto2LC.contains (proto))
			return;

		const QDir dir (path);
		for (const auto& accDirName : dir.entryList (QDir::Dirs | QDir::NoDotAndDotDot))
		{
			QDir accDir = dir;
			if (!accDir.cd (accDirName))
			{
				qWarning () << Q_FUNC_INFO
						<< "unable to cd to"
						<< accDirName
						<< path;
				continue;
			}

			QString accId = accDirName;
			accId.replace ('-', '.');

			const QVariantMap accountData
			{
				{ "Jid", accId },
				{ "Protocol", kopeteProto2LC.value (proto) },
				{ "LogsPath", accDir.absolutePath () }
			};

			QList<QStandardItem*> row;
			row << new QStandardItem (accId);
			row << new QStandardItem (accId);
			row << new QStandardItem ();
			row << new QStandardItem ();
			row.first ()->setData (accountData, IMImportPage::Roles::AccountData);
			row [IMImportPage::Column::ImportAcc]->setCheckState (Qt::Checked);
			row [IMImportPage::Column::ImportAcc]->setCheckable (true);
			row [IMImportPage::Column::ImportAcc]->setEnabled (false);
			row [IMImportPage::Column::ImportHist]->setCheckState (Qt::Checked);
			row [IMImportPage::Column::ImportHist]->setCheckable (true);
			Q_FOREACH (auto item, row)
				item->setEditable (false);

			protoItem->appendRow (row);
		}
	}
}
}
}
