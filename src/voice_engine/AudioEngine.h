#ifndef AUDIOENGINE_H
#define AUDIOENGINE_H

#include <QObject>
#include <QAudioFormat>
#include <QAudioSink>
#include <QAudioSource>
#include <QIODevice>
#include <QByteArray>
#include <QVector>
#include <QList>
#include <QMutex>

#include <opus.h>

class AudioEngine : public QObject {
    Q_OBJECT

public:
    explicit AudioEngine(QObject *parent = nullptr);
    ~AudioEngine();

    void startAudio();
    void stopAudio();
    void decodeOpusToSpeaker(const unsigned char *opusBytes, int opusLen);

    void setPttActive(bool active);
    void preparePttTransmission();

signals:
    void audioOutputLevel(float level);
    void opusFrameReady(const QByteArray &opusBytes);

private slots:
    void onInputReadyRead();

private:
    QAudioFormat m_outputFormat;
    QAudioFormat m_inputFormat;

    QAudioSink *m_audioSink = nullptr;
    QIODevice *m_outputDevice = nullptr;

    QAudioSource *m_audioSource = nullptr;
    QIODevice *m_inputDevice = nullptr;

    OpusDecoder *m_opusDecoder = nullptr;
    OpusEncoder *m_opusEncoder = nullptr;

    QByteArray m_micPcmBuffer;
    float m_agcGain = 60.0f; // Automatic Gain Control baseline

    QMutex m_mutex;
    bool m_isStarted = false;
    bool m_isPttActive = false;

    void processRawInputPcm(const QByteArray &rawBytes);
    void encodeAndDispatchFrames();
};

#endif // AUDIOENGINE_H
