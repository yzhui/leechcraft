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

#include "poster.h"
#include <QClipboard>
#include <QApplication>
#include <QStandardItemModel>
#include <QNetworkReply>
#include <QtDebug>
#include <interfaces/structures.h>
#include <interfaces/core/ientitymanager.h>
#include <interfaces/ijobholder.h>
#include <util/xpc/util.h>
#include <util/util.h>

namespace LeechCraft
{
namespace Imgaste
{
	Poster::Poster (HostingService service,
			const QByteArray& data,
			const QString& format,
			ICoreProxy_ptr proxy,
			DataFilterCallback_f callback,
			QStandardItemModel *reprModel,
			QObject *parent)
	: QObject (parent)
	, Reply_ (0)
	, Service_ (service)
	, Worker_ (MakeWorker (service))
	, Proxy_ (proxy)
	, Callback_ (callback)
	, ReprModel_ (reprModel)
	, ReprRow_ ([reprModel]
			{
				const QList<QStandardItem*> result
				{
					new QStandardItem { tr ("Image upload") },
					new QStandardItem { tr ("Uploading...") },
					new QStandardItem { }
				};
				reprModel->appendRow (result);
				return result;
			} ())
	, RowRemoveGuard_ (nullptr, [this] (void*) { ReprModel_->removeRow (ReprRow_.first ()->row ()); })
	{
		for (const auto item : ReprRow_)
		{
			item->setEditable (false);
			item->setData (QVariant::fromValue<JobHolderRow> (JobHolderRow::ProcessProgress),
					CustomDataRoles::RoleJobHolderRow);
		}
		setUploadProgress (0, data.size ());

		Reply_ = Worker_->Post (data, format, proxy->GetNetworkAccessManager ());
		connect (Reply_,
				SIGNAL (uploadProgress (qint64, qint64)),
				this,
				SLOT (setUploadProgress (qint64, qint64)));

		connect (Reply_,
				SIGNAL (finished ()),
				this,
				SLOT (handleFinished ()));
		connect (Reply_,
				SIGNAL (error (QNetworkReply::NetworkError)),
				this,
				SLOT (handleError ()));
	}

	void Poster::handleFinished ()
	{
		const auto& result = Reply_->readAll ();
		Reply_->deleteLater ();

		const auto& pasteUrl = Worker_->GetLink (result, Reply_);

		auto em = Proxy_->GetEntityManager ();
		if (pasteUrl.isEmpty ())
		{
			em->HandleEntity (Util::MakeNotification ("Imgaste",
					tr ("Page parse failed"), PCritical_));
			return;
		}

		if (!Callback_)
		{
			QApplication::clipboard ()->setText (pasteUrl, QClipboard::Clipboard);
			QApplication::clipboard ()->setText (pasteUrl, QClipboard::Selection);

			QString text = tr ("Image pasted: %1, the URL was copied to the clipboard")
				.arg (pasteUrl);
			em->HandleEntity (Util::MakeNotification ("Imgaste", text, PInfo_));
		}
		else
			Callback_ (pasteUrl);

		deleteLater ();
	}

	void Poster::handleError ()
	{
		qWarning () << Q_FUNC_INFO
			<< Reply_->errorString ();
		Reply_->deleteLater ();

		QString text = tr ("Image upload failed: %1")
							.arg (Reply_->errorString ());
		Proxy_->GetEntityManager ()->HandleEntity (Util::MakeNotification ("Imgaste", text, PCritical_));

		deleteLater ();
	}

	void Poster::setUploadProgress (qint64 done, qint64 total)
	{
		Util::SetJobHolderProgress (ReprRow_, done, total,
				tr ("%1 of %2")
					.arg (Util::MakePrettySize (done))
					.arg (Util::MakePrettySize (total)));
	}
}
}
