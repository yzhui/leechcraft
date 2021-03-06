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

#include <QStandardItemModel>
#include <QHash>
#include <util/models/dndactionsmixin.h>
#include "interfaces/lmp/icollectionmodel.h"
#include "interfaces/lmp/collectiontypes.h"

namespace LeechCraft
{
namespace LMP
{
	class LocalCollectionStorage;

	class LocalCollectionModel : public Util::DndActionsMixin<QStandardItemModel>
							   , public ICollectionModel
	{
		Q_OBJECT

		LocalCollectionStorage * const Storage_;

		QIcon ArtistIcon_ = QIcon::fromTheme ("view-media-artist");

		QHash<int, QStandardItem*> Artist2Item_;
		QHash<int, QHash<int, QStandardItem*>> Album2Item_;
		QHash<int, QStandardItem*> Track2Item_;
	public:
		enum NodeType
		{
			Artist,
			Album,
			Track
		};

		enum Role
		{
			Node = Qt::UserRole + 1,
			ArtistName,
			AlbumYear,
			AlbumName,
			AlbumArt,
			TrackID,
			TrackNumber,
			TrackTitle,
			TrackPath,
			TrackGenres,
			TrackLength,
			IsTrackIgnored
		};

		LocalCollectionModel (LocalCollectionStorage*, QObject*);

		QStringList mimeTypes () const override;
		QMimeData* mimeData (const QModelIndexList&) const override;
		QVariant data (const QModelIndex& index, int role) const override;

		QList<QUrl> ToSourceUrls (const QList<QModelIndex>&) const override;

		void AddArtists (const Collection::Artists_t&);
		void Clear ();

		void IgnoreTrack (int);

		void RemoveTrack (int);
		void RemoveAlbum (int);
		void RemoveArtist (int);

		void SetAlbumArt (int, const QString&);
		QVariant GetTrackData (int trackId, Role) const;

		void UpdatePlayStats (int);
	};
}
}
