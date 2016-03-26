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

#pragma once

#include <QObject>
#include <QPair>
#include <QList>

class QXmlStreamWriter;
class QWebView;
class QUrl;
class QImage;

namespace LeechCraft
{
namespace Poshuku
{
class IProxyObject;

namespace SpeedDial
{
	class ImageCache;
	class CustomSitesManager;

	typedef QList<QPair<QUrl, QString>> TopList_t;

	struct LoadResult;

	class ViewHandler : public QObject
	{
		Q_OBJECT

		QWebView * const View_;
		QObject * const BrowserWidget_;
		ImageCache * const ImageCache_;
		IProxyObject * const PoshukuProxy_;

		bool IsLoading_ = false;
	public:
		ViewHandler (QWebView*, QObject*, ImageCache*, CustomSitesManager*, IProxyObject*);
	private:
		void LoadStatistics ();
		void WriteTables (const QList<QPair<QString, TopList_t>>&);
		void WriteTable (QXmlStreamWriter&, const TopList_t&, size_t, size_t, const QString&);
	private slots:
		void handleLoadStarted ();

		void handleSnapshot (const QUrl&, const QImage&);
	};
}
}
}
