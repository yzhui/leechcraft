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

#include "desaturateeffect.h"
#include <limits>
#include <cmath>
#include <QPainter>

namespace LeechCraft
{
namespace SB2
{
	DesaturateEffect::DesaturateEffect (QObject *parent)
	: QGraphicsEffect (parent)
	, Strength_ (0)
	{
	}

	qreal DesaturateEffect::GetStrength () const
	{
		return Strength_;
	}

	void DesaturateEffect::SetStrength (qreal strength)
	{
		if (std::fabs (static_cast<float> (strength) - Strength_) < std::numeric_limits<qreal>::epsilon ())
			return;

		Strength_ = static_cast<float> (strength);
		emit strengthChanged ();

		update ();
	}

	void DesaturateEffect::draw (QPainter *painter)
	{
		if (std::fabs (Strength_) < 0.001)
		{
			drawSource (painter);
			return;
		}

		QPoint offset;
		const auto& px = sourcePixmap (Qt::LogicalCoordinates, &offset);

		auto img = px.toImage ();
		switch (img.format ())
		{
		case QImage::Format_ARGB32:
		case QImage::Format_ARGB32_Premultiplied:
			break;
		default:
			img = img.convertToFormat (QImage::Format_ARGB32);
			break;
		}
		img.detach ();

		const auto height = img.height ();
		const auto width = img.width ();
		for (int y = 0; y < height; ++y)
		{
			const auto scanline = reinterpret_cast<QRgb*> (img.scanLine (y));
			for (int x = 0; x < width; ++x)
			{
				auto& color = scanline [x];
				const auto grayPart = qGray (color) * Strength_;
				const auto r = qRed (color) * (1 - Strength_) + grayPart;
				const auto g = qGreen (color) * (1 - Strength_) + grayPart;
				const auto b = qBlue (color) * (1 - Strength_) + grayPart;
				color = qRgba (r, g, b, qAlpha (color));
			}
		}

		painter->drawImage (offset, img);
	}
}
}
