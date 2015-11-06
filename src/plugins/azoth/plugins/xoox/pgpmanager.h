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

#include <QMap>
#include <QByteArray>
#include <QtCrypto>
#include <QXmppClientExtension.h>
#include <QXmppMessage.h>
#include <QXmppPresence.h>

namespace LeechCraft
{
namespace Azoth
{
namespace Xoox
{
	class PgpManager : public QXmppClientExtension
	{
		Q_OBJECT

		// private key, used for decrypting messages
		QCA::PGPKey PrivateKey_;

		// map of userIDs and corresponding public keys
		// each user ID is a completely arbitrary value, one can use JIDs for this purpose
		QMap<QString, QCA::PGPKey> PublicKeys_;
	public:
		QCA::PGPKey PublicKey (const QString&) const;
		void SetPublicKey (const QString&, const QCA::PGPKey&);

		QCA::PGPKey PrivateKey () const;
		void SetPrivateKey (const QCA::PGPKey&);

		QByteArray EncryptBody (const QCA::PGPKey&, const QByteArray&);
		QByteArray SignMessage (const QByteArray&);
		QByteArray SignPresence (const QByteArray&);

		QByteArray DecryptBody (const QByteArray&);
		bool IsValidSignature (const QCA::PGPKey&, const QByteArray&, const QByteArray&);

		bool handleStanza (const QDomElement&);
	signals:
		void encryptedMessageReceived (const QString&, const QString&);
		void signedMessageReceived (const QString&);
		void signedPresenceReceived (const QString&);
		void invalidSignatureReceived (const QString&);
	};
}
}
}
