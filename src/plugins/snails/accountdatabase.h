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

#include <optional>
#include <memory>
#include <QObject>
#include <QStringList>
#include <QMap>
#include <QSqlDatabase>
#include <util/db/oral/oralfwd.h>

class QDir;

namespace LeechCraft
{
namespace Util
{
	class DBLock;
}

namespace Snails
{
	class Account;
	struct MessageInfo;
	struct MessageBodies;

	class AccountDatabase
	{
		QSqlDatabase DB_;
	public:
		struct Message;
		struct Address;
		struct Attachment;

		struct MessageBodies;

		struct Folder;
		struct Msg2Folder;
		struct MsgHeader;
	private:
		Util::oral::ObjectInfo_ptr<Message> Messages_;
		Util::oral::ObjectInfo_ptr<Address> Addresses_;
		Util::oral::ObjectInfo_ptr<Attachment> Attachments_;

		Util::oral::ObjectInfo_ptr<MessageBodies> MessagesBodies_;

		Util::oral::ObjectInfo_ptr<Folder> Folders_;
		Util::oral::ObjectInfo_ptr<Msg2Folder> Msg2Folder_;
		Util::oral::ObjectInfo_ptr<MsgHeader> MsgHeader_;

		QMap<QStringList, int> KnownFolders_;
	public:
		AccountDatabase (const QDir&, const Account*);

		Util::DBLock BeginTransaction ();

		QList<QByteArray> GetIDs (const QStringList& folder);
		std::optional<QByteArray> GetLastID (const QStringList& folder);
		int GetMessageCount (const QStringList& folder);
		int GetUnreadMessageCount (const QStringList& folder);
		int GetMessageCount ();

		QList<MessageInfo> GetMessageInfos (const QStringList& folder);
		std::optional<MessageInfo> GetMessageInfo (const QStringList& folder, const QByteArray& msgId);

		void AddMessage (const MessageInfo&);
		void RemoveMessage (const QByteArray& msgId, const QStringList& folder);

		void SaveMessageBodies (const QStringList& folder, const QByteArray& msgId, const Snails::MessageBodies&);
		std::optional<Snails::MessageBodies> GetMessageBodies (const QStringList& folder, const QByteArray& msgId);

		std::optional<bool> IsMessageRead (const QByteArray& msgId, const QStringList& folder);
		void SetMessageRead (const QByteArray& msgId, const QStringList& folder, bool read);

		void SetMessageHeader (const QByteArray& msgId, const QByteArray& header);
		std::optional<QByteArray> GetMessageHeader (const QByteArray& uniqueMsgId) const;
		std::optional<QByteArray> GetMessageHeader (const QStringList& folderId, const QByteArray& msgId) const;

		std::optional<int> GetMsgTableId (const QByteArray& uniqueId);
		std::optional<int> GetMsgTableId (const QByteArray& msgId, const QStringList& folder);
	private:
		int AddMessageUnfoldered (const MessageInfo&);
		void AddMessageToFolder (int msgTableId, int folderTableId, const QByteArray& msgId);

		int AddFolder (const QStringList&);
		int GetFolder (const QStringList&) const;
		void LoadKnownFolders ();
	};
}
}
