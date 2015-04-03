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

#include "proxiesconfigwidget.h"
#include <QStandardItemModel>
#include <QSettings>
#include <util/sll/slotclosure.h>
#include "proxiesstorage.h"
#include "editurlsdialog.h"
#include "editlistsdialog.h"
#include "scriptsmanager.h"
#include "proxyconfigdialog.h"
#include "proxiesstorage.h"

namespace LeechCraft
{
namespace XProxy
{
	namespace
	{
		QString ProxyType2Str (QNetworkProxy::ProxyType type)
		{
			switch (type)
			{
			case QNetworkProxy::ProxyType::Socks5Proxy:
				return "SOCKS5";
				break;
			case QNetworkProxy::ProxyType::HttpProxy:
				return "HTTP";
				break;
			case QNetworkProxy::ProxyType::HttpCachingProxy:
				return ProxiesConfigWidget::tr ("caching HTTP");
				break;
			case QNetworkProxy::ProxyType::FtpCachingProxy:
				return ProxiesConfigWidget::tr ("caching FTP");
				break;
			case QNetworkProxy::ProxyType::NoProxy:
				return ProxiesConfigWidget::tr ("no proxy");
				break;
			default:
				return ProxiesConfigWidget::tr ("other type");
				break;
			}
		}

		QList<QStandardItem*> Proxy2Row (const Proxy& proxy)
		{
			QList<QStandardItem*> row;
			row << new QStandardItem (ProxyType2Str (proxy.Type_));
			row << new QStandardItem (proxy.Host_ + ":" + QString::number (proxy.Port_));
			row << new QStandardItem (proxy.User_);
			return row;
		}
	}

	ProxiesConfigWidget::ProxiesConfigWidget (ProxiesStorage *storage,
			ScriptsManager *manager, QWidget *parent)
	: QWidget (parent)
	, Storage_ (storage)
	, ScriptsMgr_ (manager)
	, Model_ (new QStandardItemModel (this))
	{
		Ui_.setupUi (this);
		Ui_.ProxiesList_->setModel (Model_);

		connect (Ui_.ProxiesList_->selectionModel (),
				SIGNAL (currentRowChanged (QModelIndex, QModelIndex)),
				this,
				SLOT (handleItemSelected (QModelIndex)));

		const QStringList labels
		{
			tr ("Proxy type"),
			tr ("Proxy target"),
			tr ("User")
		};
		Model_->setHorizontalHeaderLabels (labels);

		reject ();
	}

	void ProxiesConfigWidget::accept ()
	{
		Storage_->SaveSettings ();
	}

	void ProxiesConfigWidget::reject ()
	{
		if (const auto rc = Model_->rowCount ())
			Model_->removeRows (0, rc);

		Storage_->LoadSettings ();

		Proxies_ = Storage_->GetKnownProxies ();
		for (const auto& proxy : Proxies_)
			Model_->appendRow (Proxy2Row (proxy));
	}

	void ProxiesConfigWidget::handleItemSelected (const QModelIndex& idx)
	{
		Ui_.UpdateProxyButton_->setEnabled (idx.isValid ());
		Ui_.RemoveProxyButton_->setEnabled (idx.isValid ());
		Ui_.EditUrlsButton_->setEnabled (idx.isValid ());
		Ui_.EditListsButton_->setEnabled (idx.isValid ());
	}

	void ProxiesConfigWidget::on_AddProxyButton__released ()
	{
		ProxyConfigDialog dia { this };
		if (dia.exec () != QDialog::Accepted)
			return;

		const auto& proxy = dia.GetProxy ();
		Proxies_ << proxy;
		Model_->appendRow (Proxy2Row (proxy));

		Storage_->AddProxy (proxy);
	}

	void ProxiesConfigWidget::on_UpdateProxyButton__released ()
	{
		const int row = Ui_.ProxiesList_->currentIndex ().row ();
		if (row < 0 || row >= Proxies_.size ())
			return;

		const auto oldProxy = Proxies_.at (row);

		ProxyConfigDialog dia { this };
		dia.SetProxy (oldProxy);
		if (dia.exec () != QDialog::Accepted)
			return;

		const auto& proxy = dia.GetProxy ();
		if (proxy == oldProxy)
			return;

		Proxies_ [row] = proxy;
		Model_->removeRow (row);
		Model_->insertRow (row, Proxy2Row (proxy));

		Storage_->UpdateProxy (oldProxy, proxy);
	}

	void ProxiesConfigWidget::on_RemoveProxyButton__released ()
	{
		const int row = Ui_.ProxiesList_->currentIndex ().row ();
		if (row < 0 || row >= Proxies_.size ())
			return;

		Model_->removeRow (row);
		Storage_->RemoveProxy (Proxies_.takeAt (row));
	}

	void ProxiesConfigWidget::on_MoveUpButton__released ()
	{
		const int row = Ui_.ProxiesList_->currentIndex ().row ();
		if (row <= 0)
			return;

		Model_->insertRow (row, Model_->takeRow (row - 1));
	}

	void ProxiesConfigWidget::on_MoveDownButton__released ()
	{
	}

	void ProxiesConfigWidget::on_EditUrlsButton__released ()
	{
		const int row = Ui_.ProxiesList_->currentIndex ().row ();
		if (row < 0 || row >= Proxies_.size ())
			return;

		const auto& proxy = Proxies_.value (row);
		auto dialog = new EditUrlsDialog { Storage_->GetTargets (proxy), this };
		dialog->setAttribute (Qt::WA_DeleteOnClose);

		new Util::SlotClosure<Util::DeleteLaterPolicy>
		{
			[this, proxy, dialog] { Storage_->SetTargets (proxy, dialog->GetTargets ()); },
			dialog,
			SIGNAL (accepted ()),
			dialog
		};

		dialog->show ();
	}

	void ProxiesConfigWidget::on_EditListsButton__released ()
	{
		const int row = Ui_.ProxiesList_->currentIndex ().row ();
		if (row < 0 || row >= Proxies_.size ())
			return;

		const auto& proxy = Proxies_.value (row);
		auto dialog = new EditListsDialog { Storage_->GetScripts (proxy), ScriptsMgr_, this };
		dialog->setAttribute (Qt::WA_DeleteOnClose);

		new Util::SlotClosure<Util::DeleteLaterPolicy>
		{
			[this, proxy, dialog] { Storage_->SetScripts (proxy, dialog->GetScripts ()); },
			dialog,
			SIGNAL (accepted ()),
			dialog
		};

		dialog->show ();
	}
}
}
