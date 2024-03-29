/*
 * Copyright (C) 2008-2012 The QXmpp developers
 *
 * Author:
 *  Jeremy Lainé
 *
 * Source:
 *  http://code.google.com/p/qxmpp
 *
 * This file is a part of QXmpp library.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 */

/*
 *  G.711 based on reference implementation by Sun Microsystems, Inc.
 */

#include <QDataStream>
#include <QDebug>
#include <QSize>
#include <QMutex>
#include <QMutexLocker>

#include "QXmppCodec_p.h"
#include "QXmppRtpChannel.h"

#include <cstring>

#ifdef QXMPP_USE_SPEEX
#include <speex/speex.h>
#endif

#define BIAS        (0x84)  /* Bias for linear code. */
#define CLIP        8159

#define SIGN_BIT    (0x80)  /* Sign bit for a A-law byte. */
#define QUANT_MASK  (0xf)   /* Quantization field mask. */
#define NSEGS       (8)     /* Number of A-law segments. */
#define SEG_SHIFT   (4)     /* Left shift for segment number. */
#define SEG_MASK    (0x70)  /* Segment field mask. */

enum FragmentType {
    NoFragment = 0,
    StartFragment,
    MiddleFragment,
    EndFragment,
};

static qint16 seg_aend[8] = {0x1F, 0x3F, 0x7F, 0xFF,
                0x1FF, 0x3FF, 0x7FF, 0xFFF};
static qint16 seg_uend[8] = {0x3F, 0x7F, 0xFF, 0x1FF,
                0x3FF, 0x7FF, 0xFFF, 0x1FFF};

static qint16 search(qint16 val, qint16 *table, qint16 size)
{
   qint16 i;

   for (i = 0; i < size; i++) {
      if (val <= *table++)
     return (i);
   }
   return (size);
}

/*
 * linear2alaw() - Convert a 16-bit linear PCM value to 8-bit A-law
 *
 * Accepts a 16-bit integer and encodes it as A-law data.
 *
 *      Linear Input Code   Compressed Code
 *  ------------------------    ---------------
 *  0000000wxyza            000wxyz
 *  0000001wxyza            001wxyz
 *  000001wxyzab            010wxyz
 *  00001wxyzabc            011wxyz
 *  0001wxyzabcd            100wxyz
 *  001wxyzabcde            101wxyz
 *  01wxyzabcdef            110wxyz
 *  1wxyzabcdefg            111wxyz
 *
 * For further information see John C. Bellamy's Digital Telephony, 1982,
 * John Wiley & Sons, pps 98-111 and 472-476.
 */
static quint8 linear2alaw(qint16 pcm_val)
{
   qint16 mask;
   qint16 seg;
   quint8 aval;

   pcm_val = pcm_val >> 3;

   if (pcm_val >= 0) {
      mask = 0xD5;      /* sign (7th) bit = 1 */
   } else {
      mask = 0x55;      /* sign bit = 0 */
      pcm_val = -pcm_val - 1;
   }

   /* Convert the scaled magnitude to segment number. */
   seg = search(pcm_val, seg_aend, 8);

   /* Combine the sign, segment, and quantization bits. */

   if (seg >= 8)        /* out of range, return maximum value. */
      return (quint8) (0x7F ^ mask);
   else {
      aval = (quint8) seg << SEG_SHIFT;
      if (seg < 2)
     aval |= (pcm_val >> 1) & QUANT_MASK;
      else
     aval |= (pcm_val >> seg) & QUANT_MASK;
      return (aval ^ mask);
   }
}

/*
 * alaw2linear() - Convert an A-law value to 16-bit linear PCM
 *
 */
static qint16 alaw2linear(quint8 a_val)
{
   qint16 t;
   qint16 seg;

   a_val ^= 0x55;

   t = (a_val & QUANT_MASK) << 4;
   seg = ((qint16)a_val & SEG_MASK) >> SEG_SHIFT;
   switch (seg) {
   case 0:
      t += 8;
      break;
   case 1:
      t += 0x108;
      break;
   default:
      t += 0x108;
      t <<= seg - 1;
   }
   return ((a_val & SIGN_BIT) ? t : -t);
}

/*
 * linear2ulaw() - Convert a linear PCM value to u-law
 *
 * In order to simplify the encoding process, the original linear magnitude
 * is biased by adding 33 which shifts the encoding range from (0 - 8158) to
 * (33 - 8191). The result can be seen in the following encoding table:
 *
 *   Biased Linear Input Code    Compressed Code
 *   ------------------------    ---------------
 *   00000001wxyza           000wxyz
 *   0000001wxyzab           001wxyz
 *   000001wxyzabc           010wxyz
 *   00001wxyzabcd           011wxyz
 *   0001wxyzabcde           100wxyz
 *   001wxyzabcdef           101wxyz
 *   01wxyzabcdefg           110wxyz
 *   1wxyzabcdefgh           111wxyz
 *
 * Each biased linear code has a leading 1 which identifies the segment
 * number. The value of the segment number is equal to 7 minus the number
 * of leading 0's. The quantization interval is directly available as the
 * four bits wxyz.  * The trailing bits (a - h) are ignored.
 *
 * Ordinarily the complement of the resulting code word is used for
 * transmission, and so the code word is complemented before it is returned.
 *
 * For further information see John C. Bellamy's Digital Telephony, 1982,
 * John Wiley & Sons, pps 98-111 and 472-476.
 */
static quint8 linear2ulaw(qint16 pcm_val)
{
   qint16 mask;
   qint16 seg;
   quint8 uval;

   /* Get the sign and the magnitude of the value. */
   pcm_val = pcm_val >> 2;
   if (pcm_val < 0) {
      pcm_val = -pcm_val;
      mask = 0x7F;
   } else {
      mask = 0xFF;
   }
   if (pcm_val > CLIP) pcm_val = CLIP;        /* clip the magnitude */
   pcm_val += (BIAS >> 2);

   /* Convert the scaled magnitude to segment number. */
   seg = search(pcm_val, seg_uend, 8);

   /*
   * Combine the sign, segment, quantization bits;
   * and complement the code word.
   */
   if (seg >= 8)        /* out of range, return maximum value. */
      return (quint8) (0x7F ^ mask);
   else {
      uval = (quint8) (seg << 4) | ((pcm_val >> (seg + 1)) & 0xF);
      return (uval ^ mask);
   }
}

/*
 * ulaw2linear() - Convert a u-law value to 16-bit linear PCM
 *
 * First, a biased linear code is derived from the code word. An unbiased
 * output can then be obtained by subtracting 33 from the biased code.
 *
 * Note that this function expects to be passed the complement of the
 * original code word. This is in keeping with ISDN conventions.
 */
static qint16 ulaw2linear(quint8 u_val)
{
   qint16 t;

   /* Complement to obtain normal u-law value. */
   u_val = ~u_val;

   /*
    * Extract and bias the quantization bits. Then
    * shift up by the segment number and subtract out the bias.
    */
   t = ((u_val & QUANT_MASK) << 3) + BIAS;
   t <<= ((unsigned)u_val & SEG_MASK) >> SEG_SHIFT;

   return ((u_val & SIGN_BIT) ? (BIAS - t) : (t - BIAS));
}

QXmppCodec::~QXmppCodec()
{
}

QXmppVideoDecoder::~QXmppVideoDecoder()
{
}

QXmppVideoEncoder::~QXmppVideoEncoder()
{
}

QXmppG711aCodec::QXmppG711aCodec(int clockrate)
{
    m_frequency = clockrate;
}

qint64 QXmppG711aCodec::encode(QDataStream &input, QDataStream &output)
{
    qint64 samples = 0;
    qint16 pcm;
    while (!input.atEnd())
    {
        input >> pcm;
        output << linear2alaw(pcm);
        ++samples;
    }
    return samples;
}

qint64 QXmppG711aCodec::decode(QDataStream &input, QDataStream &output)
{
    qint64 samples = 0;
    quint8 g711;
    while (!input.atEnd())
    {
        input >> g711;
        output << alaw2linear(g711);
        ++samples;
    }
    return samples;
}

QXmppG711uCodec::QXmppG711uCodec(int clockrate)
{
    m_frequency = clockrate;
}

qint64 QXmppG711uCodec::encode(QDataStream &input, QDataStream &output)
{
    qint64 samples = 0;
    qint16 pcm;
    while (!input.atEnd())
    {
        input >> pcm;
        output << linear2ulaw(pcm);
        ++samples;
    }
    return samples;
}

qint64 QXmppG711uCodec::decode(QDataStream &input, QDataStream &output)
{
    qint64 samples = 0;
    quint8 g711;
    while (!input.atEnd())
    {
        input >> g711;
        output << ulaw2linear(g711);
        ++samples;
    }
    return samples;
}

#ifdef QXMPP_USE_SPEEX
QXmppSpeexCodec::QXmppSpeexCodec(int clockrate)
{
    const SpeexMode *mode = &speex_nb_mode;
    if (clockrate == 32000)
        mode = &speex_uwb_mode;
    else if (clockrate == 16000)
        mode = &speex_wb_mode;
    else if (clockrate == 8000)
        mode = &speex_nb_mode;
    else
        qWarning() << "QXmppSpeexCodec got invalid clockrate" << clockrate;

    // encoder
    encoder_bits = new SpeexBits;
    speex_bits_init(encoder_bits);
    encoder_state = speex_encoder_init(mode);

    // decoder
    decoder_bits = new SpeexBits;
    speex_bits_init(decoder_bits);
    decoder_state = speex_decoder_init(mode);

    // get frame size in samples
    speex_encoder_ctl(encoder_state, SPEEX_GET_FRAME_SIZE, &frame_samples);
}

QXmppSpeexCodec::~QXmppSpeexCodec()
{
    delete encoder_bits;
    delete decoder_bits;
}

qint64 QXmppSpeexCodec::encode(QDataStream &input, QDataStream &output)
{
    QByteArray pcm_buffer(frame_samples * 2, 0);
    const int length = input.readRawData(pcm_buffer.data(), pcm_buffer.size());
    if (length != pcm_buffer.size())
    {
        qWarning() << "Read only read" << length << "bytes";
        return 0;
    }
    speex_bits_reset(encoder_bits);
    speex_encode_int(encoder_state, (short*)pcm_buffer.data(), encoder_bits);
    QByteArray speex_buffer(speex_bits_nbytes(encoder_bits), 0);
    speex_bits_write(encoder_bits, speex_buffer.data(), speex_buffer.size());
    output.writeRawData(speex_buffer.data(), speex_buffer.size());
    return frame_samples;
}

qint64 QXmppSpeexCodec::decode(QDataStream &input, QDataStream &output)
{
    const int length = input.device()->bytesAvailable();
    QByteArray speex_buffer(length, 0);
    input.readRawData(speex_buffer.data(), speex_buffer.size());
    speex_bits_read_from(decoder_bits, speex_buffer.data(), speex_buffer.size());
    QByteArray pcm_buffer(frame_samples * 2, 0);
    speex_decode_int(decoder_state, decoder_bits, (short*)pcm_buffer.data());
    output.writeRawData(pcm_buffer.data(), pcm_buffer.size());
    return frame_samples;
}

#endif

class QXmppFFmpegDecoderPrivate
{
public:
   int bitrate;
   AVCodec* codec;
   AVCodecContext* codecContext;
   SwsContext* scaler;
};

//TODO: move ffmpeg initialization here
QXmppFFmpegDecoder::QXmppFFmpegDecoder(CodecID codecID)
{
    d = new QXmppFFmpegDecoderPrivate;
    d->codec = avcodec_find_decoder(codecID);
    d->codecContext = avcodec_alloc_context3(d->codec);
    d->codecContext->strict_std_compliance = -2;
    if(avcodec_open2(d->codecContext,d->codec,0) < 0) {
        qWarning("Couldn't initialize h264 decoder");
    }
    d->scaler = 0;
}

QXmppFFmpegDecoder::~QXmppFFmpegDecoder()
{
    avcodec_close(d->codecContext);
    delete d;
}

QXmppVideoFormat QXmppFFmpegDecoder::format() const
{
    QXmppVideoFormat format;
    format.setFrameRate(d->codecContext->time_base.den);
    format.setFrameSize(QSize(d->codecContext->width, d->codecContext->height));
    format.setPixelFormat(d->codecContext->pix_fmt);
    format.setBitrate(d->codecContext->bit_rate);
    format.setGopSize(d->codecContext->gop_size);
    return format;
}

QList<AVFrame*> QXmppFFmpegDecoder::handlePacket(const QXmppRtpPacket &packet)
{
   QList<AVFrame*> frameList;
   AVPacket* pkt = new AVPacket;
   av_init_packet(pkt);
   //TODO: Timestamps?
   pkt->size = packet.payload.size();
   pkt->data = (uchar*) packet.payload.data();
   AVFrame* frame = avcodec_alloc_frame();
   int got_picture = 0;
   if(avcodec_decode_video2(d->codecContext,frame,&got_picture,pkt) < 0) {
      return frameList;
   }
   if(!got_picture) { av_free_packet(pkt); return frameList; }
   frameList << frame;
   return frameList;
}

bool QXmppFFmpegDecoder::setParameters(const QMap<QString, QString> &parameters)
{
    return true;
}

class QXmppFFmpegEncoderPrivate
{
public:
   int bitrate;
   AVCodec* codec;
   AVCodecContext* codecContext;
   SwsContext* scaler;
   int64_t pts;
   QMutex formatLocker;
};

QXmppFFmpegEncoder::QXmppFFmpegEncoder(CodecID codecID) {
   qDebug("Initializing ffmpeg encoder");
   d = new QXmppFFmpegEncoderPrivate;
   d->scaler = 0;
   d->codec = avcodec_find_encoder(codecID);
   if(!d->codec) qWarning("Encoder not found");
   d->codecContext = 0;
   d->pts = 0;
   QXmppVideoFormat format;
    format.setFrameRate(30.0);
    format.setFrameSize(QSize(640, 480));
    if(d->codec->pix_fmts)
       format.setPixelFormat(d->codec->pix_fmts[0]);
    else
       format.setPixelFormat(PIX_FMT_YUV420P);
    format.setGopSize(5);
    format.setBitrate(800000);
    format.setQscale(-1);
   setFormat(format);
}

QXmppFFmpegEncoder::~QXmppFFmpegEncoder() {
   if(d->codecContext) {
      qDebug("Freeing codec context as its a destructor");
      avcodec_close(d->codecContext);
      av_free(d->codecContext);
   }
   delete d;
}

bool QXmppFFmpegEncoder::setFormat(const QXmppVideoFormat &format)
{
   d->formatLocker.lock();
   qDebug("Setting up encoder format");
   if(d->codecContext) {
      qDebug("Freeing codec context as we have it");
      avcodec_close(d->codecContext);
      av_free(d->codecContext);
      qDebug("Freeing codec context successfully");
   }
    d->codecContext = avcodec_alloc_context3(d->codec);
    //TODO: Properly detect pix_fmt
    if(d->codec->pix_fmts) 
       d->codecContext->pix_fmt = d->codec->pix_fmts[0];
    else {
       d->codecContext->pix_fmt = format.pixelFormat();
    }
    d->codecContext->width = format.frameWidth();
    d->codecContext->height = format.frameHeight();
    d->codecContext->time_base.num = 1;
    d->codecContext->time_base.den = qRound(format.frameRate());
    d->codecContext->gop_size = format.gopSize();
    d->codecContext->bit_rate = format.bitrate();
    d->codecContext->strict_std_compliance = -2;
    d->codecContext->max_b_frames = 0;
    d->codecContext->thread_count = 2;
    if(format.qscale() > 0) {
       d->codecContext->flags |= CODEC_FLAG_QSCALE;
       d->codecContext->global_quality = FF_QP2LAMBDA * format.qscale();
    }
    if(avcodec_open2(d->codecContext,d->codec,0)<0) {
        qWarning("Couldn't initialize encoder");
        d->formatLocker.unlock();
        return false;
    }
    qDebug("Successfully set up encoder format");
    d->formatLocker.unlock();
    return true;
}

QList<QByteArray> QXmppFFmpegEncoder::handleFrame(AVFrame *frame)
{
   d->formatLocker.lock();
   AVFrame* newFrame = avcodec_alloc_frame();
   d->scaler = sws_getCachedContext( d->scaler, frame->width, frame->height
                                , (PixelFormat)frame->format
                                , d->codecContext->width, d->codecContext->height, d->codecContext->pix_fmt
                                , SWS_BICUBIC, 0, 0, 0);
   avpicture_alloc( (AVPicture*)newFrame, d->codecContext->pix_fmt, d->codecContext->width
                  , d->codecContext->height);
   sws_scale( d->scaler, frame->data, frame->linesize, 0, frame->height
            , newFrame->data, newFrame->linesize);
   newFrame->width = d->codecContext->width;
   newFrame->height = d->codecContext->height;
   newFrame->pts = frame->pts;

   QList<QByteArray> packets;    
   AVPacket* pkt = new AVPacket();
   av_init_packet(pkt);
   pkt->size = 0;
   int got_packet = 0;
   if(avcodec_encode_video2(d->codecContext,pkt,newFrame,&got_packet) < 0) {
      d->formatLocker.unlock();
      return packets;
   }
   if(!got_packet) { av_free_packet(pkt); d->formatLocker.unlock(); return packets; }
   QByteArray packet((const char*)pkt->data,pkt->size);
   packets << packet;
   av_free_packet(pkt);
   d->formatLocker.unlock();
   avpicture_free((AVPicture*)newFrame);
   av_free(newFrame);
   return packets;
}

QMap<QString, QString> QXmppFFmpegEncoder::parameters() const
{
    return QMap<QString, QString>();
}
