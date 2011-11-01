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

#include "playlistmodel.h"

namespace LeechCraft
{
namespace Laure
{
	PlayListModel::PlayListModel (QObject* parent)
	: QStandardItemModel (parent)
	{
		setColumnCount (6);
		setHeaderData (1, Qt::Horizontal,  tr ("Artist"));
		setHeaderData (2, Qt::Horizontal, tr ("Title"));
		setHeaderData (3, Qt::Horizontal, tr ("Album"));
		setHeaderData (4, Qt::Horizontal, tr ("Genre"));
		setHeaderData (5, Qt::Horizontal, tr ("Date"));
	}
	
	Qt::ItemFlags PlayListModel::flags (const QModelIndex& index) const
	{
		return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDropEnabled;
	}
}
}

