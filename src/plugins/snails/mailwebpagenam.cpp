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

#include "mailwebpagenam.h"
#include <util/network/customnetworkreply.h>
#include <util/sll/prelude.h>
#include <util/sll/visitor.h>
#include <util/threads/futures.h>
#include "accountthread.h"
#include "attachmentsfetcher.h"
#include "mailwebpage.h"

namespace LeechCraft::Snails
{
	MailWebPageNAM::MailWebPageNAM (ContextGetter getter, QObject *parent)
	: QNetworkAccessManager { parent }
	, CtxGetter_ { std::move (getter) }
	{
	}

	QNetworkReply* MailWebPageNAM::createRequest (Operation op, const QNetworkRequest& origReq, QIODevice *dev)
	{
		const auto& scheme = origReq.url ().scheme ();
		if (scheme == "http" || scheme == "https")
			return HandleNetworkRequest (op, origReq, dev);
		if (scheme == "cid")
			return HandleCIDRequest (op, origReq, dev);

		return QNetworkAccessManager::createRequest (op, origReq, dev);
	}

	QNetworkReply* MailWebPageNAM::HandleNetworkRequest (Operation, const QNetworkRequest& origReq, QIODevice*)
	{
		const auto reply = new Util::CustomNetworkReply { origReq.url (), this };
		reply->SetContent (QByteArray { "Blocked" });
		reply->setError (QNetworkReply::ContentAccessDenied,
				QString { "Blocked: %1" }.arg (origReq.url ().toString ()));
		return reply;
	}

	QNetworkReply* MailWebPageNAM::HandleCIDRequest (Operation op, const QNetworkRequest& origReq, QIODevice*)
	{
		const auto reply = new Util::CustomNetworkReply { origReq.url (), this };

		if (op != GetOperation)
		{
			qWarning () << Q_FUNC_INFO
					<< "unsupported operation"
					<< op
					<< origReq.url ();

			reply->SetContent (QByteArray { "Unsupported operation" });
			reply->setError (QNetworkReply::ContentOperationNotPermittedError,
					QString { "Unsupported operation %1 on %2" }
						.arg (op)
						.arg (origReq.url ().toString ()));
			return reply;
		}

		const auto& reqPath = origReq.url ().path ();

		const auto& ctx = CtxGetter_ ();
		// FIXME
		// This shall be cid-based, but we don't necessarily have cid at this point,
		// so let's use the next best approximation.
		const auto& atts = ctx.MsgInfo_.Attachments_;
		const auto attPos = std::find_if (atts.begin (), atts.end (),
				[&reqPath] (const AttDescr& att)
				{
					const auto& name = att.GetName ();
					return !name.isEmpty () &&
							(name.contains (reqPath) || reqPath.contains (name));
				});

		if (attPos == atts.end ())
		{
			qWarning () << Q_FUNC_INFO
					<< "attachment"
					<< reqPath
					<< "not found among"
					<< Util::Map (atts, &AttDescr::GetName);

			reply->SetContent (QByteArray { "Attachment not found" });
			reply->setError (QNetworkReply::ContentNotFoundError,
					QString { "Attachment %1 not found" }
							.arg (origReq.url ().toString ()));
		}

		qDebug () << Q_FUNC_INFO
				<< "fetching attachment"
				<< attPos->GetName ();

		const auto af = std::make_shared<AttachmentsFetcher> (ctx.Acc_,
				ctx.MsgInfo_.Folder_, ctx.MsgInfo_.FolderId_, QStringList { attPos->GetName () });

		reply->setHeader (QNetworkRequest::ContentLengthHeader, attPos->GetSize ());
		reply->setHeader (QNetworkRequest::ContentTypeHeader, attPos->GetType () + '/' + attPos->GetSubType ());

		Util::Sequence (reply, af->GetFuture ()) >>
				Util::Visitor
				{
					[reply, af] (const AttachmentsFetcher::FetchResult& result)
					{
						qDebug () << Q_FUNC_INFO << "done fetching attachment into" << result.Paths_;

						QFile file { result.Paths_.value (0) };
						reply->SetContent (QFile { result.Paths_.value (0) }.readAll ());
					},
					[reply, af] (auto)
					{
						reply->SetContent (QByteArray { "Unable to fetch attachment" });
						reply->setError (QNetworkReply::InternalServerError,
								QString { "Unable to fetch attachment" });
					}
				};

		return reply;
	}
}
