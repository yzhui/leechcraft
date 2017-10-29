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

#include "contactlistdelegate.h"
#include <algorithm>
#include <QPainter>
#include <QPixmap>
#include <QApplication>
#include <QAbstractProxyModel>
#include <QTreeView>
#include <util/sys/resourceloader.h>
#include <util/xpc/defaulthookproxy.h>
#include <interfaces/media/audiostructs.h>
#include "interfaces/azoth/iclentry.h"
#include "interfaces/azoth/isupportgeolocation.h"
#include "interfaces/azoth/iaccount.h"
#include "interfaces/azoth/iextselfinfoaccount.h"
#include "interfaces/azoth/ihavecontacttune.h"
#include "interfaces/azoth/ihavecontactmood.h"
#include "interfaces/azoth/ihavecontactactivity.h"
#include "interfaces/azoth/moodinfo.h"
#include "core.h"
#include "xmlsettingsmanager.h"
#include "util.h"
#include "resourcesmanager.h"

namespace LeechCraft
{
namespace Azoth
{
	const int CPadding = 2;

	ContactListDelegate::ContactListDelegate (QTreeView* parent)
	: QStyledItemDelegate (parent)
	, ContactHeight_ (24)
	, View_ (parent)
	{
		handleShowAvatarsChanged ();
		handleShowClientIconsChanged ();
		handleActivityIconsetChanged ();
		handleMoodIconsetChanged ();
		handleSystemIconsetChanged ();
		handleShowStatusesChanged ();
		handleHighlightGroupsChanged ();
		handleContactHeightChanged ();

		XmlSettingsManager::Instance ().RegisterObject ("ShowAvatars",
				this, "handleShowAvatarsChanged");
		XmlSettingsManager::Instance ().RegisterObject ("ShowClientIcons",
				this, "handleShowClientIconsChanged");
		XmlSettingsManager::Instance ().RegisterObject ("ActivityIcons",
				this, "handleActivityIconsetChanged");
		XmlSettingsManager::Instance ().RegisterObject ("MoodIcons",
				this, "handleMoodIconsetChanged");
		XmlSettingsManager::Instance ().RegisterObject ("SystemIcons",
				this, "handleSystemIconsetChanged");
		XmlSettingsManager::Instance ().RegisterObject ("ShowStatuses",
				this, "handleShowStatusesChanged");
		XmlSettingsManager::Instance ().RegisterObject ("HighlightGroups",
				this, "handleHighlightGroupsChanged");
		XmlSettingsManager::Instance ().RegisterObject ("RosterContactHeight",
				this, "handleContactHeightChanged");

		Core::Instance ().RegisterHookable (this);
	}

	void ContactListDelegate::paint (QPainter *painter,
			const QStyleOptionViewItem& sopt, const QModelIndex& index) const
	{
		auto o = sopt;
		Core::CLEntryType type = index.data (Core::CLREntryType).value<Core::CLEntryType> ();

		painter->save ();

		switch (type)
		{
		case Core::CLETAccount:
			DrawAccount (painter, o, index);
			break;
		case Core::CLETCategory:
			DrawCategory (painter, o, index);
			break;
		case Core::CLETContact:
			o.rect.adjust (-1.2 * View_->indentation (), 0, 0, 0);
			DrawContact (painter, o, index);
			break;
		}

		painter->restore ();
	}

	QSize ContactListDelegate::sizeHint (const QStyleOptionViewItem& option, const QModelIndex& index) const
	{
		QSize size = QStyledItemDelegate::sizeHint (option, index);

		switch (index.data (Core::CLREntryType).value<Core::CLEntryType> ())
		{
		case Core::CLETContact:
			if (size.height () < ContactHeight_)
				size.setHeight (ContactHeight_);
			break;
		case Core::CLETAccount:
			size.setHeight (size.height () * 1.5);
			break;
		case Core::CLETCategory:
			const int textHeight = option.fontMetrics.height ();
			size.setHeight (qMax (textHeight + CPadding * 2, size.height ()));
			break;
		}

		return size;
	}

	void ContactListDelegate::DrawAccount (QPainter *painter,
			QStyleOptionViewItem o, const QModelIndex& index) const
	{
		QStyle *style = o.widget ?
				o.widget->style () :
				QApplication::style ();

		painter->save ();
		painter->setRenderHints (QPainter::HighQualityAntialiasing | QPainter::Antialiasing);

		style->drawPrimitive (QStyle::PE_PanelButtonCommand,
				&o, painter, o.widget);

		painter->restore ();

		o.font.setBold (true);

		QStyledItemDelegate::paint (painter, o, index);

		const auto acc = index.data (Core::CLRAccountObject).value<IAccount*> ();
		const auto extAcc = qobject_cast<IExtSelfInfoAccount*> (acc->GetQObject ());

		QIcon accIcon = extAcc ? extAcc->GetAccountIcon () : QIcon ();
		if (accIcon.isNull ())
			accIcon = qobject_cast<IProtocol*> (acc->GetParentProtocol ())->GetProtocolIcon ();

		const QRect& r = o.rect;
		const int iconSize = r.height () - 2 * CPadding;

		QImage avatarImg;
		if (extAcc && extAcc->GetSelfContact ())
		{
			const auto selfEntry = qobject_cast<ICLEntry*> (extAcc->GetSelfContact ());
			avatarImg = Core::Instance ().GetAvatar (selfEntry, iconSize);
		}

		if (avatarImg.isNull ())
			avatarImg = ResourcesManager::Instance ().GetDefaultAvatar (iconSize);
		else
			avatarImg = avatarImg.scaled (iconSize, iconSize,
					Qt::KeepAspectRatio, Qt::SmoothTransformation);

		QPoint pxDraw = o.rect.topRight () - QPoint (CPadding, 0);

		if (!avatarImg.isNull ())
		{
			pxDraw.rx () -= avatarImg.width ();
			const QPoint delta { 0, (r.height () - avatarImg.height ()) / 2 };
			painter->drawPixmap (pxDraw + delta,
					QPixmap::fromImage (avatarImg));
			pxDraw.rx () -= CPadding;
		}

		if (!accIcon.isNull ())
		{
			const int size = std::min (16, iconSize);
			const QPixmap& px = accIcon.pixmap (size, size);
			pxDraw.rx () -= px.width ();
			const QPoint delta { 0, (r.height () - px.height ()) / 2 };
			painter->drawPixmap (pxDraw + delta, px);
		}
	}

	namespace
	{
		QPair<int, int> GetCounts (const QModelIndex& index)
		{
			const int visibleCount = index.model ()->rowCount (index);

			auto model = index.model ();
			auto sourceIndex = index;
			while (auto proxyModel = qobject_cast<const QAbstractProxyModel*> (model))
			{
				model = proxyModel->sourceModel ();
				sourceIndex = proxyModel->mapToSource (sourceIndex);
			}

			if (model->rowCount (sourceIndex) != visibleCount)
				return { visibleCount, model->rowCount (sourceIndex) };

			const auto numOnline = index.data (Core::CLRNumOnline).toInt ();

			return { numOnline, visibleCount };
		}
	}

	void ContactListDelegate::DrawCategory (QPainter *painter,
			QStyleOptionViewItem o, const QModelIndex& index) const
	{
		const QRect& r = o.rect;

		auto style = o.widget ?
				o.widget->style () :
				QApplication::style ();

		style->drawPrimitive (HighlightGroups_ ? QStyle::PE_PanelButtonCommand : QStyle::PE_PanelItemViewRow,
				&o, painter, o.widget);

		const int unread = index.data (Core::CLRUnreadMsgCount).toInt ();
		if (unread)
		{
			painter->save ();

			const QString& text = QString (" %1 :: ").arg (unread);

			QFont unreadFont = o.font;
			unreadFont.setBold (true);

			int unreadSpace = CPadding + QFontMetrics (unreadFont).width (text);

			painter->setFont (unreadFont);
			painter->drawText (r.left () + CPadding, r.top () + CPadding,
					unreadSpace, r.height () - 2 * CPadding,
					Qt::AlignVCenter | Qt::AlignLeft,
					text);

			painter->restore ();

			o.rect.setLeft (unreadSpace + o.rect.left ());
		}

		QStyledItemDelegate::paint (painter, o, index);

		o.state &= ~(QStyle::State_Selected | QStyle::State_MouseOver);

		const int textWidth = o.fontMetrics.width (index.data ().value<QString> () + " ");
		const int rem = r.width () - textWidth;

		const auto& counts = GetCounts (index);

		const QString& str = QString (" %1/%2 ")
				.arg (counts.first)
				.arg (counts.second);

		painter->save ();

		painter->setRenderHints (QPainter::HighQualityAntialiasing | QPainter::Antialiasing);

		if (rem >= o.fontMetrics.width (str))
		{
			if (o.state & QStyle::State_Selected)
				painter->setPen (o.palette.color (QPalette::HighlightedText));

			QFont font = painter->font ();
			font.setItalic (true);
			painter->setFont (font);
			const QRect numRect (r.left () + textWidth - 1, r.top () + CPadding,
					rem - 1, r.height () - 2 * CPadding);
			painter->drawText (numRect, Qt::AlignVCenter | Qt::AlignRight, str);
		}

		painter->restore ();
	}

	void ContactListDelegate::DrawContact (QPainter *painter,
			QStyleOptionViewItem option, const QModelIndex& index) const
	{
		QObject *entryObj = index.data (Core::CLREntryObject).value<QObject*> ();
		ICLEntry *entry = qobject_cast<ICLEntry*> (entryObj);

		const bool isMUC = entry->GetEntryType () == ICLEntry::EntryType::MUC;

		QStyle *style = option.widget ?
				option.widget->style () :
				QApplication::style ();

		const QRect& r = option.rect;
		const int sHeight = r.height ();
		const int iconSize = sHeight - 2 * CPadding;
		const int clientIconSize = (iconSize > 16) ? 16 : iconSize;

		const QIcon& stateIcon = index.data (Qt::DecorationRole).value<QIcon> ();
		QString name = index.data (Qt::DisplayRole).value<QString> ();
		const QString status = entry->GetStatus ().StatusString_.replace ('\n', ' ');
		const QImage& avatarImg = ShowAvatars_ ?
				Core::Instance ().GetAvatar (entry, iconSize) :
				QImage ();
		const int unreadNum = index.data (Core::CLRUnreadMsgCount).toInt ();
		const QString& unreadStr = unreadNum ?
				QString (" %1 :: ").arg (unreadNum) :
				QString ();
		if (ShowStatuses_ && !status.isEmpty ())
			name += " (" + status + ")";

		const bool selected = option.state & QStyle::State_Selected;
		const QColor fgColor = selected ?
				option.palette.color (QPalette::HighlightedText) :
				option.palette.color (QPalette::Text);

		QFont unreadFont;
		int unreadSpace = 0;
		if (unreadNum)
		{
			unreadFont = option.font;
			unreadFont.setBold (true);

			unreadSpace = CPadding + QFontMetrics (unreadFont).width (unreadStr);
		}

		const int textShift = 2 * CPadding + iconSize + unreadSpace;

		const QStringList& vars = entry->Variants ();

		const auto& clientIcons = GetContactIcons (index, entry, vars);
		const int clientsIconsWidth = clientIcons.isEmpty () ?
				0 :
				clientIcons.size () * (clientIconSize + CPadding);
		/* text for width is total width minus shift of the text from
		 * the left (textShift) minus space for avatar (if present) with
		 * paddings minus space for client icons and paddings between
		 * them: there are N-1 paddings inbetween if there are N icons.
		 */
		const int textWidth = r.width () - textShift -
				(isMUC || !ShowAvatars_ ? 0 : (iconSize + 2 * CPadding)) -
				clientsIconsWidth;

		QPixmap pixmap (r.size ());
		pixmap.fill (option.palette.color (QPalette::Base));
		QPainter p (&pixmap);

		if (selected ||
				(option.state & QStyle::State_MouseOver))
		{
			auto bgOpt = option;
			bgOpt.rect.moveTopLeft (QPoint (0, 0));
			style->drawPrimitive (QStyle::PE_PanelItemViewItem,
					&bgOpt, &p, option.widget);
		}

		p.setPen (fgColor);

		if (unreadNum)
		{
			p.setFont (unreadFont);
			p.drawText (textShift - unreadSpace, CPadding,
					textWidth, r.height () - 2 * CPadding,
					Qt::AlignVCenter | Qt::AlignLeft,
					unreadStr);
		}

		p.setFont (option.font);
		p.drawText (textShift, CPadding,
				textWidth, r.height () - 2 * CPadding,
				Qt::AlignVCenter | Qt::AlignLeft,
				option.fontMetrics.elidedText (name, Qt::ElideRight, textWidth));

		const QPixmap& stateIconPx = stateIcon.pixmap (iconSize, iconSize);
		p.drawPixmap (QPoint (CPadding, (sHeight - stateIconPx.height ()) / 2),
				stateIconPx);

		if (!avatarImg.isNull ())
			p.drawPixmap (QPoint (textShift + textWidth + clientsIconsWidth + CPadding, CPadding),
					QPixmap::fromImage (avatarImg));

		int currentShift = textShift + textWidth + CPadding;

		for (const auto& icon : clientIcons)
		{
			const auto& px = icon.pixmap (clientIconSize, clientIconSize);
			p.drawPixmap (QPoint { currentShift, (sHeight - px.size ().height ()) / 2 }, px);
			currentShift += clientIconSize + CPadding;
		}

		painter->drawPixmap (option.rect, pixmap);
	}

	QList<QIcon> ContactListDelegate::GetContactIcons (const QModelIndex& index,
			ICLEntry *entry, const QStringList& vars) const
	{
		QList<QIcon> clientIcons;

		const bool isMUC = entry->GetEntryType () == ICLEntry::EntryType::MUC;

		if (!isMUC && ShowClientIcons_)
		{
			const auto& iconsMap = ResourcesManager::Instance ().GetClientIconForEntry (entry);
			for (int i = 0; i < std::min (vars.size (), 4); ++i)
				clientIcons << iconsMap [vars.at (i)];

			clientIcons.erase (std::remove_if (clientIcons.begin (), clientIcons.end (),
						[] (const QIcon& icon) { return icon.isNull (); }),
					clientIcons.end ());
		}

		if (entry->GetEntryType () == ICLEntry::EntryType::PrivateChat)
		{
			const QByteArray& aff = index.data (Core::CLRAffiliation).toByteArray ();
			const QIcon& icon = ResourcesManager::Instance ().GetAffIcon (aff);

			if (!icon.isNull ())
				clientIcons.prepend (icon);
		}

		if (vars.isEmpty ())
		{
			emit hookCollectContactIcons (IHookProxy_ptr { new Util::DefaultHookProxy },
					entry->GetQObject (), clientIcons);
			return clientIcons;
		}

		if (const auto ihca = qobject_cast<IHaveContactActivity*> (entry->GetQObject ()))
		{
			QString iconName;
			for (const auto& var : vars)
			{
				const auto& info = ihca->GetUserActivity (var);
				iconName = GetActivityIconName (info.General_, info.Specific_);
				if (!iconName.isEmpty ())
				{
					iconName.prepend (ActivityIconset_ + '/');
					break;
				}
			}

			auto icon = ActivityIconCache_ [iconName];
			if (icon.isNull ())
				icon = QIcon (ResourcesManager::Instance ()
						.GetResourceLoader (ResourcesManager::RLTActivityIconLoader)->
								GetIconPath (iconName));

			if (!icon.isNull ())
			{
				clientIcons.prepend (icon);
				ActivityIconCache_ [iconName] = icon;
			}
		}
		if (const auto ihcm = qobject_cast<IHaveContactMood*> (entry->GetQObject ()))
		{
			QString iconName;
			for (const auto& var : vars)
			{
				iconName = ihcm->GetUserMood (var).Mood_;
				if (!iconName.isEmpty ())
				{
					iconName [0] = iconName.at (0).toUpper ();
					iconName.prepend (MoodIconset_ + '/');
					break;
				}
			}

			auto icon = MoodIconCache_ [iconName];
			if (icon.isNull ())
				icon = QIcon (ResourcesManager::Instance ()
						.GetResourceLoader (ResourcesManager::RLTMoodIconLoader)->
								GetIconPath (iconName));

			if (!icon.isNull ())
			{
				clientIcons.prepend (icon);
				MoodIconCache_ [iconName] = icon;
			}
		}
		if (const auto ihct = qobject_cast<IHaveContactTune*> (entry->GetQObject ()))
		{
			if (std::any_of (vars.begin (), vars.end (),
					[ihct] (const QString& var)
					{
						const auto& info = ihct->GetUserTune (var);
						return !info.Artist_.isEmpty () || !info.Album_.isEmpty ();
					}))
				LoadSystemIcon ("/notification_roster_tune", clientIcons);
		}

		if (auto geoloc = qobject_cast<ISupportGeolocation*> (entry->GetParentAccount ()->GetQObject ()))
		{
			const auto& info = geoloc->GetUserGeolocationInfo (entry->GetQObject (), vars.value (0));
			if (!info.isEmpty ())
				LoadSystemIcon ("/geolocation", clientIcons);
		}

		emit hookCollectContactIcons (IHookProxy_ptr { new Util::DefaultHookProxy },
				entry->GetQObject (), clientIcons);

		return clientIcons;
	}

	void ContactListDelegate::LoadSystemIcon (const QString& name,
			QList<QIcon>& clientIcons) const
	{
		const QString& iconName = SystemIconset_ + name;
		QIcon icon = SystemIconCache_ [iconName];
		if (icon.isNull ())
			icon = QIcon (ResourcesManager::Instance ()
					.GetResourceLoader (ResourcesManager::RLTSystemIconLoader)->
							GetIconPath (iconName));

		if (!icon.isNull ())
		{
			clientIcons.prepend (icon);
			SystemIconCache_ [iconName] = icon;
		}
	}

	void ContactListDelegate::handleShowAvatarsChanged ()
	{
		ShowAvatars_ = XmlSettingsManager::Instance ()
				.property ("ShowAvatars").toBool ();
	}

	void ContactListDelegate::handleShowClientIconsChanged ()
	{
		ShowClientIcons_ = XmlSettingsManager::Instance ()
				.property ("ShowClientIcons").toBool ();
	}

	void ContactListDelegate::handleActivityIconsetChanged ()
	{
		ActivityIconCache_.clear ();

		ActivityIconset_ = XmlSettingsManager::Instance ()
				.property ("ActivityIcons").toString ();
	}

	void ContactListDelegate::handleMoodIconsetChanged ()
	{
		MoodIconCache_.clear ();

		MoodIconset_ = XmlSettingsManager::Instance ()
				.property ("MoodIcons").toString ();
	}

	void ContactListDelegate::handleSystemIconsetChanged ()
	{
		SystemIconCache_.clear ();

		SystemIconset_ = XmlSettingsManager::Instance ()
				.property ("SystemIcons").toString ();
	}

	void ContactListDelegate::handleShowStatusesChanged ()
	{
		ShowStatuses_ = XmlSettingsManager::Instance ()
				.property ("ShowStatuses").toBool ();

		View_->viewport ()->update ();
		View_->update ();
	}

	void ContactListDelegate::handleHighlightGroupsChanged ()
	{
		HighlightGroups_ = XmlSettingsManager::Instance ()
				.property ("HighlightGroups").toBool ();

		View_->viewport ()->update ();
		View_->update ();
	}

	void ContactListDelegate::handleContactHeightChanged ()
	{
		ContactHeight_ = XmlSettingsManager::Instance ()
				.property ("RosterContactHeight").toInt ();
		if (ContactHeight_ <= 0)
			ContactHeight_ = 24;

		View_->setIconSize (QSize (ContactHeight_, ContactHeight_));

		View_->viewport ()->update ();
		View_->update ();
	}
}
}
