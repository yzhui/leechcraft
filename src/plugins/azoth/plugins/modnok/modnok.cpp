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

#include "modnok.h"
#include <QIcon>
#include <QTextDocument>
#include <QProcess>
#include <QTranslator>
#include <interfaces/azoth/imessage.h>
#include <interfaces/azoth/irichtextmessage.h>
#include <util/util.h>
#include <util/sys/paths.h>
#include <xmlsettingsdialog/xmlsettingsdialog.h>
#include "xmlsettingsmanager.h"

namespace LeechCraft
{
namespace Azoth
{
namespace Modnok
{
	void Plugin::Init (ICoreProxy_ptr)
	{
		Translator_.reset (Util::InstallTranslator ("azoth_modnok"));

		SettingsDialog_.reset (new Util::XmlSettingsDialog);
		SettingsDialog_->RegisterObject (&XmlSettingsManager::Instance (),
				"azothmodnoksettings.xml");

		XmlSettingsManager::Instance ().RegisterObject ("RenderCacheSize",
				this, "handleCacheSize");
		handleCacheSize ();

		QList<QByteArray> invalidates;
		invalidates << "HorizontalDPI"
				<< "VerticalDPI"
				<< "TextColor";
		XmlSettingsManager::Instance ().RegisterObject (invalidates, this, "clearCaches");

		QStringList candidates;
		candidates << "/usr/local/bin"
				<< "/usr/bin"
				<< "/usr/local/share/leechcraft/azoth"
				<< "/usr/share/leechcraft/azoth";

		Q_FOREACH (const QString& dir, candidates)
		{
			const QString& path = dir + "/lc_azoth_modnok_latexconvert.sh";
			QFileInfo info (path);
			if (info.exists () &&
					info.isReadable () &&
					info.isExecutable ())
			{
				ConvScriptPath_ = path;
				break;
			}
		}
	}

	void Plugin::SecondInit ()
	{
	}

	QByteArray Plugin::GetUniqueID () const
	{
		return "org.LeechCraft.Azoth.Modnok";
	}

	void Plugin::Release ()
	{
	}

	QString Plugin::GetName () const
	{
		return "Azoth Modnok";
	}

	QString Plugin::GetInfo () const
	{
		return tr ("Azoth Modnok allows one to render LaTeX formulas and display them in the chat window.");
	}

	QIcon Plugin::GetIcon () const
	{
		static QIcon icon ("lcicons:/plugins/azoth/plugins/modnok/resources/images/modnok.svg");
		return icon;
	}

	QSet<QByteArray> Plugin::GetPluginClasses () const
	{
		QSet<QByteArray> result;
		result << "org.LeechCraft.Plugins.Azoth.Plugins.IGeneralPlugin";
		return result;
	}

	Util::XmlSettingsDialog_ptr Plugin::GetSettingsDialog () const
	{
		return SettingsDialog_;
	}

	namespace
	{
		bool IsSecure (const QString& formula)
		{
			static QRegExp rx ("\\\\(def|let|futurelet|newcommand|renewcomment|else|fi|write|input|include"
					"|chardef|catcode|makeatletter|noexpand|toksdef|every|errhelp|errorstopmode|scrollmode|nonstopmode|batchmode"
					"|read|csname|newhelp|relax|afterground|afterassignment|expandafter|noexpand|special|command|loop|repeat|toks"
					"|output|line|mathcode|name|item|section|mbox|DeclareRobustCommand)[^a-zA-Z]");
			return !formula.contains (rx);
		}
	}

	QImage Plugin::GetRenderedImage (const QString& formula)
	{
		if (FormulasCache_.contains (formula))
			return *FormulasCache_.object (formula);

		const QString& filename = Util::GetTemporaryName ("lc_azoth_modnok.XXXXXX.png");

		const int dpiX = XmlSettingsManager::Instance ().property ("HorizontalDPI").toInt ();
		const int dpiY = XmlSettingsManager::Instance ().property ("VerticalDPI").toInt ();
		const auto& color = XmlSettingsManager::Instance ().property ("TextColor").toString ();

		QStringList args;
		args << QString ("-r %1x%2").arg (dpiX).arg (dpiY);
		args << QString ("-o %1").arg (filename);
		args << "-t" << color;
		if (color == "white")
			args << "-b" << "black";

		// TODO
		const QString& incPath = QString ();
		if (!incPath.isEmpty ())
			args << (QString ("-x ") + incPath);

		args << formula;

		QProcess process;
		process.start (ConvScriptPath_, args);
		process.waitForFinished (5000);

		QImage img (filename);
		FormulasCache_.insert (formula, new QImage (filename), img.byteCount () / 1024);

		QFile (filename).remove ();

		return img;
	}

	QString Plugin::HandleBody (QString body)
	{
		QRegExp rx ("\\$\\$.+\\$\\$");
		rx.setMinimal (true);

		int pos = 0;

		QMap<QString, QString> replaceMap;
		while (pos >= 0 && pos < body.size ())
		{
			pos = rx.indexIn (body, pos);

			if (pos < 0)
				break;

			const QString& match = rx.cap (0);
			pos += rx.matchedLength ();

			QString formula = match;
			formula.remove ("$$");
			formula = formula.trimmed ();
			if (formula.isEmpty () || !IsSecure (formula))
				continue;

			formula.replace ("&lt;", "<");
			formula.replace ("&gt;", ">");
			formula.replace ("&quot;", "\"");
			formula.replace ("&amp;", "&");

			const QImage& rendered = GetRenderedImage (formula);
			if (rendered.isNull ())
				continue;

			replaceMap [match] = Util::GetAsBase64Src (rendered);
		}

		if (replaceMap.isEmpty ())
			return body;

		Q_FOREACH (const QString& key, replaceMap.keys ())
		{
			QString escFormula = key;
			escFormula.replace ('\"', "&quot;");
			escFormula.remove ("$$");
			const QString img = QString ("<img src=\"%1\" alt=\"%2\" style=\"vertical-align: middle;\" />")
					.arg (replaceMap [key])
					.arg (escFormula.trimmed ().simplified ());
			body.replace (key, img);
		}

		return body;
	}

	void Plugin::hookFormatBodyEnd (IHookProxy_ptr proxy, QObject*)
	{
		if (ConvScriptPath_.isEmpty ())
			return;

		if (!XmlSettingsManager::Instance ()
				.property ("OnDisplayRendering").toBool ())
			return;

		const QString& body = proxy->GetValue ("body").toString ();

		if (!body.contains ("$$"))
			return;

		const QString newBody = HandleBody (body);
		if (body != newBody)
			proxy->SetValue ("body", newBody);
	}

	void Plugin::hookGonnaHandleSmiles (IHookProxy_ptr proxy,
			QString body, QString)
	{
		if (ConvScriptPath_.isEmpty ())
			return;

		if (!XmlSettingsManager::Instance ()
				.property ("OnDisplayRendering").toBool ())
			return;

		if (!body.contains ("$$"))
			return;

		proxy->CancelDefault ();
	}

	void Plugin::hookMessageCreated (IHookProxy_ptr, QObject*, QObject *msgObj)
	{
		if (ConvScriptPath_.isEmpty ())
			return;

		if (!XmlSettingsManager::Instance ()
				.property ("SubstituteOutgoing").toBool ())
			return;

		IMessage *msg = qobject_cast<IMessage*> (msgObj);
		if (!msg)
		{
			qWarning () << Q_FUNC_INFO
					<< "message"
					<< msgObj
					<< "doesn't implement IMessage";
			return;
		}

		IRichTextMessage *richMsg = qobject_cast<IRichTextMessage*> (msgObj);
		if (!richMsg)
			return;

		QString body = richMsg->GetRichBody ();
		if (body.isEmpty ())
			body = msg->GetBody ();

		if (!body.contains ("$$"))
			return;

		const QString newBody = HandleBody (body);
		if (newBody == body)
			return;

		if (XmlSettingsManager::Instance ()
				.property ("WarnInOutgoing").toBool ())
			msg->SetBody (msg->GetBody () + " (this message contains "
						"inline formulas, enable XHTML-IM to view them)");

		richMsg->SetRichBody (newBody);
	}

	void Plugin::clearCaches ()
	{
		FormulasCache_.clear ();
	}

	void Plugin::handleCacheSize ()
	{
		const int mibs = XmlSettingsManager::Instance ()
				.property ("RenderCacheSize").toInt ();
		FormulasCache_.setMaxCost (mibs * 1024);
	}
}
}
}

LC_EXPORT_PLUGIN (leechcraft_azoth_modnok, LeechCraft::Azoth::Modnok::Plugin);
