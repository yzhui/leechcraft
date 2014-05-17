/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2012  Georg Rudoy
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

#include "icecastfetcher.h"
#include "roles.h"
#include <algorithm>
#include <functional>
#include <QStandardItem>
#include <QFileInfo>
#include <QUrl>
#include <QDateTime>
#include <QDomDocument>
#include <QTimer>
#include <QtConcurrentRun>
#include <QFutureWatcher>
#include <QDir>
#include <util/xpc/util.h>
#include <util/sys/paths.h>
#include <interfaces/idownload.h>

Q_DECLARE_METATYPE (QList<QUrl>);

namespace LeechCraft
{
namespace HotStreams
{
	const QString XiphFilename ("yp.xml");

	namespace
	{
		QString GetFilePath ()
		{
			return Util::CreateIfNotExists ("hotstreams/cache").filePath (XiphFilename);
		}

		bool ShouldUpdateFile (const QString& path)
		{
			return QFileInfo (path).lastModified ().daysTo (QDateTime::currentDateTime ()) > 2;
		}
	}

	IcecastFetcher::IcecastFetcher (QStandardItem *root, QNetworkAccessManager*, QObject *parent)
	: QObject (parent)
	, Root_ (root)
	, JobID_ (0)
	, RadioIcon_ (":/hotstreams/resources/images/radio.png")
	{
		auto dir = Util::CreateIfNotExists ("hotstreams/cache");
		const bool exists = dir.exists (XiphFilename);
		if (!exists || ShouldUpdateFile (dir.filePath (XiphFilename)))
		{
			if (exists)
				dir.remove (XiphFilename);

			QTimer::singleShot (0,
					this,
					SLOT (handleFetchList ()));
		}
		else
			ParseList ();
	}

	void IcecastFetcher::FetchList ()
	{
		auto entity = Util::MakeEntity (QUrl ("http://dir.xiph.org/yp.xml"),
				GetFilePath (),
				OnlyDownload |
					Internal |
					DoNotAnnounceEntity |
					DoNotNotifyUser |
					DoNotSaveInHistory);
		QObject *obj = 0;
		emit delegateEntity (entity, &JobID_, &obj);
		if (!obj)
		{
			qWarning () << Q_FUNC_INFO
					<< "unable to delegate entity";
			deleteLater ();
			return;
		}

		connect (obj,
				SIGNAL (jobFinished (int)),
				this,
				SLOT (handleJobFinished (int)));
		connect (obj,
				SIGNAL (jobRemoved (int)),
				this,
				SLOT (checkDelete (int)));
	}

	namespace
	{
		struct StationInfo
		{
			QString Name_;
			QString Genre_;
			int Bitrate_;
			QList<QUrl> URLs_;
			QString MIME_;
		};

		typedef QMap<QString, QList<StationInfo>> Stations_t;

		void SortInfoList (QList<StationInfo>& infos)
		{
			std::sort (infos.begin (), infos.end (),
				[] (decltype (infos.at (0)) left, decltype (infos.at (0)) right)
					{ return QString::localeAwareCompare (left.Name_, right.Name_) < 0; });
		}

		Stations_t ParseWorker ()
		{
			QFile file (GetFilePath ());
			if (!file.open (QIODevice::ReadOnly))
			{
				qWarning () << Q_FUNC_INFO
						<< "unable to open file";
				return Stations_t ();
			}

			QDomDocument doc;
			if (!doc.setContent (&file))
			{
				qWarning () << Q_FUNC_INFO
						<< "parse failure, removing the file";
				file.remove ();
				return Stations_t ();
			}

			Stations_t stations;

			auto entry = doc.documentElement ().firstChildElement ("entry");
			while (!entry.isNull ())
			{
				auto getText = [&entry] (const QString& tagName)
				{
					return entry.firstChildElement (tagName).text ();
				};

				const auto& genre = getText ("genre");

				auto& genreStations = stations [genre];
				const StationInfo info =
				{
					getText ("server_name"),
					genre,
					getText ("bitrate").toInt (),
					QList<QUrl> () << QUrl (getText ("listen_url")),
					getText ("server_type")
				};
				const auto pos = std::find_if (genreStations.begin (), genreStations.end (),
						[&info] (const StationInfo& otherInfo)
							{ return info.Name_ == otherInfo.Name_ &&
									info.Bitrate_ == otherInfo.Bitrate_ &&
									info.MIME_ == otherInfo.MIME_; });
				if (pos == genreStations.end ())
					genreStations << info;
				else
					pos->URLs_ << info.URLs_;

				entry = entry.nextSiblingElement ("entry");
			}

			if (stations.size () <= 20)
				return stations;

			QList<int> lengths;
			for (const auto& genre : stations.keys ())
				lengths << stations [genre].size ();

			std::sort (lengths.begin (), lengths.end (), std::greater<int> ());
			const int threshold = lengths.at (20);

			QList<StationInfo> otherInfos;
			for (const auto& genre : stations.keys ())
			{
				auto& genreStations = stations [genre];
				if (genreStations.size () <= threshold && genre != "metal")
				{
					otherInfos += genreStations;
					stations.remove (genre);
				}
				else
					SortInfoList (genreStations);
			}
			SortInfoList (otherInfos);
			stations ["Other"] = otherInfos;
			return stations;
		}
	}

	void IcecastFetcher::ParseList ()
	{
		auto watcher = new QFutureWatcher<Stations_t> (this);
		connect (watcher,
				SIGNAL (finished ()),
				this,
				SLOT (handleParsed ()));
		watcher->setFuture (QtConcurrent::run (ParseWorker));
	}

	void IcecastFetcher::handleFetchList ()
	{
		FetchList ();
	}

	void IcecastFetcher::handleParsed ()
	{
		auto watcher = dynamic_cast<QFutureWatcher<Stations_t>*> (sender ());
		watcher->deleteLater ();

		const auto& stringTemplate = tr ("Genre: %1\nBitrate: %2 kbps\nType: %3");

		const auto& result = watcher->result ();
		for (const auto& genre : result.keys ())
		{
			auto uppercased = genre;
			uppercased [0] = uppercased.at (0).toUpper ();

			auto genreItem = new QStandardItem (uppercased);
			genreItem->setEditable (false);

			for (const auto& station : result [genre])
			{
				const auto& tooltip = stringTemplate
						.arg (station.Genre_)
						.arg (station.Bitrate_)
						.arg (station.MIME_);
				auto item = new QStandardItem (station.Name_);
				item->setToolTip (tooltip);
				item->setIcon (RadioIcon_);
				item->setData (station.Name_, StreamItemRoles::PristineName);
				item->setData (Media::RadioType::Predefined, Media::RadioItemRole::ItemType);
				item->setData ("urllist", StreamItemRoles::PlaylistFormat);
				item->setData (QVariant::fromValue (station.URLs_), Media::RadioItemRole::RadioID);
				item->setEditable (false);

				genreItem->appendRow (item);
			}

			Root_->appendRow (genreItem);
		}
	}

	void IcecastFetcher::handleJobFinished (int id)
	{
		if (id != JobID_)
			return;

		ParseList ();

		checkDelete (id);
	}

	void IcecastFetcher::checkDelete (int id)
	{
		if (id == JobID_)
			deleteLater ();
	}
}
}
