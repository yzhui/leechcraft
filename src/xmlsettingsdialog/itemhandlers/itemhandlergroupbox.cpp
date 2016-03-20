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

#include "itemhandlergroupbox.h"
#include <QLabel>
#include <QGroupBox>
#include <QGridLayout>
#include <QtDebug>

namespace LeechCraft
{
	bool ItemHandlerGroupbox::CanHandle (const QDomElement& element) const
	{
		return element.attribute ("type") == "groupbox" &&
				element.attribute ("checkable") == "true";
	}

	void ItemHandlerGroupbox::Handle (const QDomElement& item, QWidget *pwidget)
	{
		QGroupBox *box = new QGroupBox (XSD_->GetLabel (item));
		box->setObjectName (item.attribute ("property"));
		QGridLayout *groupLayout = new QGridLayout ();
		groupLayout->setContentsMargins (2, 2, 2, 2);
		box->setLayout (groupLayout);
		box->setCheckable (true);

		const QVariant& value = XSD_->GetValue (item);

		box->setChecked (value.toBool ());
		connect (box,
				SIGNAL (toggled (bool)),
				this,
				SLOT (updatePreferences ()));
		box->setProperty ("ItemHandler", QVariant::fromValue<QObject*> (this));
		box->setProperty ("SearchTerms", box->title ());

		XSD_->ParseEntity (item, box);

		QGridLayout *lay = qobject_cast<QGridLayout*> (pwidget->layout ());
		lay->addWidget (box, lay->rowCount (), 0, 1, 2);
	}

	void ItemHandlerGroupbox::SetValue (QWidget *widget,
			const QVariant& value) const
	{
		QGroupBox *groupbox = qobject_cast<QGroupBox*> (widget);
		if (!groupbox)
		{
			qWarning () << Q_FUNC_INFO
				<< "not a QGroupBox"
				<< widget;
			return;
		}
		groupbox->setChecked (value.toBool ());
	}

	QVariant ItemHandlerGroupbox::GetObjectValue (QObject *object) const
	{
		QGroupBox *groupbox = qobject_cast<QGroupBox*> (object);
		if (!groupbox)
		{
			qWarning () << Q_FUNC_INFO
				<< "not a QGroupBox"
				<< object;
			return QVariant ();
		}
		return groupbox->isChecked ();
	}
}
