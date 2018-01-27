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

#include "pendingsimilarartists.h"
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QDomDocument>
#include "util.h"

namespace LeechCraft
{
namespace Lastfmscrobble
{
	PendingSimilarArtists::PendingSimilarArtists (const QString& name,
			int num, QNetworkAccessManager *nam, QObject *parent)
	: BaseSimilarArtists (name, num, parent)
	, NAM_ (nam)
	{
		QMap<QString, QString> params
		{
			{ "artist", name },
			{ "autocorrect", "1" },
			{ "limit", QString::number (num) }
		};
		auto reply = Request ("artist.getSimilar", nam, params);
		connect (reply,
				SIGNAL (finished ()),
				this,
				SLOT (handleReplyFinished ()));
		connect (reply,
				SIGNAL (error (QNetworkReply::NetworkError)),
				this,
				SLOT (handleReplyError ()));
	}

	void PendingSimilarArtists::handleReplyFinished ()
	{
		auto reply = qobject_cast<QNetworkReply*> (sender ());
		reply->deleteLater ();

		QDomDocument doc;
		if (!doc.setContent (reply->readAll ()))
		{
			emit ready ();
			return;
		}

		const auto& artistElems = doc.elementsByTagName ("artist");
		if (artistElems.isEmpty ())
		{
			emit ready ();
			return;
		}

		QList<QPair<QString, double>> similar;
		for (int i = 0, size = artistElems.size (); i < size; ++i)
		{
			const auto& elem = artistElems.at (i).toElement ();
			similar << qMakePair (elem.firstChildElement ("name").text (),
						elem.firstChildElement ("match").text ().toDouble () * 100);
		}

		auto begin = similar.begin ();
		auto end = similar.end ();
		const int distance = std::distance (begin, end);
		if (distance > NumGet_)
			std::advance (begin, distance - NumGet_);

		InfosWaiting_ = std::distance (begin, end);

		for (auto i = begin; i != end; ++i)
		{
			QMap<QString, QString> params { { "artist", i->first } };
			AddLanguageParam (params);
			auto infoReply = Request ("artist.getInfo", NAM_, params);

			infoReply->setProperty ("Similarity", i->second);
			connect (infoReply,
					SIGNAL (finished ()),
					this,
					SLOT (handleInfoReplyFinished ()));
			connect (infoReply,
					SIGNAL (error (QNetworkReply::NetworkError)),
					this,
					SLOT (handleInfoReplyError ()));
		}
	}
}
}
