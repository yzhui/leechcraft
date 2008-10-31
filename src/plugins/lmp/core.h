#ifndef CORE_H
#define CORE_H
#include <memory>
#include <QObject>
#include <MediaObject>
#include <AudioOutput>

class Core : public QObject
{
	Q_OBJECT

	std::auto_ptr<Phonon::MediaObject> MediaObject_;
	std::auto_ptr<Phonon::AudioOutput> AudioOutput_;

	Core ();
public:
	static Core& Instance ();
	void Release ();

	void Reinitialize (const QString&);
	Phonon::MediaObject* GetMediaObject () const;
	Phonon::AudioOutput* GetAudioOutput () const;
};

#endif

