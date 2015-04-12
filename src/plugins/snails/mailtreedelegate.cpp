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

#include "mailtreedelegate.h"
#include <QPainter>
#include <QToolBar>
#include <QtDebug>
#include <util/sll/slotclosure.h>
#include "mailtab.h"
#include "mailmodel.h"
#include "messagelistactioninfo.h"

namespace LeechCraft
{
namespace Snails
{
	MailTreeDelegate::MailTreeDelegate (const MessageLoader_f& loader, QObject *parent)
	: QStyledItemDelegate { parent }
	, Loader_ { loader }
	{
	}

	void MailTreeDelegate::paint (QPainter *painter,
			const QStyleOptionViewItem& stockItem, const QModelIndex& index) const
	{
		const bool isRead = index.data (MailModel::MailRole::IsRead).toBool ();
		const bool isEnabled = index.flags () & Qt::ItemIsEnabled;

		QStyleOptionViewItemV4 item { stockItem };
		if (!isRead && isEnabled)
			item.font.setBold (true);
		QStyledItemDelegate::paint (painter, item, index);
	}

	QWidget* MailTreeDelegate::createEditor (QWidget *parent,
			const QStyleOptionViewItem&, const QModelIndex& index) const
	{
		const auto& actionsVar = index.data (MailModel::MailRole::MessageActions);
		if (actionsVar.isNull ())
			return nullptr;

		const auto& actionInfos = actionsVar.value<QList<MessageListActionInfo>> ();
		if (actionInfos.isEmpty ())
			return nullptr;

		const auto& id = index.data (MailModel::MailRole::ID).toByteArray ();

		Message_ptr msg;
		try
		{
			msg = Loader_ (id);
		}
		catch (const std::exception& e)
		{
			qWarning () << Q_FUNC_INFO
					<< "unable to load message"
					<< id.toHex ()
					<< e.what ();
			return nullptr;
		}

		const auto container = new QToolBar { parent };
		for (const auto actInfo : actionInfos)
		{
			const auto action = container->addAction (actInfo.Icon_, actInfo.Name_);
			action->setToolTip (actInfo.Description_);

			new Util::SlotClosure<Util::NoDeletePolicy>
			{
				[handler = actInfo.Handler_, msg] { handler (msg); },
				action,
				SIGNAL (triggered ()),
				action
			};
		}

		return container;
	}

	void MailTreeDelegate::updateEditorGeometry (QWidget *editor,
			const QStyleOptionViewItem& option, const QModelIndex&) const
	{
		qobject_cast<QToolBar*> (editor)->setIconSize (option.decorationSize * 0.75);

		editor->setMaximumSize (option.rect.size ());
		editor->move (option.rect.topRight () - QPoint { editor->width (), 0 });
	}

	bool MailTreeDelegate::eventFilter (QObject *object, QEvent *event)
	{
		blockSignals (true);
		const auto res = QStyledItemDelegate::eventFilter (object, event);
		blockSignals (false);
		return res;
	}
}
}
