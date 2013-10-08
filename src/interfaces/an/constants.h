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

#include <QString>
#include <QFlags>

namespace LeechCraft
{
namespace AN
{
	/** @brief Category of Instant Messaging-related events.
	 */
	const QString CatIM = "org.LC.AdvNotifications.IM";
	/** @brief Another user has requested our user's attention.
	 */
	const QString TypeIMAttention = "org.LC.AdvNotifications.IM.AttentionDrawn";
	/** @brief Another user has sent our user a file.
	 */
	const QString TypeIMIncFile = "org.LC.AdvNotifications.IM.IncomingFile";
	/** @brief User has received a message in a standard one-to-one chat.
	 */
	const QString TypeIMIncMsg = "org.LC.AdvNotifications.IM.IncomingMessage";
	/** @brief User has been highlighted in a multiuser chat.
	 *
	 * The primary difference from TypeIMMUCMsg is that our user must be
	 * explicitly mentioned in another user's message for this event.
	 *
	 * @sa TypeIMMUCMsg
	 */
	const QString TypeIMMUCHighlight = "org.LC.AdvNotifications.IM.MUCHighlightMessage";
	/** @brief User has been invited to a multiuser chat.
	 */
	const QString TypeIMMUCInvite = "org.LC.AdvNotifications.IM.MUCInvitation";
	/** @brief A message has been sent to a multiuser chat.
	 *
	 * This event should be emitted for each MUC message, even for those
	 * our user isn't mentioned in.
	 *
	 * @sa TypeIMMUCHighlight
	 */
	const QString TypeIMMUCMsg = "org.LC.AdvNotifications.IM.MUCMessage";
	/** @brief Another user in our user's contact list has changed its
	 * status.
	 */
	const QString TypeIMStatusChange = "org.LC.AdvNotifications.IM.StatusChange";
	/** @brief Another user has granted subscription to our user.
	 */
	const QString TypeIMSubscrGrant = "org.LC.AdvNotifications.IM.Subscr.Granted";
	/** @brief Another user has revoked subscription from our user.
	 */
	const QString TypeIMSubscrRevoke = "org.LC.AdvNotifications.IM.Subscr.Revoked";
	/** @brief Another user has requested subscription from our user.
	 */
	const QString TypeIMSubscrRequest = "org.LC.AdvNotifications.IM.Subscr.Requested";
	/** @brief Another user has subscribed to our user.
	 */
	const QString TypeIMSubscrSub = "org.LC.AdvNotifications.IM.Subscr.Subscribed";
	/** @brief Another user has unsubscribed from our user.
	 */
	const QString TypeIMSubscrUnsub = "org.LC.AdvNotifications.IM.Subscr.Unsubscribed";

	/** @brief Category of Organizer-related events.
	 */
	const QString CatOrganizer = "org.LC.AdvNotifications.Organizer";
	/** @brief An event due date is coming.
	 */
	const QString TypeOrganizerEventDue = "org.LC.AdvNotifications.Organizer.EventDue";

	/** @brief Category of Downloads-related events.
	 */
	const QString CatDownloads = "org.LC.AdvNotifications.Downloads";
	/** @brief A download has been finished successfully without errors.
	 */
	const QString TypeDownloadFinished = "org.LC.AdvNotifications.Downloads.DownloadFinished";
	/** @brief A download has been failed.
	 */
	const QString TypeDownloadError = "org.LC.AdvNotifications.Downloads.DownloadError";

	/** @brief Category of package manager-related events.
	 */
	const QString CatPackageManager = "org.LC.AdvNotifications.PackageManager";
	/** @brief A package has been updated.
	 */
	const QString TypePackageUpdated = "org.LC.AdvNotifications.PackageManager.PackageUpdated";

	/** @brief Generic notifications that don't fit into any other category.
	 */
	const QString CatGeneric = "org.LC.AdvNotifications.Generic";

	/** @brief Generic category for generic notifications.
	 */
	const QString TypeGeneric = "org.LC.AdvNotifications.Generic.Generic";

	enum NotifyFlag
	{
		NotifyNone			= 0,
		NotifySingleShot	= 1 << 0,
		NotifyTransient		= 1 << 1,
		NotifyPersistent	= 1 << 2,
		NotifyAudio			= 1 << 3
	};
	Q_DECLARE_FLAGS (NotifyFlags, NotifyFlag);

	Q_DECLARE_OPERATORS_FOR_FLAGS (NotifyFlags);
}
}
