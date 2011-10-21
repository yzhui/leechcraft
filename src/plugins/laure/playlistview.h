/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2011 Minh Ngo
 * Copyright (C) 2006-2011  Georg Rudoy
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **********************************************************************/

#ifndef PLUGINS_LAURE_PLAYLISTVIEW_H
#define PLUGINS_LAURE_PLAYLISTVIEW_H

#include <QListView>
#include "playlistmodel.h"
#include "core.h"

class QKeyEvent;

namespace LeechCraft
{
namespace Laure
{
	class PlayListModel;
	
	class PlayListView : public QListView
	{
		Q_OBJECT
		
		PlayListModel *PlayListModel_;
	public:
		PlayListView (QWidget* = 0);
		
		void AddItem (const MediaMeta&);
	protected:
		void keyPressEvent (QKeyEvent*);
	public slots:
		void selectRow (int);
		void removeSelectedRows ();
		void handleItemAdded (); 
	private slots:
		void handleDoubleClicked (const QModelIndex&);
	signals:
		void itemRemoved (int);
		void playItem (int);
	};
}
}
#endif // PLUGINS_LAURE_PLAYLISTVIEW_H
