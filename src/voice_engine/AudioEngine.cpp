#include "AudioEngine.h"
#include <QMediaDevices>
#include <QAudioDevice>
#include <QtMath>
#include <cmath>
#include <QDebug>

AudioEngine::AudioEngine(QObject *parent) : QObject(parent) {
    // Standard Windows Audio Output: 48000 Hz, 2 Channels (Stereo), Int16
    m_outputFormat.setSampleRate(48000);
    m_outputFormat.setChannelCount(2);
    m_outputFormat.setSampleFormat(QAudioFormat::Int16);

    // Standard Windows Microphone Input: 48000 Hz, Int16 (or preferred format detected on start)
    m_inputFormat.setSampleRate(48000);
    m_inputFormat.setChannelCount(1);
    m_inputFormat.setSampleFormat(QAudioFormat::Int16);

    int err = 0;
    m_opusDecoder = opus_decoder_create(48000, 1, &err);
    if (err != OPUS_OK || !m_opusDecoder) {
        qWarning() << "[OPUS ERROR] Failed to create OpusDecoder:" << err;
    }

    m_opusEncoder = opus_encoder_create(48000, 1, OPUS_APPLICATION_VOIP, &err);
    if (err == OPUS_OK && m_opusEncoder) {
        opus_encoder_ctl(m_opusEncoder, OPUS_SET_APPLICATION(OPUS_APPLICATION_VOIP));
        opus_encoder_ctl(m_opusEncoder, OPUS_SET_BITRATE(64000));
        opus_encoder_ctl(m_opusEncoder, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE));
        opus_encoder_ctl(m_opusEncoder, OPUS_SET_COMPLEXITY(10));
        opus_encoder_ctl(m_opusEncoder, OPUS_SET_VBR(1));
        opus_encoder_ctl(m_opusEncoder, OPUS_SET_INBAND_FEC(1));
        opus_encoder_ctl(m_opusEncoder, OPUS_SET_DTX(0));
    }
}

AudioEngine::~AudioEngine() {
    stopAudio();
    if (m_opusDecoder) {
        opus_decoder_destroy(m_opusDecoder);
        m_opusDecoder = nullptr;
    }
    if (m_opusEncoder) {
        opus_encoder_destroy(m_opusEncoder);
        m_opusEncoder = nullptr;
    }
}

void AudioEngine::startAudio() {
    if (m_isStarted) return;

    QAudioDevice defaultOutput = QMediaDevices::defaultAudioOutput();
    if (!defaultOutput.isFormatSupported(m_outputFormat)) {
        m_outputFormat = defaultOutput.preferredFormat();
        if (m_outputFormat.sampleRate() <= 0) m_outputFormat.setSampleRate(48000);
        if (m_outputFormat.channelCount() <= 0) m_outputFormat.setChannelCount(2);
        m_outputFormat.setSampleFormat(QAudioFormat::Int16);
    }

    m_audioSink = new QAudioSink(defaultOutput, m_outputFormat, this);
    m_audioSink->setBufferSize(48000 * 2 * sizeof(int16_t)); // 1 second stereo buffer
    m_audioSink->setVolume(1.0f);
    m_outputDevice = m_audioSink->start(); // Direct Push mode

    QAudioDevice defaultInput = QMediaDevices::defaultAudioInput();
    m_inputFormat = defaultInput.preferredFormat();
    if (m_inputFormat.sampleRate() <= 0) m_inputFormat.setSampleRate(48000);
    if (m_inputFormat.channelCount() <= 0) m_inputFormat.setChannelCount(1);

    m_audioSource = new QAudioSource(defaultInput, m_inputFormat, this);
    m_audioSource->setBufferSize(48000 * sizeof(int16_t) * 4);
    m_audioSource->setVolume(1.0f);
    m_inputDevice = m_audioSource->start();

    if (m_inputDevice) {
        connect(m_inputDevice, &QIODevice::readyRead, this, &AudioEngine::onInputReadyRead);
    }

    m_isStarted = true;
    qDebug() << "[AUDIO ENGINE STARTED] Output:" << m_outputFormat.sampleRate() << "Hz" 
             << m_outputFormat.channelCount() << "ch format:" << m_outputFormat.sampleFormat()
             << "| Input:" << m_inputFormat.sampleRate() << "Hz" << m_inputFormat.channelCount() 
             << "ch format:" << m_inputFormat.sampleFormat();
}

void AudioEngine::stopAudio() {
    if (!m_isStarted) return;
    if (m_audioSink) {
        m_audioSink->stop();
        delete m_audioSink;
        m_audioSink = nullptr;
        m_outputDevice = nullptr;
    }
    if (m_audioSource) {
        m_audioSource->stop();
        delete m_audioSource;
        m_audioSource = nullptr;
        m_inputDevice = nullptr;
    }
    m_micPcmBuffer.clear();
    m_isStarted = false;
}

void AudioEngine::setPttActive(bool active) {
    QMutexLocker locker(&m_mutex);
    m_isPttActive = active;
    if (active) {
        m_micPcmBuffer.clear();
        if (m_inputDevice && m_inputDevice->isOpen()) {
            m_inputDevice->readAll(); // Flush old buffer
        }
        if (m_opusEncoder) {
            opus_encoder_ctl(m_opusEncoder, OPUS_RESET_STATE);
        }
    } else {
        m_micPcmBuffer.clear();
    }
}

void AudioEngine::preparePttTransmission() {
    setPttActive(true);
}

void AudioEngine::decodeOpusToSpeaker(const unsigned char *opusBytes, int opusLen) {
    if (!m_opusDecoder || opusLen <= 0) return;
    if (!m_isStarted) {
        startAudio();
    }

    const int maxSamples = 5760; // Max 120ms @ 48kHz mono
    int16_t pcmMono[maxSamples];

    int decodedSamples = opus_decode(m_opusDecoder, opusBytes, opusLen, pcmMono, maxSamples, 0);
    if (decodedSamples > 0 && m_outputDevice && m_outputDevice->isOpen()) {
        int outChannels = m_outputFormat.channelCount();
        if (outChannels >= 2) {
            // Duplicate mono to Stereo Left & Right for native Windows WASAPI playback
            int16_t pcmStereo[maxSamples * 2];
            for (int i = 0; i < decodedSamples; ++i) {
                pcmStereo[2 * i]     = pcmMono[i];
                pcmStereo[2 * i + 1] = pcmMono[i];
            }
            m_outputDevice->write(reinterpret_cast<const char *>(pcmStereo), decodedSamples * 2 * sizeof(int16_t));
        } else {
            m_outputDevice->write(reinterpret_cast<const char *>(pcmMono), decodedSamples * sizeof(int16_t));
        }
    }
}

void AudioEngine::onInputReadyRead() {
    QMutexLocker locker(&m_mutex);
    if (m_inputDevice && m_inputDevice->isOpen()) {
        QByteArray chunk = m_inputDevice->readAll();
        if (!chunk.isEmpty()) {
            processRawInputPcm(chunk);
            if (m_isPttActive) {
                encodeAndDispatchFrames();
            } else {
                // If PTT not active, drop buffer to avoid latency buildup
                m_micPcmBuffer.clear();
            }
        }
    }
}

void AudioEngine::processRawInputPcm(const QByteArray &rawBytes) {
    if (rawBytes.isEmpty()) return;

    int channels = qMax(1, m_inputFormat.channelCount());
    int inSampleRate = qMax(8000, m_inputFormat.sampleRate());
    QAudioFormat::SampleFormat sampleFormat = m_inputFormat.sampleFormat();
    
    // Step 1: Decode to Normalized Float Mono [-1.0 .. 1.0]
    QVector<float> inputMonoFloat;

    if (sampleFormat == QAudioFormat::Float) {
        const float *src = reinterpret_cast<const float *>(rawBytes.constData());
        int numFrames = (rawBytes.size() / sizeof(float)) / channels;
        inputMonoFloat.reserve(numFrames);

        for (int i = 0; i < numFrames; ++i) {
            float sum = 0.0f;
            for (int c = 0; c < channels; ++c) {
                sum += src[i * channels + c];
            }
            inputMonoFloat.append(sum / channels);
        }
    } else if (sampleFormat == QAudioFormat::Int32) {
        const int32_t *src = reinterpret_cast<const int32_t *>(rawBytes.constData());
        int numFrames = (rawBytes.size() / sizeof(int32_t)) / channels;
        inputMonoFloat.reserve(numFrames);

        for (int i = 0; i < numFrames; ++i) {
            int64_t sum = 0;
            for (int c = 0; c < channels; ++c) {
                sum += src[i * channels + c];
            }
            double val = (double)(sum / channels) / 2147483648.0;
            inputMonoFloat.append(static_cast<float>(val));
        }
    } else {
        // Int16 default
        const int16_t *src = reinterpret_cast<const int16_t *>(rawBytes.constData());
        int numFrames = (rawBytes.size() / sizeof(int16_t)) / channels;
        inputMonoFloat.reserve(numFrames);

        for (int i = 0; i < numFrames; ++i) {
            int32_t sum = 0;
            for (int c = 0; c < channels; ++c) {
                sum += src[i * channels + c];
            }
            float val = (float)(sum / channels) / 32768.0f;
            inputMonoFloat.append(val);
        }
    }

    if (inputMonoFloat.isEmpty()) return;

    // Step 2: Resample from inSampleRate to 48000 Hz if needed
    QVector<float> resampled48k;
    if (inSampleRate == 48000) {
        resampled48k = inputMonoFloat;
    } else {
        double ratio = (double)inSampleRate / 48000.0;
        int outSamples = static_cast<int>(inputMonoFloat.size() / ratio);
        resampled48k.reserve(outSamples);

        for (int i = 0; i < outSamples; ++i) {
            double srcIdx = i * ratio;
            int idx0 = static_cast<int>(srcIdx);
            int idx1 = qMin(idx0 + 1, inputMonoFloat.size() - 1);
            double frac = srcIdx - idx0;

            float s0 = inputMonoFloat[idx0];
            float s1 = inputMonoFloat[idx1];
            float interpolated = static_cast<float>((1.0 - frac) * s0 + frac * s1);
            resampled48k.append(interpolated);
        }
    }

    // Step 3: Clean Studio Vocal Chain (Noise Gate + Stable 6.0x Boost + Soft-Knee Limiter)
    // Completely eliminates hunting, oscillating, and periodic background noise pumping!
    const float staticGain = 6.0f;
    const float noiseGateThreshold = 0.0012f; // Cuts out low room tone & mic hiss

    QVector<int16_t> finalPcm;
    finalPcm.reserve(resampled48k.size());

    for (float f : resampled48k) {
        if (std::abs(f) < noiseGateThreshold) {
            f = 0.0f; // Silence noise floor
        }
        float amplified = f * staticGain;
        float saturated = std::tanh(amplified);
        int32_t sampleInt = static_cast<int32_t>(saturated * 32767.0f);
        if (sampleInt > 32767) sampleInt = 32767;
        else if (sampleInt < -32768) sampleInt = -32768;
        finalPcm.append(static_cast<int16_t>(sampleInt));
    }

    m_micPcmBuffer.append(reinterpret_cast<const char *>(finalPcm.constData()), finalPcm.size() * sizeof(int16_t));
}

void AudioEngine::encodeAndDispatchFrames() {
    if (!m_opusEncoder) return;

    // 960 samples = 20ms @ 48kHz mono = 1920 bytes
    const int frameSamples = 960;
    const int frameBytes = frameSamples * sizeof(int16_t);

    while (m_micPcmBuffer.size() >= frameBytes) {
        QByteArray frameData = m_micPcmBuffer.left(frameBytes);
        m_micPcmBuffer.remove(0, frameBytes);

        unsigned char opusOut[4000];
        int encodedBytes = opus_encode(m_opusEncoder,
                                      reinterpret_cast<const int16_t *>(frameData.constData()),
                                      frameSamples,
                                      opusOut,
                                      sizeof(opusOut));

        if (encodedBytes > 0) {
            emit opusFrameReady(QByteArray(reinterpret_cast<const char *>(opusOut), encodedBytes));
        }
    }
}
