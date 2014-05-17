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

#include "networkdiskcache.h"
#include <QtDebug>
#include <QDateTime>
#include <QDir>
#include <QtConcurrentRun>
#include <QFutureWatcher>
#include <QTimer>
#include <QDirIterator>
#include <QMutexLocker>
#include <util/sys/paths.h>

namespace LeechCraft
{
namespace Util
{
	NetworkDiskCache::NetworkDiskCache (const QString& subpath, QObject *parent)
	: QNetworkDiskCache (parent)
	, CurrentSize_ (-1)
	, InsertRemoveMutex_ (QMutex::Recursive)
	, GarbageCollectorWatcher_ (nullptr)
	{
		setCacheDirectory (GetUserDir (UserDir::Cache, "network/" + subpath).absolutePath ());
	}

	NetworkDiskCache::~NetworkDiskCache ()
	{
		if (GarbageCollectorWatcher_)
			GarbageCollectorWatcher_->waitForFinished ();
	}

	qint64 NetworkDiskCache::cacheSize () const
	{
		return CurrentSize_;
	}

	QIODevice* NetworkDiskCache::data (const QUrl& url)
	{
		QMutexLocker lock (&InsertRemoveMutex_);
		return QNetworkDiskCache::data (url);
	}

	void NetworkDiskCache::insert (QIODevice *device)
	{
		QMutexLocker lock (&InsertRemoveMutex_);
		if (!PendingDev2Url_.contains (device))
		{
			qWarning () << Q_FUNC_INFO
					<< "stall device detected";
			return;
		}

		PendingUrl2Devs_ [PendingDev2Url_.take (device)].removeAll (device);

		CurrentSize_ += device->size ();
		QNetworkDiskCache::insert (device);
	}

	QNetworkCacheMetaData NetworkDiskCache::metaData (const QUrl& url)
	{
		QMutexLocker lock (&InsertRemoveMutex_);
		return QNetworkDiskCache::metaData (url);
	}

	QIODevice* NetworkDiskCache::prepare (const QNetworkCacheMetaData& metadata)
	{
		QMutexLocker lock (&InsertRemoveMutex_);
		const auto dev = QNetworkDiskCache::prepare (metadata);
		PendingDev2Url_ [dev] = metadata.url ();
		PendingUrl2Devs_ [metadata.url ()] << dev;
		return dev;
	}

	bool NetworkDiskCache::remove (const QUrl& url)
	{
		QMutexLocker lock (&InsertRemoveMutex_);
		for (const auto dev : PendingUrl2Devs_.take (url))
			PendingDev2Url_.remove (dev);
		return QNetworkDiskCache::remove (url);
	}

	void NetworkDiskCache::updateMetaData (const QNetworkCacheMetaData& metaData)
	{
		QMutexLocker lock (&InsertRemoveMutex_);
		QNetworkDiskCache::updateMetaData (metaData);
	}

	qint64 NetworkDiskCache::expire ()
	{
		if (CurrentSize_ < 0)
		{
			collectGarbage ();
			return maximumCacheSize () * 8 / 10;
		}

		if (CurrentSize_ > maximumCacheSize ())
			collectGarbage ();

		return CurrentSize_;
	}

	namespace
	{
		qint64 Collector (const QString& cacheDirectory, qint64 goal, QMutex *fileOpMutex)
		{
			if (cacheDirectory.isEmpty ())
				return 0;

			qDebug () << Q_FUNC_INFO << "running..." << cacheDirectory;

			QDir::Filters filters = QDir::AllDirs | QDir:: Files | QDir::NoDotAndDotDot;
			QDirIterator it (cacheDirectory, filters, QDirIterator::Subdirectories);

			QMultiMap<QDateTime, QString> cacheItems;
			qint64 totalSize = 0;
			while (it.hasNext ())
			{
				const auto& path = it.next ();
				const auto& info = it.fileInfo ();
				cacheItems.insert (info.created (), path);
				totalSize += info.size ();
			}

			auto i = cacheItems.constBegin ();
			while (i != cacheItems.constEnd ())
			{
				if (totalSize < goal)
					break;

				QFile file (*i);
				const auto size = file.size ();
				totalSize -= size;
				++i;

				QMutexLocker lock (fileOpMutex);
				file.remove ();
			}

			qDebug () << "collector finished" << totalSize;

			return totalSize;
		}
	};

	void NetworkDiskCache::collectGarbage ()
	{
		if (GarbageCollectorWatcher_)
			return;

		if (cacheDirectory ().isEmpty ())
			return;

		GarbageCollectorWatcher_ = new QFutureWatcher<qint64> (this);
		connect (GarbageCollectorWatcher_,
				SIGNAL (finished ()),
				this,
				SLOT (handleCollectorFinished ()));

		auto future = QtConcurrent::run (Collector,
				cacheDirectory (), maximumCacheSize () * 9 / 10, &InsertRemoveMutex_);
		GarbageCollectorWatcher_->setFuture (future);
	}

	void NetworkDiskCache::handleCollectorFinished ()
	{
		CurrentSize_ = GarbageCollectorWatcher_->result ();
		GarbageCollectorWatcher_->deleteLater ();
		GarbageCollectorWatcher_ = nullptr;
	}
}
}
