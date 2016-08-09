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

#ifndef PLUGINS_POSHUKU_CUSTOMWEBVIEW_H
#define PLUGINS_POSHUKU_CUSTOMWEBVIEW_H
#include <qwebview.h>
#include <interfaces/structures.h>
#include <interfaces/core/ihookproxy.h>
#include "interfaces/poshuku/poshukutypes.h"
#include "pageformsdata.h"

class QTimer;

namespace LeechCraft
{
namespace Poshuku
{
	class BrowserWidget;

	class CustomWebView : public QWebView
	{
		Q_OBJECT

		BrowserWidget *Browser_;
		QString PreviousEncoding_;
		QTimer *ScrollTimer_;
		double ScrollDelta_;
		double AccumulatedScrollShift_;
	public:
		CustomWebView (QWidget* = 0);
		virtual ~CustomWebView ();

		void SetBrowserWidget (BrowserWidget*);
		void Load (const QString&, QString = QString ());
		void Load (const QUrl&, QString = QString ());
		void Load (const QNetworkRequest&,
				QNetworkAccessManager::Operation = QNetworkAccessManager::GetOperation,
				const QByteArray& = QByteArray ());

		/** This function is equivalent to url.toString() if the url is
		 * all in UTF-8. But if the site is in another encoding,
		 * QUrl::toString() returns a bad, unreadable and, moreover,
		 * unusable string. In this case, this function converts the url
		 * to its percent-encoding representation.
		 *
		 * @param[in] url The possibly non-UTF-8 URL.
		 * @return The \em url converted to Unicode.
		 */
		QString URLToProperString (const QUrl& url);
	protected:
		virtual void mousePressEvent (QMouseEvent*);
		virtual void contextMenuEvent (QContextMenuEvent*);
		virtual void keyReleaseEvent (QKeyEvent*);
	private:
		void NavigatePlugins ();
		void NavigateHome ();
	private slots:
		void remakeURL (const QUrl&);
		void handleLoadFinished (bool);
		void handleFrameState (QWebFrame*, QWebHistoryItem*);
		void openLinkHere ();
		void openLinkInNewTab ();
		void saveLink ();
		void subscribeToLink ();
		void bookmarkLink ();
		void copyLink ();
		void openImageHere ();
		void openImageInNewTab ();
		void saveImage ();
		void savePixmap ();
		void copyImage ();
		void copyImageLocation ();
		void searchSelectedText ();
		void renderSettingsChanged ();
		void handleAutoscroll ();
	signals:
		void urlChanged (const QString&);
		void gotEntity (const LeechCraft::Entity&);
		void delegateEntity (const LeechCraft::Entity&, int*, QObject**);
		void couldHandle (const LeechCraft::Entity&, bool*);
		void addToFavorites (const QString&, const QString&);
		void printRequested (QWebFrame*);
		void closeRequested ();
		void storeFormData (const PageFormsData_t&);

		void navigateRequested (const QUrl&);

		void zoomChanged ();

		// Hook support signals
		void hookWebViewContextMenu (LeechCraft::IHookProxy_ptr,
				QWebView*, QContextMenuEvent*,
				const QWebHitTestResult&, QMenu*,
				WebViewCtxMenuStage);
	};
}
}

#endif
