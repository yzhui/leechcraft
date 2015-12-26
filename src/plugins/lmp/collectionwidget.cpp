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

#include "collectionwidget.h"
#include <QSortFilterProxyModel>
#include <QMessageBox>
#include <QMenu>
#include <util/gui/clearlineeditaddon.h>
#include <util/util.h>
#include <util/xpc/defaulthookproxy.h>
#include <interfaces/core/iiconthememanager.h>
#include "core.h"
#include "localcollection.h"
#include "localcollectionmodel.h"
#include "palettefixerfilter.h"
#include "collectiondelegate.h"
#include "audiopropswidget.h"
#include "util.h"
#include "albumartmanagerdialog.h"
#include "collectionsmanager.h"
#include "hookinterconnector.h"

namespace LeechCraft
{
namespace LMP
{
	namespace
	{
		class CollectionFilterModel : public QSortFilterProxyModel
		{
		public:
			CollectionFilterModel (QObject *parent = 0)
			: QSortFilterProxyModel (parent)
			{
				setDynamicSortFilter (true);
			}
		protected:
			bool filterAcceptsRow (int sourceRow, const QModelIndex& sourceParent) const
			{
				const auto& source = sourceModel ()->index (sourceRow, 0, sourceParent);
				const auto type = source.data (LocalCollectionModel::Role::Node).toInt ();

				const auto& pattern = filterRegExp ().pattern ();

				if (type != LocalCollectionModel::NodeType::Track)
					for (int i = 0, rc = sourceModel ()->rowCount (source); i < rc; ++i)
						if (filterAcceptsRow (i, source))
							return true;

				auto check = [&source, &pattern] (int role)
				{
					return source.data (role).toString ().contains (pattern, Qt::CaseInsensitive);
				};
				return check (Qt::DisplayRole) ||
						check (LocalCollectionModel::Role::ArtistName) ||
						check (LocalCollectionModel::Role::AlbumName) ||
						check (LocalCollectionModel::Role::TrackTitle) ||
						check (LocalCollectionModel::Role::AlbumYear);
			}
		};
	}

	CollectionWidget::CollectionWidget (QWidget *parent)
	: QWidget { parent }
	, Player_ { Core::Instance ().GetPlayer () }
	, CollectionFilterModel_ { new CollectionFilterModel { this } }
	{
		Ui_.setupUi (this);

		new Util::ClearLineEditAddon (Core::Instance ().GetProxy (), Ui_.CollectionFilter_);
		new PaletteFixerFilter (Ui_.CollectionTree_);

		connect (Core::Instance ().GetLocalCollection (),
				SIGNAL (scanStarted (int)),
				Ui_.ScanProgress_,
				SLOT (setMaximum (int)));
		connect (Core::Instance ().GetLocalCollection (),
				SIGNAL (scanProgressChanged (int)),
				this,
				SLOT (handleScanProgress (int)));
		connect (Core::Instance ().GetLocalCollection (),
				SIGNAL (scanFinished ()),
				Ui_.ScanProgress_,
				SLOT (hide ()));
		Ui_.ScanProgress_->hide ();

		Ui_.CollectionTree_->setItemDelegate (new CollectionDelegate (Ui_.CollectionTree_));
		auto collMgr = Core::Instance ().GetCollectionsManager ();
		CollectionFilterModel_->setSourceModel (collMgr->GetModel ());
		Ui_.CollectionTree_->setModel (CollectionFilterModel_);

		connect (Ui_.CollectionTree_,
				SIGNAL (doubleClicked (QModelIndex)),
				this,
				SLOT (loadFromCollection ()));

		connect (Ui_.CollectionFilter_,
				SIGNAL (textChanged (QString)),
				CollectionFilterModel_,
				SLOT (setFilterFixedString (QString)));

		Core::Instance ().GetHookInterconnector ()->RegisterHookable (this);
	}

	void CollectionWidget::showCollectionTrackProps ()
	{
		const auto& index = Ui_.CollectionTree_->currentIndex ();
		const auto& info = index.data (LocalCollectionModel::Role::TrackPath).toString ();
		if (info.isEmpty ())
			return;

		AudioPropsWidget::MakeDialog ()->SetProps (info);
	}

	void CollectionWidget::showCollectionAlbumArt ()
	{
		const auto& index = Ui_.CollectionTree_->currentIndex ();
		const auto& path = index.data (LocalCollectionModel::Role::AlbumArt).toString ();
		if (path.isEmpty ())
			return;

		ShowAlbumArt (path, QCursor::pos ());
	}

	void CollectionWidget::showAlbumArtManager ()
	{
		auto aamgr = Core::Instance ().GetLocalCollection ()->GetAlbumArtManager ();

		const auto& index = Ui_.CollectionTree_->currentIndex ();
		const auto& album = index.data (LocalCollectionModel::Role::AlbumName).toString ();
		const auto& artist = index.data (LocalCollectionModel::Role::ArtistName).toString ();

		auto dia = new AlbumArtManagerDialog (artist, album, aamgr, this);
		dia->setAttribute (Qt::WA_DeleteOnClose);
		dia->show ();
	}

	void CollectionWidget::showInArtistBrowser ()
	{
		const auto& index = Ui_.CollectionTree_->currentIndex ();
		const auto& artist = index.data (LocalCollectionModel::Role::ArtistName).toString ();
		Core::Instance ().RequestArtistBrowser (artist);
	}

	namespace
	{
		template<typename T>
		QList<T> CollectFromModel (const QModelIndex& root, int role)
		{
			QList<T> result;

			const auto& var = root.data (role);
			if (!var.isNull ())
				result << var.value<T> ();

			auto model = root.model ();
			for (int i = 0; i < model->rowCount (root); ++i)
				result += CollectFromModel<T> (root.child (i, 0), role);

			return result;
		}
	}

	void CollectionWidget::handleCollectionRemove ()
	{
		const auto& index = Ui_.CollectionTree_->currentIndex ();
		const auto& paths = CollectFromModel<QString> (index, LocalCollectionModel::Role::TrackPath);
		if (paths.isEmpty ())
			return;

		auto response = QMessageBox::question (this,
				"LeechCraft",
				tr ("Are you sure you want to remove %n track(s) from your collection?<br/><br/>"
					"Please note that if tracks remain on your disk they will be re-added next "
					"time collection is scanned, but you will lose the statistics.",
					0,
					paths.size ()),
					QMessageBox::Yes | QMessageBox::No);
		if (response != QMessageBox::Yes)
			return;

		auto collection = Core::Instance ().GetLocalCollection ();
		Q_FOREACH (const auto& path, paths)
			collection->RemoveTrack (path);
	}

	void CollectionWidget::handleCollectionDelete ()
	{
		const auto& index = Ui_.CollectionTree_->currentIndex ();
		const auto& paths = CollectFromModel<QString> (index, LocalCollectionModel::Role::TrackPath);
		if (paths.isEmpty ())
			return;

		auto response = QMessageBox::question (this,
				"LeechCraft",
				tr ("Are you sure you want to erase %n track(s)? This action cannot be undone.",
					0,
					paths.size ()),
					QMessageBox::Yes | QMessageBox::No);
		if (response != QMessageBox::Yes)
			return;

		for (const auto& path : paths)
			QFile::remove (path);
	}

	void CollectionWidget::loadFromCollection ()
	{
		const auto& idxs = Ui_.CollectionTree_->selectionModel ()->selectedRows ();

		QModelIndexList mapped;
		for (const auto& src : idxs)
		{
			const auto& index = CollectionFilterModel_->mapToSource (src);
			if (index.isValid ())
				mapped << index;
		}

		Core::Instance ().GetCollectionsManager ()->Enqueue (mapped, Player_);
	}

	namespace
	{
		MediaInfo ColIndex2MediaInfo (const QModelIndex& index)
		{
			return
			{
				index.data (LocalCollectionModel::Role::TrackPath).toString (),
				index.data (LocalCollectionModel::Role::ArtistName).toString (),
				index.data (LocalCollectionModel::Role::AlbumName).toString (),
				index.data (LocalCollectionModel::Role::TrackTitle).toString (),
				index.data (LocalCollectionModel::Role::TrackGenres).toStringList (),
				index.data (LocalCollectionModel::Role::TrackLength).toInt (),
				index.data (LocalCollectionModel::Role::AlbumYear).toInt (),
				index.data (LocalCollectionModel::Role::TrackNumber).toInt ()
			};
		}
	}

	void CollectionWidget::on_CollectionTree__customContextMenuRequested (const QPoint& point)
	{
		const auto& index = Ui_.CollectionTree_->indexAt (point);
		if (!index.isValid ())
			return;

		const int nodeType = index.data (LocalCollectionModel::Role::Node).value<int> ();

		QMenu menu;

		auto addToPlaylist = menu.addAction (tr ("Add to playlist"),
				this, SLOT (loadFromCollection ()));
		addToPlaylist->setProperty ("ActionIcon", "list-add");

		if (nodeType == LocalCollectionModel::NodeType::Track)
		{
			auto showTrackProps = menu.addAction (tr ("Show track properties"),
					this, SLOT (showCollectionTrackProps ()));
			showTrackProps->setProperty ("ActionIcon", "document-properties");
		}

		if (nodeType == LocalCollectionModel::NodeType::Album)
		{
			auto showAlbumArt = menu.addAction (tr ("Show album art"),
					this, SLOT (showCollectionAlbumArt ()));
			showAlbumArt->setProperty ("ActionIcon", "media-optical");

			menu.addAction (tr ("Album art manager..."), this, SLOT (showAlbumArtManager ()));
		}

		auto showInArtistBrowser = menu.addAction (tr ("Show in artist browser"),
				this, SLOT (showInArtistBrowser ()));
		menu.addSeparator ();

		auto remove = menu.addAction (tr ("Remove from collection..."), this, SLOT (handleCollectionRemove ()));
		remove->setProperty ("ActionIcon", "list-remove");

		auto del = menu.addAction (tr ("Delete from disk..."), this, SLOT (handleCollectionDelete ()));
		del->setProperty ("ActionIcon", "edit-delete");

		emit hookCollectionContextMenuRequested (std::make_shared<Util::DefaultHookProxy> (),
				&menu, ColIndex2MediaInfo (index));

		Core::Instance ().GetProxy ()->GetIconThemeManager ()->ManageWidget (&menu);

		menu.exec (Ui_.CollectionTree_->viewport ()->mapToGlobal (point));
	}

	void CollectionWidget::handleScanProgress (int progress)
	{
		if (progress >= Ui_.ScanProgress_->maximum ())
		{
			Ui_.ScanProgress_->hide ();
			return;
		}

		if (!Ui_.ScanProgress_->isVisible ())
			Ui_.ScanProgress_->show ();
		Ui_.ScanProgress_->setValue (progress);
	}
}
}
