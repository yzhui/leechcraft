/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2013  Georg Rudoy
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

#pragma once

#include <QObject>
#include <QMap>

namespace LeechCraft
{
namespace LMP
{
	class ISyncPlugin;
	class TranscodeManager;
	struct TranscodingParams;

	class SyncManagerBase : public QObject
	{
		Q_OBJECT

	protected:
		TranscodeManager *Transcoder_;

		int TranscodedCount_;
		int TotalTCCount_;
		bool WereTCErrors_;

		int CopiedCount_;
		int TotalCopyCount_;
	public:
		SyncManagerBase (QObject* = 0);
	protected:
		void AddFiles (const QStringList&, const TranscodingParams&);
		void HandleFileTranscoded (const QString&, const QString&);
	private:
		void CheckTCFinished ();
		void CheckUploadFinished ();
	protected slots:
		void handleStartedTranscoding (const QString&);
		virtual void handleFileTranscoded (const QString&, const QString&, QString) = 0;
		void handleFileTCFailed (const QString&);
		void handleStartedCopying (const QString&);
		void handleFinishedCopying ();
		void handleCopyProgress (qint64, qint64);
		void handleErrorCopying (const QString&, const QString&);
	signals:
		void uploadLog (const QString&);

		void transcodingProgress (int, int, SyncManagerBase*);
		void uploadProgress (int, int, SyncManagerBase*);
		void singleUploadProgress (int, int, SyncManagerBase*);
	};
}
}
