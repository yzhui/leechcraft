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

#include "storage.h"
#include <stdexcept>
#include <QFile>
#include <QApplication>
#include <QtConcurrentMap>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDataStream>
#include <QtConcurrentRun>
#include <util/db/dblock.h>
#include <util/sys/paths.h>
#include <util/threads/futures.h>
#include "xmlsettingsmanager.h"
#include "account.h"
#include "accountdatabase.h"

namespace LeechCraft
{
namespace Snails
{
	namespace
	{
		template<typename T>
		QByteArray Serialize (const T& t)
		{
			QByteArray result;
			QDataStream stream (&result, QIODevice::WriteOnly);
			stream << t;
			return result;
		}
	}

	Storage::Storage (QObject *parent)
	: QObject (parent)
	{
		SDir_ = Util::CreateIfNotExists ("snails/storage");
	}

	namespace
	{
		QList<Message_ptr> MessageSaverProc (QList<Message_ptr> msgs, const QDir& dir)
		{
			for (const auto& msg : msgs)
			{
				if (msg->GetFolderID ().isEmpty ())
					continue;

				const QString dirName = msg->GetFolderID ().toHex ().right (3);

				QDir msgDir = dir;
				if (!dir.exists (dirName))
					msgDir.mkdir (dirName);
				if (!msgDir.cd (dirName))
				{
					qWarning () << Q_FUNC_INFO
							<< "unable to cd into"
							<< msgDir.filePath (dirName);
					continue;
				}

				QFile file (msgDir.filePath (msg->GetFolderID ().toHex ()));
				if (!file.open (QIODevice::WriteOnly))
				{
					qWarning () << Q_FUNC_INFO
							<< "unable to open file"
							<< file.fileName ()
							<< file.errorString ();
					continue;
				}

				file.write (qCompress (msg->Serialize (), 9));
			}

			return msgs;
		}
	}

	void Storage::SaveMessages (Account *acc, const QStringList& folder, const QList<Message_ptr>& msgs)
	{
		auto dir = DirForAccount (acc);
		for (const auto& elem : folder)
		{
			const auto& subdir = elem.toUtf8 ().toHex ();
			if (!dir.exists (subdir))
				dir.mkdir (subdir);

			if (!dir.cd (subdir))
			{
				qWarning () << Q_FUNC_INFO
						<< "unable to cd to"
						<< dir.filePath (subdir);
				throw std::runtime_error ("Unable to cd to the directory");
			}
		}

		for (const auto& msg : msgs)
			PendingSaveMessages_ [acc] [msg->GetFolderID ()] = msg;

		Util::Sequence (this, QtConcurrent::run (MessageSaverProc, msgs, dir)) >>
				[this, acc] (const QList<Message_ptr>& messages)
				{
					auto& hash = PendingSaveMessages_ [acc];

					for (const auto& msg : messages)
						hash.remove (msg->GetFolderID ());
				};

		for (const auto& msg : msgs)
		{
			if (msg->GetFolderID ().isEmpty ())
				continue;

			AddMessage (msg, acc);
		}
	}

	MessageSet Storage::LoadMessages (Account *acc)
	{
		MessageSet result;

		const QDir& dir = DirForAccount (acc);
		for (const auto& str : dir.entryList (QDir::NoDotAndDotDot | QDir::Dirs))
		{
			QDir subdir = dir;
			if (!subdir.cd (str))
			{
				qWarning () << Q_FUNC_INFO
						<< "unable to cd to"
						<< str;
				continue;
			}

			for (const auto& str : subdir.entryList (QDir::NoDotAndDotDot | QDir::Files))
			{
				QFile file (subdir.filePath (str));
				if (!file.open (QIODevice::ReadOnly))
				{
					qWarning () << Q_FUNC_INFO
							<< "unable to open"
							<< str
							<< file.errorString ();
					continue;
				}

				const auto& msg = std::make_shared<Message> ();
				try
				{
					msg->Deserialize (qUncompress (file.readAll ()));
				}
				catch (const std::exception& e)
				{
					qWarning () << Q_FUNC_INFO
							<< "error deserializing the message from"
							<< file.fileName ()
							<< e.what ();
					continue;
				}
				result << msg;
			}
		}

		for (const auto& msg : PendingSaveMessages_ [acc])
			result << msg;

		return result;
	}

	Message_ptr Storage::LoadMessage (Account *acc, const QStringList& folder, const QByteArray& id)
	{
		if (PendingSaveMessages_ [acc].contains (id))
			return PendingSaveMessages_ [acc] [id];

		auto dir = DirForAccount (acc);
		for (const auto& elem : folder)
		{
			const auto& subdir = elem.toUtf8 ().toHex ();
			if (!dir.cd (subdir))
			{
				qWarning () << Q_FUNC_INFO
						<< "unable to cd to"
						<< dir.filePath (subdir);
				throw std::runtime_error ("Unable to cd to the directory");
			}
		}

		return LoadMessage (acc, dir, id);
	}

	Message_ptr Storage::LoadMessage (Account *acc, QDir dir, const QByteArray& id) const
	{
		if (!dir.cd (id.toHex ().right (3)))
		{
			qWarning () << Q_FUNC_INFO
					<< "unable to cd to"
					<< dir.filePath (id.toHex ().right (3));
			throw std::runtime_error ("Unable to cd to the directory");
		}

		QFile file (dir.filePath (id.toHex ()));
		if (!file.open (QIODevice::ReadOnly))
		{
			qWarning () << Q_FUNC_INFO
					<< "unable to open"
					<< file.fileName ()
					<< file.errorString ();
			throw std::runtime_error ("Unable to open the message file");
		}

		const auto& msg = std::make_shared<Message> ();
		try
		{
			msg->Deserialize (qUncompress (file.readAll ()));
		}
		catch (const std::exception& e)
		{
			qWarning () << Q_FUNC_INFO
					<< "error deserializing the message from"
					<< file.fileName ()
					<< e.what ();
			throw;
		}

		return msg;
	}

	QList<Message_ptr> Storage::LoadMessages (Account *acc, const QStringList& folder, const QList<QByteArray>& ids)
	{
		auto rootDir = DirForAccount (acc);
		for (const auto& elem : folder)
		{
			const auto& subdir = elem.toUtf8 ().toHex ();
			if (!rootDir.cd (subdir) &&
					!(rootDir.mkpath (subdir) && rootDir.cd (subdir)))
			{
				qWarning () << Q_FUNC_INFO
						<< "unable to cd to"
						<< rootDir.filePath (subdir);
				throw std::runtime_error ("Unable to cd to the directory");
			}
		}

		QList<Message_ptr> result;
		auto future = QtConcurrent::mapped (ids,
				std::function<Message_ptr (QByteArray)>
				{
					[this, acc, rootDir] (const QByteArray& id)
						{ return LoadMessage (acc, rootDir, id); }
				});

		for (const auto& item : future.results ())
			result << item;

		return result;
	}

	QList<QByteArray> Storage::LoadIDs (Account *acc, const QStringList& folder)
	{
		return BaseForAccount (acc)->GetIDs (folder);
	}

	boost::optional<QByteArray> Storage::GetLastID (Account *acc, const QStringList& folder)
	{
		return BaseForAccount (acc)->GetLastID (folder);
	}

	void Storage::RemoveMessage (Account *acc, const QStringList& folder, const QByteArray& id)
	{
		PendingSaveMessages_ [acc].remove (id);

		BaseForAccount (acc)->RemoveMessage (id, folder);
		RemoveMessageFile (acc, folder, id);
	}

	int Storage::GetNumMessages (Account *acc, const QStringList& folder)
	{
		return BaseForAccount (acc)->GetMessageCount (folder);
	}

	int Storage::GetNumUnread (Account *acc, const QStringList& folder)
	{
		return BaseForAccount (acc)->GetUnreadMessageCount (folder);
	}

	bool Storage::IsMessageRead (Account *acc, const QStringList& folder, const QByteArray& id)
	{
		return BaseForAccount (acc)->IsMessageRead (id, folder).value ();
	}

	void Storage::SetMessagesRead (Account *acc,
			const QStringList& folder, const QList<QByteArray>& folderIds, bool read)
	{
		if (folderIds.isEmpty ())
			return;

		auto base = BaseForAccount (acc);

		qDebug () << "SetRead" << folderIds.size ();
		auto ts = base->BeginTransaction ();
		for (const auto& id : folderIds)
			base->SetMessageRead (id, folder, read);
		ts.Good ();
		qDebug () << "done";
	}

	void Storage::RemoveMessageFile (Account *acc, const QStringList& folder, const QByteArray& id)
	{
		auto dir = DirForAccount (acc);
		for (const auto& elem : folder)
		{
			const auto& subdir = elem.toUtf8 ().toHex ();
			if (!dir.cd (subdir))
			{
				qWarning () << Q_FUNC_INFO
						<< "unable to cd to"
						<< dir.filePath (subdir);
				throw std::runtime_error ("Unable to cd to the directory");
			}
		}

		if (!dir.cd (id.toHex ().right (3)))
		{
			qWarning () << Q_FUNC_INFO
					<< "unable to cd to"
					<< dir.filePath (id.toHex ().right (3));
			throw std::runtime_error ("Unable to cd to the directory");
		}

		QFile file (dir.filePath (id.toHex ()));
		if (!file.exists ())
			return;

		if (!file.remove ())
		{
			qWarning () << Q_FUNC_INFO
					<< "unable to remove the file:"
					<< file.errorString ();
			throw std::runtime_error ("Unable to remove the file");
		}
	}

	QDir Storage::DirForAccount (const Account *acc) const
	{
		const QByteArray& id = acc->GetID ().toHex ();

		QDir dir = SDir_;
		if (!dir.exists (id))
			dir.mkdir (id);
		if (!dir.cd (id))
		{
			qWarning () << Q_FUNC_INFO
					<< "unable to cd into"
					<< dir.filePath (id);
			throw std::runtime_error ("Unable to cd to the dir");
		}

		return dir;
	}

	AccountDatabase_ptr Storage::BaseForAccount (const Account *acc)
	{
		if (AccountBases_.contains (acc))
			return AccountBases_ [acc];

		const auto& dir = DirForAccount (acc);
		const auto& base = std::make_shared<AccountDatabase> (dir, acc);
		AccountBases_ [acc] = base;
		return base;
	}

	void Storage::AddMessage (Message_ptr msg, Account *acc)
	{
		const auto& base = BaseForAccount (acc);
		base->AddMessage (msg);
	}
}
}
