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

#include "callmanager.h"

#ifdef ENABLE_MEDIACALLS
#include <QAudioDeviceInfo>
#include <QAudioInput>
#include <QAudioOutput>
#endif

#include <QtDebug>
#include <util/xpc/util.h>
#include <util/xpc/notificationactionhandler.h>
#include "interfaces/azoth/iclentry.h"
#include "interfaces/azoth/iaccount.h"
#include "xmlsettingsmanager.h"
#include "core.h"

namespace LeechCraft
{
namespace Azoth
{
	CallManager::CallManager (QObject *parent)
	: QObject (parent)
	{
	}

	void CallManager::AddAccount (QObject *account)
	{
		if (!qobject_cast<ISupportMediaCalls*> (account))
			return;

		connect (account,
				SIGNAL (called (QObject*)),
				this,
				SLOT (handleCall (QObject*)));
	}

	QObject* CallManager::Call (ICLEntry *entry, const QString& variant)
	{
#ifdef ENABLE_MEDIACALLS
		const auto ismc = qobject_cast<ISupportMediaCalls*> (entry->GetParentAccount ()->GetQObject ());
		if (!ismc)
		{
			qWarning () << Q_FUNC_INFO
					<< entry->GetQObject ()
					<< "parent account doesn't support media calls";
			return nullptr;
		}

		const auto callObj = ismc->Call (entry->GetEntryID (), variant);
		if (!callObj)
		{
			qWarning () << Q_FUNC_INFO
					<< "got null call obj for"
					<< entry->GetEntryID ()
					<< variant;
			return nullptr;
		}

		return callObj;
#else
		return nullptr;
#endif
	}

	QObjectList CallManager::GetCallsForEntry (const QString& id) const
	{
		return Entry2Calls_ [id];
	}

#ifdef ENABLE_MEDIACALLS
	namespace
	{
		QAudioDeviceInfo FindDevice (const QByteArray& property, QAudio::Mode mode)
		{
			const QString& name = XmlSettingsManager::Instance ()
					.property (property).toString ();

			QAudioDeviceInfo result = mode == QAudio::AudioInput ?
					QAudioDeviceInfo::defaultInputDevice () :
					QAudioDeviceInfo::defaultOutputDevice ();
			Q_FOREACH (const QAudioDeviceInfo& info,
					QAudioDeviceInfo::availableDevices (mode))
				if (info.deviceName () == name)
				{
					result = info;
					break;
				}

			return result;
		}
	}
#endif

	void CallManager::HandleIncomingCall (IMediaCall *mediaCall)
	{
		const auto entry = qobject_cast<ICLEntry*> (Core::Instance ().GetEntry (mediaCall->GetSourceID ()));
		const auto& name = entry ?
				entry->GetEntryName () :
				mediaCall->GetSourceID ();

		auto e = Util::MakeNotification ("Azoth",
				tr ("Incoming call from %1").arg (name),
				PInfo_);
		const auto nh = new Util::NotificationActionHandler (e, this);
		nh->AddFunction (tr ("Accept"), [mediaCall] () { mediaCall->Accept (); });
		nh->AddFunction (tr ("Hangup"), [mediaCall] () { mediaCall->Hangup (); });
		Core::Instance ().SendEntity (e);
	}

	void CallManager::handleCall (QObject *obj)
	{
		const auto mediaCall = qobject_cast<IMediaCall*> (obj);
		if (!mediaCall)
		{
			qWarning () << Q_FUNC_INFO
					<< obj
					<< "is not a IMediaCall, got from"
					<< sender ();
			return;
		}

		const auto& sourceId = mediaCall->GetSourceID ();
		if (Entry2Calls_.value (sourceId).contains (obj))
		{
			qWarning () << Q_FUNC_INFO
					<< "attempt to double-add the call"
					<< obj
					<< "from"
					<< sourceId
					<< sender ();
			return;
		}

		Entry2Calls_ [sourceId] << obj;

		connect (obj,
				SIGNAL (stateChanged (LeechCraft::Azoth::IMediaCall::State)),
				this,
				SLOT (handleStateChanged (LeechCraft::Azoth::IMediaCall::State)));
		connect (obj,
				SIGNAL (audioModeChanged (QIODevice::OpenMode)),
				this,
				SLOT (handleAudioModeChanged (QIODevice::OpenMode)));
		connect (obj,
				SIGNAL (readFormatChanged ()),
				this,
				SLOT (handleReadFormatChanged ()));
		connect (obj,
				SIGNAL (writeFormatChanged ()),
				this,
				SLOT (handleWriteFormatChanged ()));

		if (mediaCall->GetDirection () == IMediaCall::DIn)
			HandleIncomingCall (mediaCall);
	}

	void CallManager::handleStateChanged (IMediaCall::State state)
	{
		qDebug () << Q_FUNC_INFO << state << (state == IMediaCall::SActive);

		if (state != IMediaCall::SFinished)
			return;

		CallStates_.remove (sender ());

		if (const auto call = qobject_cast<IMediaCall*> (sender ()))
			Entry2Calls_ [call->GetSourceID ()].removeAll (sender ());
		else
			qWarning () << Q_FUNC_INFO
					<< "sender isn't an IMediaCall";
	}

	namespace
	{
#ifdef ENABLE_MEDIACALLS
		void WarnUnsupported (const QAudioDeviceInfo& info, const QAudioFormat& format, const QByteArray& dbgstr)
		{
			qWarning () << dbgstr
					<< "\n\tbyte orders:"
					<< info.supportedByteOrders ()
					<< "\n\tchannel counts:"
					<< info.supportedChannelCounts ()
					<< "\n\tcodecs:"
					<< info.supportedCodecs ()
					<< "\n\tfrequencies:"
#if QT_VERSION < 0x050000
					<< info.supportedFrequencies ()
#else
					<< info.supportedSampleRates ()
#endif
					<< "\n\tsample types:"
					<< info.supportedSampleTypes ();
			qWarning () << "instead we got:"
					<< "\n\tbyte order:"
					<< format.byteOrder ()
					<< "\n\tchannel count:"
					<< format.channelCount ()
					<< "\n\tcodec:"
					<< format.codec ()
					<< "\n\tfrequency:"
					<< format.sampleRate ()
					<< "\n\tsample type:"
					<< format.sampleType ();
		}

		int GetBufSize (const QAudioFormat& format)
		{
#if QT_VERSION < 0x050000
			const auto frequency = format.frequency ();
			const auto channels = format.channels ();
#else
			const auto frequency = format.sampleRate ();
			const auto channels = format.channelCount ();
#endif
			return (frequency * channels * (format.sampleSize () / 8) * 160) / 1000;
		}
#endif
	}

	void CallManager::handleAudioModeChanged (QIODevice::OpenMode mode)
	{
		qDebug () << Q_FUNC_INFO;
#ifdef ENABLE_MEDIACALLS
		const auto mediaCall = qobject_cast<IMediaCall*> (sender ());
		const auto callAudioDev = mediaCall->GetAudioDevice ();

		auto& callState = CallStates_ [sender ()];
		callState.LastMode_ = mode;

		if ((mode & QIODevice::WriteOnly) && !callState.OutDevice_)
		{
			const auto& format = mediaCall->GetAudioWriteFormat ();

			const auto& outInfo = FindDevice ("OutputAudioDevice", QAudio::AudioOutput);

			qDebug () << "opening output:" << outInfo.deviceName ();

			if (!outInfo.isFormatSupported (format))
				WarnUnsupported (outInfo, format,
						"raw audio format not supported by backend, cannot play audio");

			const auto output = std::make_shared<QAudioOutput> (outInfo, format);
			connect (output.get (),
					SIGNAL (stateChanged (QAudio::State)),
					this,
					SLOT (handleDevStateChanged (QAudio::State)));
			output->setBufferSize (GetBufSize (format));
			output->start (callAudioDev);

			callState.OutDevice_ = output;
		}

		if ((mode & QIODevice::ReadOnly) && !callState.InDevice_)
		{
			const auto& format = mediaCall->GetAudioReadFormat ();

			const auto& inInfo = FindDevice ("InputAudioDevice", QAudio::AudioInput);

			qDebug () << "opening input:" << inInfo.deviceName ();

			if (!inInfo.isFormatSupported (format))
				WarnUnsupported (inInfo, format,
						"raw audio format not supported by backend, cannot record audio");

			const auto input = std::make_shared<QAudioInput> (inInfo, format);
			connect (input.get (),
					SIGNAL (stateChanged (QAudio::State)),
					this,
					SLOT (handleDevStateChanged (QAudio::State)));
			input->setBufferSize (GetBufSize (format));
			input->start (callAudioDev);

			callState.InDevice_ = input;
		}
#endif
	}

	void CallManager::handleReadFormatChanged ()
	{
		qDebug () << Q_FUNC_INFO;
	}

	void CallManager::handleWriteFormatChanged ()
	{
		qDebug () << Q_FUNC_INFO;
	}

#ifdef ENABLE_MEDIACALLS
	void CallManager::handleDevStateChanged (QAudio::State state)
	{
		auto input = qobject_cast<QAudioInput*> (sender ());
		auto output = qobject_cast<QAudioOutput*> (sender ());
		if (state == QAudio::StoppedState)
		{
			 if (input && input->error () != QAudio::NoError)
				 qWarning () << Q_FUNC_INFO << input->error ();
			 if (output && output->error () != QAudio::NoError)
				 qWarning () << Q_FUNC_INFO << output->error ();
		}
	}
#endif
}
}
