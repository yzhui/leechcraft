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

#include <memory>
#include <QDialog>
#include "tagsconfig.h"

class QStringList;
class QString;

namespace Ui
{
	class CategorySelector;
}

namespace LeechCraft
{
	namespace Util
	{
		/** @brief The CategorySelector widget provides a way to select amongst
		 * a group of items.
		 *
		 * The CategorySelector is a QWidget having Qt::Tool window hint. That
		 * results in representing this widget as a tool window - usually a small
		 * window with smaller than usual title bar and decoration.
		 * CategorySelector represents the possible selections as a list of
		 * check boxes.
		 *
		 * Programmer can set the list of possible choice variants using
		 * SetPossibleSelections and get selected items with GetSelections.
		 *
		 * CategorySelector emits selectionChanged() signal when user changes
		 * his selection. CategorySelector's primary purpose is to help user to
		 * select tags using a line edit, so there's a convenience slot
		 * lineTextChanged() which could be used to notify CategorySelector
		 * about changes of possible categories. There are also convenience
		 * slots selectAll() and selectNone() which could be used to mark all
		 * and no elements in the list respectively.
		 */
		class UTIL_TAGS_API CategorySelector : public QDialog
		{
			Q_OBJECT

			std::shared_ptr<Ui::CategorySelector> Ui_;

			QString Caption_;
			QString Separator_;
		public:
			/** @brief Constructor.
			 *
			 * Sets the default window title and window flags
			 * (Qt::Tool | Qt::WindowStaysOnTopHint), calculates the
			 * default geometry.
			 *
			 * @param[in] parent Pointer to parent widget.
			 */
			CategorySelector (QWidget *parent = 0);

			/** @brief Sets the caption of this selector.
			 *
			 * By default, the selector has no caption.
			 *
			 * @param[in] caption The new caption of this selector.
			 */
			void SetCaption (const QString& caption);

			/** @brief Gets selected items.
			 *
			 * Returns the selected items - a subset of selection variants
			 * passed via SetPossibleSelections.
			 *
			 * @return Selected items.
			 *
			 * @sa SetPossibleSelections
			 */
			QStringList GetSelections ();

			/** @brief Selects some of the items.
			 *
			 * Selects some of the items presented by elements of the
			 * subset list.
			 *
			 * This function won't emit selectionChanged() signal.
			 *
			 * @param[in] subset The list of items to select.
			 */
			void SetSelections (const QStringList& subset);

			/** @brief Returns the separator for the tags.
			 *
			 * The default separator is "; ".
			 *
			 * @sa SetSeparator()
			 */
			QString GetSeparator () const;

			/** @brief Sets the separator for the tags.
			 *
			 * This function doesn't update the text in the line edit.
			 *
			 * @sa GetSeparator()
			 */
			void SetSeparator (const QString&);
		protected:
			/** @brief Checks whether after the move event the selector
			 * won't be beoynd the screen. if it would, moves back.
			 */
			virtual void moveEvent (QMoveEvent*);
		public slots:
			/** @brief Selects all variants.
			 */
			void selectAll ();
			/** @brief Deselects all variants.
			 */
			void selectNone ();
			/** @brief Sets possible selections.
			 *
			 * Clears previous selections list, sets new possible selections
			 * according to selections parameter. By default, no items are
			 * selected.
			 *
			 * @param[in] selections Possible selections.
			 *
			 * @sa GetSelections
			 */
			void setPossibleSelections (QStringList selections);
			/** @brief Notifies CategorySelector about logical selection
			 * changes.
			 *
			 * This slot is usually used to notify CategorySelector about
			 * selection changes done via a related widget - for example, a line
			 * edit with tags.
			 *
			 * @param[in] newText The text of the line edit.
			 */
			void lineTextChanged (const QString& newText);
		private slots:
			/** @brief Emits selectionChanged() to notify about selection changes.
			 */
			void buttonToggled ();
		signals:
			/** @brief Indicates that selections have changed.
			 *
			 * @param[out] newSelections Selected items.
			 */
			void tagsSelectionChanged (const QStringList& newSelections);
		};
	}
}
