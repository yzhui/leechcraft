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

#include "hypedartistsfetcher.h"
#include <algorithm>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QDomDocument>
#include <QtDebug>
#include <util/sll/visitor.h>
#include <util/sll/either.h>
#include <util/threads/futures.h>
#include "util.h"
#include "pendingartistbio.h"

namespace LeechCraft
{
namespace Lastfmscrobble
{
	HypedArtistsFetcher::HypedArtistsFetcher (QNetworkAccessManager *nam, Media::IHypesProvider::HypeType type, QObject *parent)
	: QObject (parent)
	, NAM_ (nam)
	, Type_ (type)
	{
		QMap<QString, QString> params;
		params ["limit"] = "20";
		const auto& method = type == Media::IHypesProvider::HypeType::NewArtists ?
				"chart.getHypedArtists" :
				"chart.getTopArtists";
		auto reply = Request (method, nam, params);
		connect (reply,
				SIGNAL (finished ()),
				this,
				SLOT (handleFinished ()));
		connect (reply,
				SIGNAL (error (QNetworkReply::NetworkError)),
				this,
				SLOT (handleError ()));
	}

	void HypedArtistsFetcher::DecrementWaiting ()
	{
		if (--InfoCount_)
			return;

		emit gotHypedArtists (Infos_, Type_);
		deleteLater ();
	}

	void HypedArtistsFetcher::handleFinished ()
	{
		auto reply = qobject_cast<QNetworkReply*> (sender ());
		reply->deleteLater ();

		const auto& data = reply->readAll ();
		QDomDocument doc;
		if (!doc.setContent (data))
		{
			qWarning () << Q_FUNC_INFO
					<< "error parsing reply"
					<< data;
			return;
		}

		auto artistElem = doc
				.documentElement ()
				.firstChildElement ("artists")
				.firstChildElement ("artist");
		while (!artistElem.isNull ())
		{
			auto getText = [&artistElem] (const QString& name)
			{
				return artistElem.firstChildElement (name).text ();
			};

			const auto& name = getText ("name");

			Infos_ << Media::HypedArtistInfo
			{
				Media::ArtistInfo
				{
					name,
					QString (),
					QString (),
					GetImage (artistElem, "medium"),
					GetImage (artistElem, "extralarge"),
					getText ("url"),
					Media::TagInfos_t ()
				},
				getText ("percentagechange").toInt (),
				getText ("playcount").toInt (),
				getText ("listeners").toInt ()
			};

			auto pendingBio = new PendingArtistBio (name, NAM_, false, this);
			Util::Sequence (this, pendingBio->GetFuture ()) >>
					Util::Visitor
					{
						[] (const QString&) {},
						[this] (const Media::ArtistBio& info)
						{
							const auto& name = info.BasicInfo_.Name_;
							const auto pos = std::find_if (Infos_.begin (), Infos_.end (),
									[&name] (const auto& other) { return other.Info_.Name_ == name; });
							if (pos != Infos_.end ())
								pos->Info_ = info.BasicInfo_;
						}
					}.Finally ([this] { DecrementWaiting (); });

			artistElem = artistElem.nextSiblingElement ("artist");
		}

		InfoCount_ = Infos_.size ();
		if (!InfoCount_)
			deleteLater ();
	}

	void HypedArtistsFetcher::handleError ()
	{
		auto reply = qobject_cast<QNetworkReply*> (sender ());

		qWarning () << Q_FUNC_INFO
				<< reply->errorString ();

		reply->deleteLater ();
		deleteLater ();
	}
}
}
