#include "ffmpeg_player.h"

#include <QDebug>
#include <QFileInfo>

namespace {
double speedFromAtomic(const QAtomicInt &speed)
{
    return speed.loadRelaxed() / 1000.0;
}
}

FFmpegPlayer::FFmpegPlayer(QObject *parent)
    : QThread(parent)
    , m_videoQueue(3)
    , m_audioQueue(10)
{
}

FFmpegPlayer::~FFmpegPlayer()
{
    close();
}

bool FFmpegPlayer::open(const QString &filePath)
{
    close();

    m_filePath = filePath;
    m_fileName = QFileInfo(filePath).fileName();

    int ret = avformat_open_input(&m_fmtCtx, filePath.toUtf8().constData(), nullptr, nullptr);
    if (ret < 0) {
        emit errorOccurred(tr("Failed to open file: %1").arg(m_fileName));
        return false;
    }

    ret = avformat_find_stream_info(m_fmtCtx, nullptr);
    if (ret < 0) {
        emit errorOccurred(tr("Failed to find stream information."));
        close();
        return false;
    }

    if (m_fmtCtx->duration != AV_NOPTS_VALUE) {
        m_durationMs = m_fmtCtx->duration / 1000;
        emit durationChanged(m_durationMs);
    }

    if (!openStreams()) {
        close();
        return false;
    }

    if (m_videoStreamIndex >= 0) {
        m_swsCtx = sws_getContext(m_videoWidth, m_videoHeight, m_videoCodecCtx->pix_fmt,
                                  m_videoWidth, m_videoHeight, AV_PIX_FMT_RGB24,
                                  SWS_BILINEAR, nullptr, nullptr, nullptr);
        const int bufferSize = av_image_get_buffer_size(AV_PIX_FMT_RGB24, m_videoWidth, m_videoHeight, 1);
        m_rgbBuffer = static_cast<uint8_t *>(av_malloc(bufferSize));
        av_image_fill_arrays(m_rgbTmpData, m_rgbTmpLinesize, m_rgbBuffer,
                             AV_PIX_FMT_RGB24, m_videoWidth, m_videoHeight, 1);
        emit videoInfoChanged(m_videoWidth, m_videoHeight);
    }

    if (m_audioStreamIndex >= 0) {
        emit audioFormatChanged(m_audioSampleRate, m_audioChannels);
    }

    return true;
}

bool FFmpegPlayer::openStreams()
{
    for (unsigned int i = 0; i < m_fmtCtx->nb_streams; ++i) {
        AVStream *stream = m_fmtCtx->streams[i];
        AVCodecParameters *codecPar = stream->codecpar;
        const AVCodec *codec = avcodec_find_decoder(codecPar->codec_id);
        if (!codec) {
            continue;
        }

        if (codecPar->codec_type == AVMEDIA_TYPE_VIDEO && m_videoStreamIndex < 0) {
            m_videoCodecCtx = avcodec_alloc_context3(codec);
            avcodec_parameters_to_context(m_videoCodecCtx, codecPar);
            if (avcodec_open2(m_videoCodecCtx, codec, nullptr) < 0) {
                avcodec_free_context(&m_videoCodecCtx);
                continue;
            }
            m_videoStreamIndex = int(i);
            m_videoWidth = codecPar->width;
            m_videoHeight = codecPar->height;
        } else if (codecPar->codec_type == AVMEDIA_TYPE_AUDIO && m_audioStreamIndex < 0) {
            m_audioCodecCtx = avcodec_alloc_context3(codec);
            avcodec_parameters_to_context(m_audioCodecCtx, codecPar);
            if (avcodec_open2(m_audioCodecCtx, codec, nullptr) < 0) {
                avcodec_free_context(&m_audioCodecCtx);
                continue;
            }
            m_audioStreamIndex = int(i);
            m_audioSampleRate = m_audioCodecCtx->sample_rate;
            m_audioChannels = m_audioCodecCtx->ch_layout.nb_channels;
            m_audioSampleFmt = m_audioCodecCtx->sample_fmt;
        }
    }

    if (m_videoStreamIndex < 0 && m_audioStreamIndex < 0) {
        emit errorOccurred(tr("No playable video or audio stream was found."));
        return false;
    }

    return true;
}

void FFmpegPlayer::closeStreams()
{
    if (m_videoCodecCtx) {
        avcodec_free_context(&m_videoCodecCtx);
    }
    if (m_audioCodecCtx) {
        avcodec_free_context(&m_audioCodecCtx);
    }
    if (m_swsCtx) {
        sws_freeContext(m_swsCtx);
        m_swsCtx = nullptr;
    }
    if (m_swrCtx) {
        swr_free(&m_swrCtx);
    }
    if (m_rgbBuffer) {
        av_free(m_rgbBuffer);
        m_rgbBuffer = nullptr;
    }

    m_videoStreamIndex = -1;
    m_audioStreamIndex = -1;
}

void FFmpegPlayer::close()
{
    stop();
    if (isRunning()) {
        wait(3000);
    }

    closeStreams();

    if (m_fmtCtx) {
        avformat_close_input(&m_fmtCtx);
        m_fmtCtx = nullptr;
    }

    m_videoQueue.clear();
    m_audioQueue.clear();
    m_durationMs = 0;
    m_positionMs.storeRelaxed(0);
    m_videoWidth = 0;
    m_videoHeight = 0;
    m_audioClock = 0.0;
    m_clockInited.storeRelaxed(0);
    m_wallClockBaseUs = 0;
    m_ptsBaseSec = 0.0;
}

void FFmpegPlayer::play()
{
    if (m_state.loadRelaxed() == int(PlayState::Stopped) && m_fmtCtx) {
        m_stopRequested.storeRelaxed(0);
        m_clockInited.storeRelaxed(0);
        start();
    }
    m_state.storeRelaxed(int(PlayState::Playing));
    emit stateChanged(PlayState::Playing);
}

void FFmpegPlayer::pause()
{
    if (m_state.loadRelaxed() == int(PlayState::Playing)) {
        m_state.storeRelaxed(int(PlayState::Paused));
        emit stateChanged(PlayState::Paused);
    }
}

void FFmpegPlayer::stop()
{
    m_stopRequested.storeRelaxed(1);
    m_state.storeRelaxed(int(PlayState::Stopped));
    m_videoQueue.clear();
    m_audioQueue.clear();
}

void FFmpegPlayer::seek(qint64 positionMs)
{
    QMutexLocker locker(&m_seekMutex);
    m_seekTargetMs.storeRelaxed(positionMs);
    m_seekRequested.storeRelaxed(1);
}

void FFmpegPlayer::setVolume(int volume)
{
    m_volume.storeRelaxed(qBound(0, volume, 100));
}

void FFmpegPlayer::setAudioOutputFormat(int sampleRate, int channels, QAudioFormat::SampleFormat sampleFormat)
{
    if (!m_audioCodecCtx) {
        return;
    }
    m_sinkSampleRate = sampleRate;
    m_sinkChannels = channels;
    m_sinkSampleFormat = sampleFormat;
    rebuildSwr();
}

void FFmpegPlayer::rebuildSwr()
{
    if (!m_audioCodecCtx) {
        return;
    }

    if (m_swrCtx) {
        swr_free(&m_swrCtx);
    }

    AVChannelLayout outLayout;
    av_channel_layout_default(&outLayout, m_sinkChannels);

    AVSampleFormat outFmt = AV_SAMPLE_FMT_S16;
    switch (m_sinkSampleFormat) {
    case QAudioFormat::UInt8:
        outFmt = AV_SAMPLE_FMT_U8;
        break;
    case QAudioFormat::Int16:
        outFmt = AV_SAMPLE_FMT_S16;
        break;
    case QAudioFormat::Int32:
        outFmt = AV_SAMPLE_FMT_S32;
        break;
    case QAudioFormat::Float:
        outFmt = AV_SAMPLE_FMT_FLT;
        break;
    default:
        break;
    }

    const double currentSpeed = speedFromAtomic(m_speed);
    const int outSampleRate = qBound(4000, int(m_sinkSampleRate / currentSpeed), 192000);

    m_swrCtx = swr_alloc();
    const int ret = swr_alloc_set_opts2(&m_swrCtx,
                                        &outLayout, outFmt, outSampleRate,
                                        &m_audioCodecCtx->ch_layout, m_audioCodecCtx->sample_fmt,
                                        m_audioCodecCtx->sample_rate, 0, nullptr);
    av_channel_layout_uninit(&outLayout);

    if (ret < 0 || swr_init(m_swrCtx) < 0) {
        swr_free(&m_swrCtx);
        emit errorOccurred(tr("Failed to initialize audio resampler."));
        return;
    }

    m_audioSampleRate = m_sinkSampleRate;
    m_audioChannels = m_sinkChannels;
    m_audioSampleFmt = outFmt;
}

void FFmpegPlayer::setSpeed(double speed)
{
    speed = qBound(0.25, speed, 4.0);
    m_speed.storeRelaxed(int(speed * 1000));
    rebuildSwr();
    m_clockInited.storeRelaxed(0);
    emit speedChanged(speed);
}

void FFmpegPlayer::run()
{
    decodeLoop();
}

double FFmpegPlayer::framePtsSec(const AVFrame *frame, const AVStream *stream) const
{
    int64_t timestamp = frame->pts;
    if (timestamp == AV_NOPTS_VALUE) {
        timestamp = frame->best_effort_timestamp;
    }
    if (timestamp == AV_NOPTS_VALUE) {
        return -1.0;
    }
    return timestamp * av_q2d(stream->time_base);
}

void FFmpegPlayer::resetClock(double ptsBaseSec)
{
    m_wallClockBaseUs = av_gettime_relative();
    m_ptsBaseSec = ptsBaseSec;
    m_clockInited.storeRelaxed(1);
}

void FFmpegPlayer::syncToClock(double ptsSec, qint64 leadUs)
{
    if (m_state.loadRelaxed() != int(PlayState::Playing)) {
        return;
    }
    if (!m_clockInited.loadRelaxed()) {
        resetClock(ptsSec);
        return;
    }

    const double currentSpeed = speedFromAtomic(m_speed);
    const qint64 nowUs = av_gettime_relative();
    const double relSec = ptsSec - m_ptsBaseSec;
    const qint64 targetUs = m_wallClockBaseUs + qint64(relSec / currentSpeed * 1000000.0);
    const qint64 aheadUs = targetUs - nowUs;
    if (aheadUs <= leadUs) {
        return;
    }

    qint64 sleepUs = aheadUs - leadUs;
    if (sleepUs > 1000000) {
        sleepUs = 1000000;
    }
    if (sleepUs > 0) {
        usleep(ulong(sleepUs));
    }
}

void FFmpegPlayer::decodeLoop()
{
    AVPacket *packet = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();
    bool wasPaused = false;
    int64_t pauseStartUs = 0;

    while (!m_stopRequested.loadRelaxed()) {
        if (m_state.loadRelaxed() == int(PlayState::Paused)) {
            if (!wasPaused) {
                pauseStartUs = av_gettime_relative();
                wasPaused = true;
            }
            msleep(50);
            continue;
        }
        if (wasPaused) {
            if (m_clockInited.loadRelaxed()) {
                m_wallClockBaseUs += av_gettime_relative() - pauseStartUs;
            }
            wasPaused = false;
        }

        if (m_seekRequested.loadRelaxed()) {
            qint64 targetMs = 0;
            {
                QMutexLocker locker(&m_seekMutex);
                targetMs = m_seekTargetMs.loadRelaxed();
                m_seekRequested.storeRelaxed(0);
            }

            const int streamIndex = m_videoStreamIndex >= 0 ? m_videoStreamIndex : m_audioStreamIndex;
            const AVRational timeBase = m_fmtCtx->streams[streamIndex]->time_base;
            const AVRational microSecBase = {1, 1000000};
            const int64_t seekTarget = av_rescale_q(targetMs * 1000, microSecBase, timeBase);

            av_seek_frame(m_fmtCtx, streamIndex, seekTarget, AVSEEK_FLAG_BACKWARD);
            if (m_videoCodecCtx) {
                avcodec_flush_buffers(m_videoCodecCtx);
            }
            if (m_audioCodecCtx) {
                avcodec_flush_buffers(m_audioCodecCtx);
            }
            m_videoQueue.clear();
            m_audioQueue.clear();
            m_audioClock = targetMs / 1000.0;
            resetClock(m_audioClock);
        }

        int ret = av_read_frame(m_fmtCtx, packet);
        if (ret < 0) {
            if (ret == AVERROR_EOF) {
                if (m_videoCodecCtx) {
                    avcodec_send_packet(m_videoCodecCtx, nullptr);
                    while (avcodec_receive_frame(m_videoCodecCtx, frame) == 0) {
                        processVideoFrame(frame);
                    }
                }
                if (m_audioCodecCtx) {
                    avcodec_send_packet(m_audioCodecCtx, nullptr);
                    while (avcodec_receive_frame(m_audioCodecCtx, frame) == 0) {
                        processAudioFrame(frame);
                    }
                }
                emit finished();
            }
            break;
        }

        if (packet->stream_index == m_videoStreamIndex) {
            ret = avcodec_send_packet(m_videoCodecCtx, packet);
            if (ret >= 0) {
                while (avcodec_receive_frame(m_videoCodecCtx, frame) == 0) {
                    processVideoFrame(frame);
                }
            }
        } else if (packet->stream_index == m_audioStreamIndex) {
            ret = avcodec_send_packet(m_audioCodecCtx, packet);
            if (ret >= 0) {
                while (avcodec_receive_frame(m_audioCodecCtx, frame) == 0) {
                    processAudioFrame(frame);
                }
            }
        }

        av_packet_unref(packet);
    }

    av_frame_free(&frame);
    av_packet_free(&packet);

    m_state.storeRelaxed(int(PlayState::Stopped));
    emit stateChanged(PlayState::Stopped);
}

void FFmpegPlayer::processVideoFrame(AVFrame *frame)
{
    if (!m_swsCtx || !m_rgbBuffer) {
        return;
    }

    double pts = framePtsSec(frame, m_fmtCtx->streams[m_videoStreamIndex]);
    if (pts < 0.0) {
        pts = 0.0;
    }
    syncToClock(pts, 0);

    sws_scale(m_swsCtx, frame->data, frame->linesize, 0, m_videoHeight,
              m_rgbTmpData, m_rgbTmpLinesize);

    QImage image(m_rgbTmpData[0], m_videoWidth, m_videoHeight,
                 m_rgbTmpLinesize[0], QImage::Format_RGB888);

    VideoFrame vf;
    vf.image = image.copy();
    vf.pts = pts;
    m_videoQueue.push(vf);

    if (m_audioStreamIndex < 0) {
        m_positionMs.storeRelaxed(qint64(pts * 1000));
        emit positionChanged(m_positionMs.loadRelaxed());
    }
}

void FFmpegPlayer::processAudioFrame(AVFrame *frame)
{
    if (!m_swrCtx) {
        return;
    }

    double pts = framePtsSec(frame, m_fmtCtx->streams[m_audioStreamIndex]);
    if (pts < 0.0) {
        pts = qMax(0.0, m_audioClock);
    }

    int outSamples = swr_get_out_samples(m_swrCtx, frame->nb_samples);
    if (outSamples <= 0) {
        outSamples = frame->nb_samples * 2;
    }

    uint8_t *outData = nullptr;
    int outLineSize = 0;
    const int allocRet = av_samples_alloc(&outData, &outLineSize, m_audioChannels, outSamples,
                                          m_audioSampleFmt, 0);
    if (allocRet < 0) {
        return;
    }

    const int converted = swr_convert(m_swrCtx, &outData, outSamples,
                                      const_cast<const uint8_t **>(frame->data), frame->nb_samples);
    if (converted > 0) {
        const int dataSize = av_samples_get_buffer_size(nullptr, m_audioChannels, converted,
                                                        m_audioSampleFmt, 1);
        if (dataSize > 0) {
            AudioChunk chunk;
            chunk.data = QByteArray(reinterpret_cast<const char *>(outData), dataSize);
            chunk.pts = pts;
            m_audioQueue.push(chunk);

            m_audioClock = pts;
            m_positionMs.storeRelaxed(qint64(m_audioClock * 1000));
            emit positionChanged(m_positionMs.loadRelaxed());
        }
    }

    av_freep(&outData);
}
