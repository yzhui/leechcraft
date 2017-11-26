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

#include <util/sll/either.h>
#include "audiostructs.h"

template<typename>
class QFuture;

namespace Media
{
	/** @brief Information about a track release.
	 *
	 * A release can be, for example, an album, a compilation or a
	 * single.
	 *
	 * @sa IDiscographyProvider
	 */
	struct ReleaseTrackInfo
	{
		/** @brief The number of the track in the release.
		 */
		int Number_;

		/** @brief The name of the track.
		 */
		QString Name_;

		/** @brief The length of the track in this release.
		 */
		int Length_;
	};

	/** @brief Information about a release, like an album or a single.
	 *
	 * @sa IDiscographyProvider
	 */
	struct ReleaseInfo
	{
		/** @brief The internal ID of this release.
		 */
		QString ID_;

		/** @brief The name of this release.
		 */
		QString Name_;

		/** @brief The year of this release.
		 */
		int Year_;

		/** @brief The enum describing the recognized types of the releases.
		 */
		enum class Type
		{
			/** @brief A typical album.
			 */
			Standard,

			/** @brief An EP.
			 */
			EP,

			/** @brief A single track release.
			 */
			Single,

			/** @brief A compilation.
			 */
			Compilation,

			/** @brief A live release.
			 */
			Live,

			/** @brief A soundtrack.
			 */
			Soundtrack,

			/** @brief Some other release type currently unrecognized by
			 * LeechCraft.
			 */
			Other
		} Type_;

		/** List of tracks in this release.
		 */
		QList<QList<ReleaseTrackInfo>> TrackInfos_;
	};

	/** @brief Interface for plugins supporting getting artist discography.
	 *
	 * Plugins that support fetching artists discography from sources
	 * like MusicBrainz should implement this interface.
	 *
	 * Discography includes various types of releases (albums, EPs,
	 * singles, etc) as well as the corresponding lists of tracks in
	 * those releases.
	 */
	class Q_DECL_EXPORT IDiscographyProvider
	{
	public:
		virtual ~IDiscographyProvider () {}

		using QueryResult_t = LeechCraft::Util::Either<QString, QList<ReleaseInfo>>;

		/** @brief Returns the service name.
		 *
		 * This string returns a human-readable string with the service
		 * name, like "MusicBrainz".
		 *
		 * @return The human-readable service name.
		 */
		virtual QString GetServiceName () const = 0;

		/** @brief Fetches all the discography of the given artist.
		 *
		 * This function initiates a search for artist discography and
		 * returns a handle through which the results of the search could
		 * be obtained.
		 *
		 * All known releases of this artist are returned through the
		 * handle.
		 *
		 * The handle owns itself and deletes itself after results are
		 * available — see its documentation for details.
		 *
		 * @param[in] artist The artist name.
		 * @return The pending discography search handle.
		 */
		virtual QFuture<QueryResult_t> GetDiscography (const QString& artist) = 0;

		/** @brief Fetches contents of the given release by the artist.
		 *
		 * This function initiates a search for the given release of the
		 * given artist and returns a handle through which the contents
		 * of the release can be obtained.
		 *
		 * Only matching release is returned through the handle, or no
		 * releases if, well, no releases match.
		 *
		 * The handle owns itself and deletes itself after results are
		 * available — see its documentation for details.
		 *
		 * @param[in] artist The artist name.
		 * @param[in] release The release name to search for.
		 * @return The pending discography search handle.
		 */
		virtual QFuture<QueryResult_t> GetReleaseInfo (const QString& artist, const QString& release) = 0;
	};
}

Q_DECLARE_INTERFACE (Media::IDiscographyProvider, "org.LeechCraft.Media.IDiscographyProvider/1.0")
