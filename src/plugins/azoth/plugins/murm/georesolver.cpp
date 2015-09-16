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

#include "georesolver.h"
#include <QFuture>
#include <QFutureInterface>
#include <util/sll/qtutil.h>
#include "vkconnection.h"

namespace LeechCraft
{
namespace Azoth
{
namespace Murm
{
	GeoResolver::GeoResolver (VkConnection *conn, QObject *parent)
	: QObject (parent)
	, Conn_ (conn)
	{
	}

	void GeoResolver::CacheCountries (QList<int> ids)
	{
		Cache (ids, Countries_, PendingCountries_, GeoIdType::Country);
	}

	void GeoResolver::AddCountriesToCache (const QHash<int, QString>& countries)
	{
		Countries_.unite (countries);
	}

	QFuture<QString> GeoResolver::RequestCountry (int code)
	{
		QFutureInterface<QString> iface;
		iface.reportStarted ();
		Get (code, {}, Countries_, GeoIdType::Country);
		return iface.future ();
	}

	QString GeoResolver::GetCountry (int code) const
	{
		return Countries_.value (code);
	}

	void GeoResolver::CacheCities (QList<int> ids)
	{
		Cache (ids, Cities_, PendingCities_, GeoIdType::Country);
	}

	void GeoResolver::AddCitiesToCache (const QHash<int, QString>& cities)
	{
		Cities_.unite (cities);
	}

	QFuture<QString> GeoResolver::RequestCity (int code)
	{
		QFutureInterface<QString> iface;
		iface.reportStarted ();
		Get (code, {}, Cities_, GeoIdType::City);
		return iface.future ();
	}

	QString GeoResolver::GetCity (int code) const
	{
		return Cities_.value (code);
	}

	void GeoResolver::Cache (QList<int> ids, QHash<int, QString>& result, QSet<int>& pending, GeoIdType type)
	{
		auto newEnd = std::remove_if (ids.begin (), ids.end (),
				[&result, &pending] (int id)
				{
					return result.contains (id) || pending.contains (id);
				});
		ids.erase (newEnd, ids.end ());

		if (ids.isEmpty ())
			return;

		for (const auto id : ids)
			pending << id;

		Conn_->RequestGeoIds (ids,
				[&result, &pending] (const QHash<int, QString>& newItems)
				{
					result.unite (newItems);
					for (const auto& pair : Util::Stlize (newItems))
						pending.remove (pair.first);
				},
				type);
	}

	void GeoResolver::Get (int code, std::function<void (QString)> setter,
			QHash<int, QString>& hash, GeoIdType type)
	{
		if (hash.contains (code))
		{
			setter (hash [code]);
			return;
		}

		Conn_->RequestGeoIds ({ code },
				[&hash, setter, code] (const QHash<int, QString>& result)
				{
					hash.unite (result);
					setter (result [code]);
				},
				type);
	}
}
}
}
