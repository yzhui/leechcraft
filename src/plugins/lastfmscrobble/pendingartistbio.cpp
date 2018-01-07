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

#include "pendingartistbio.h"
#include <algorithm>
#include <QNetworkReply>
#include <QDomDocument>
#include <QtDebug>
#include <util/network/handlenetworkreply.h>
#include "util.h"
#include "imagesfetcher.h"

namespace LeechCraft
{
namespace Lastfmscrobble
{
	PendingArtistBio::PendingArtistBio (QString name,
			QNetworkAccessManager *nam, bool addImages, QObject *parent)
	: QObject (parent)
	, NAM_ (nam)
	, AddImages_ (addImages)
	{
		Promise_.reportStarted ();

		QMap<QString, QString> params
		{
			{ "artist", name },
			{ "autocorrect", "1" }
		};
		AddLanguageParam (params);
		Util::Sequence (this, Util::HandleReply<QString> (Request ("artist.getInfo", nam, params), this)) >>
				Util::Visitor
				{
					[this] (const QString& err)
					{
						Util::ReportFutureResult (Promise_, err);
						deleteLater ();
					},
					[this] (const QByteArray& data) { HandleFinished (data); }
				};
	}

	QFuture<Media::IArtistBioFetcher::ArtistBioResult_t> PendingArtistBio::GetFuture ()
	{
		return Promise_.future ();
	}

	void PendingArtistBio::HandleFinished (const QByteArray& data)
	{
		QDomDocument doc;
		if (!doc.setContent (data))
		{
			qWarning () << Q_FUNC_INFO
					<< "unable to parse reply";
			Util::ReportFutureResult (Promise_, "unable to parse reply");
			deleteLater ();
			return;
		}

		Media::ArtistBio bio;
		bio.BasicInfo_ = GetArtistInfo (doc.documentElement ().firstChildElement ("artist"));
		std::reverse (bio.BasicInfo_.Tags_.begin (), bio.BasicInfo_.Tags_.end ());

		if (!AddImages_)
		{
			Util::ReportFutureResult (Promise_, bio);
			deleteLater ();
			return;
		}

		const auto imgFetcher = new ImagesFetcher { bio.BasicInfo_.Name_, NAM_, this };
		connect (imgFetcher,
				&ImagesFetcher::gotImages,
				this,
				[this, bio] (const QList<Media::ArtistImage>& images) mutable
				{
					bio.OtherImages_ = images;
					Util::ReportFutureResult (Promise_, bio);
					deleteLater ();
				});
	}
}
}
