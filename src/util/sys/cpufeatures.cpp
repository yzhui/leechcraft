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

#include "cpufeatures.h"
#include <mutex>
#include <cassert>
#include <QStringList>
#include <QtDebug>

#if defined (Q_PROCESSOR_X86) || QT_VERSION < 0x050000
#define HAS_CPUID
#endif

#ifdef HAS_CPUID
#include <cpuid.h>
#endif

namespace LeechCraft
{
namespace Util
{
	CpuFeatures::CpuFeatures ()
	{
#ifdef HAS_CPUID
		uint32_t eax = 0, ebx = 0, ecx = 0, edx = 0;
		if (!__get_cpuid (1, &eax, &ebx, &ecx, &edx))
			qWarning () << Q_FUNC_INFO
					<< "failed to get CPUID eax = 1";
		else
			Ecx1_ = ecx;

		if (__get_cpuid_max (0, nullptr) < 7)
			qWarning () << Q_FUNC_INFO
					<< "cpuid max less than 7";
		else
		{
			__cpuid_count (7, 0, eax, ebx, ecx, edx);
			Ebx7_ = ebx;
		}
#endif

		static std::once_flag dbgFlag;
		std::call_once (dbgFlag,
				[this] { DumpDetectedFeatures (); });
	}

	QString CpuFeatures::GetFeatureName (Feature feature)
	{
		switch (feature)
		{
		case Feature::SSSE3:
			return "ssse3";
		case Feature::SSE41:
			return "sse4.1";
		case Feature::AVX:
			return "avx";
		case Feature::XSave:
			return "xsave";
		case Feature::AVX2:
			return "avx2";
		case Feature::None:
			return "";
		}

		assert (0);
	}

	bool CpuFeatures::HasFeature (Feature feature) const
	{
		switch (feature)
		{
		case Feature::SSSE3:
			return Ecx1_ & (1 << 9);
		case Feature::SSE41:
			return Ecx1_ & (1 << 19);
		case Feature::AVX:
			return Ecx1_ & (1 << 28);
		case Feature::XSave:
			return Ecx1_ & (1 << 26);
		case Feature::AVX2:
			return HasFeature (Feature::XSave) && (Ebx7_ & (1 << 5));
		case Feature::None:
			return true;
		}

		assert (0);
	}

	void CpuFeatures::DumpDetectedFeatures () const
	{
		if (qgetenv ("DUMP_CPUFEATURES").isEmpty ())
			return;

		QStringList detected;
		QStringList undetected;

		for (int i = 0; i < static_cast<int> (Feature::None); ++i)
		{
			const auto feature = static_cast<Feature> (i);
			const auto& featureName = GetFeatureName (feature);
			if (HasFeature (feature))
				detected << featureName;
			else
				undetected << featureName;
		}

		qDebug () << Q_FUNC_INFO;
		qDebug () << "detected the following CPU features:" << detected.join (" ").toUtf8 ().constData ();
		qDebug () << "couldn't detect the following CPU features:" << undetected.join (" ").toUtf8 ().constData ();
	}
}
}