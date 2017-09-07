
/*
 * @file demo.c
 * @author gongjia
 *
 * FFmpeg push RTMP stream
 *
 * Implements the following command:
 *
 * ffmpeg -re -i input.flv -vcodec copy -acodec copy -f flv -y rtmp://xxxx
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <dirent.h>
#include <sys/time.h>
#include <assert.h>


#include <libavformat/avformat.h>
#include <libavutil/time.h>
#include <libavutil/timestamp.h>
#include <libswscale/swscale.h>
#include "logjni.h"

#define   FRAMENUM    256
#define   FRAME_RATE  25

void copy_stream_info(AVStream *ostream, AVStream *istream, AVFormatContext *ofmt_ctx) {
    AVCodecContext *icodec = istream->codec;
    AVCodecContext *ocodec = ostream->codec;

    ostream->id = istream->id;
    ocodec->codec_id = icodec->codec_id;
    ocodec->codec_type = icodec->codec_type;
    ocodec->bit_rate = icodec->bit_rate;

    int extra_size = (uint64_t)icodec->extradata_size + FF_INPUT_BUFFER_PADDING_SIZE;
    ocodec->extradata = (uint8_t *)av_mallocz(extra_size);
    memcpy(ocodec->extradata, icodec->extradata, icodec->extradata_size);
    ocodec->extradata_size = icodec->extradata_size;

    // Some formats want stream headers to be separate.
    if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER) {
        ostream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
    }
}

void copy_video_stream_info(AVStream *ostream, AVStream *istream, AVFormatContext *ofmt_ctx) {
    copy_stream_info(ostream, istream, ofmt_ctx);

    AVCodecContext *icodec = istream->codec;
    AVCodecContext *ocodec = ostream->codec;

    ocodec->width = icodec->width;
    ocodec->height = icodec->height;
    ocodec->time_base = icodec->time_base;
    ocodec->gop_size = icodec->gop_size;
    ocodec->pix_fmt = icodec->pix_fmt;
}

int v2v_repeat(char *in_filename, char *out_filename, int speed) {
    AVOutputFormat *ofmt = NULL;
    AVFormatContext *ifmt_ctx = NULL; // Input AVFormatContext
    AVFormatContext *ofmt_ctx = NULL; // Output AVFormatContext
    AVDictionaryEntry *tag = NULL;
    AVStream *in_stream = NULL;
    AVStream *out_stream = NULL;
    AVPacket pkt;
    AVCodec *codec = NULL;

    int ret = 0;
    int i = 0;
    int video_index = 0;
    int64_t last_pts = 0;

    memset(&pkt, 0, sizeof(pkt));

    av_register_all();

    // Network
    avformat_network_init();

    // Input
    if ((ret = avformat_open_input(&ifmt_ctx, in_filename, 0, 0)) < 0) {
        LOG("Open input file failed. %s", in_filename);
        goto end;
    }

    if ((ret = avformat_find_stream_info(ifmt_ctx, 0)) < 0) {
        LOG("Retrieve input stream info failed");
        goto end;
    }

    LOG("ifmt_ctx->nb_streams = %u", ifmt_ctx->nb_streams);

    // Find the first video stream
    video_index = -1;
    for (i = 0; i < ifmt_ctx->nb_streams; ++i) {
        if (AVMEDIA_TYPE_VIDEO == ifmt_ctx->streams[i]->codec->codec_type) {
            video_index = i;
            break;
        }
    }

    if (-1 == video_index) {
        LOG("Didn't find a video stream.");
        goto end;
    }
    LOG("video_index = %d\n", video_index);

    av_dump_format(ifmt_ctx, 0, in_filename, 0);

    // Output
    avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, out_filename);

    if (!ofmt_ctx) {
        LOG("Create output context failed.");
        ret = AVERROR_UNKNOWN;
        goto end;
    }

    ofmt = ofmt_ctx->oformat;

    if ((video_index = av_find_best_stream(ifmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0)) >= 0) {
        in_stream = ifmt_ctx->streams[video_index];
        out_stream = avformat_new_stream(ofmt_ctx, NULL);

        if (!out_stream) {
            LOG("Allocate output stream failed.");
            ret = AVERROR_UNKNOWN;
            goto end;
        }
        copy_video_stream_info(out_stream, in_stream, ofmt_ctx);
    }

    // Copy metadata
    while ((tag = av_dict_get(ifmt_ctx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
        LOG("%s = %s", tag->key, tag->value);
        av_dict_set(&ofmt_ctx->metadata, tag->key, tag->value, 0);
    }
    // Dump Format
    av_dump_format(ofmt_ctx, 0, out_filename, 1);

    // Open output URL
    if (!(ofmt->flags & AVFMT_NOFILE)) {
        ret = avio_open(&ofmt_ctx->pb, out_filename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            LOG("Open output URL '%s' failed", out_filename);
            goto end;
        }
    }

    // Write file header
    ret = avformat_write_header(ofmt_ctx, NULL);
    if (ret < 0) {
        LOG("Write output URL failed.");
        goto end;
    }

    av_init_packet(&pkt);

    codec = avcodec_find_encoder_by_name("libx264");
    if (!codec){
        LOG("libx264 Codec not found\n");
    }
    while (1) {
        ret = av_read_frame(ifmt_ctx, &pkt);
        if(ret < 0){
            av_free_packet(&pkt); 
            break;
        }
          
        if(pkt.stream_index == video_index){
            // LOG("packet pts = %lld, size = %d, stream = %d\n", pkt.pts, pkt.size, pkt.stream_index);
            pkt.pts /= speed;
            pkt.dts /= speed;
            last_pts = pkt.pts;
            if (av_interleaved_write_frame(ofmt_ctx, &pkt) < 0) {
                LOG("error av_interleaved_write_frame\n");
                break;
            }
            av_free_packet(&pkt);
        }
    }
   
    for(i=0; i < speed-1; i++){
        avformat_close_input(&ifmt_ctx);
        // Input
        if ((ret = avformat_open_input(&ifmt_ctx, in_filename, 0, 0)) < 0) {
            LOG("Open input file failed.");
            goto end;
        }

        if ((ret = avformat_find_stream_info(ifmt_ctx, 0)) < 0) {
            LOG("Retrieve input stream info failed");
            goto end;
        }

        while (1) {
            ret = av_read_frame(ifmt_ctx, &pkt);
            if(ret < 0){
                av_free_packet(&pkt); 
                LOG("");
                break;
            }
            if(pkt.stream_index == video_index){
         
                pkt.pts /= speed;
                pkt.dts /= speed;

                pkt.pts += last_pts*(i+1)+i+1;
                pkt.dts += last_pts*(i+1)+i+1;

                if (av_interleaved_write_frame(ofmt_ctx, &pkt) < 0) {
                    LOG("\n");
                    break;
                }
                av_free_packet(&pkt);
            }
        }
    }

    // Write file trailer
    av_write_trailer(ofmt_ctx);

    end:
        avformat_close_input(&ifmt_ctx);
        /* close output */
        if (ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE)) {
            avio_close(ofmt_ctx->pb);
        }
        avformat_free_context(ofmt_ctx);
        if (ret < 0 && ret != AVERROR_EOF) {
            LOG( "Error occurred.");
            return -1;
        }

    return 0;
}

int v2v_timeback(char *in_filename, char *out_filename) {
    AVOutputFormat *ofmt = NULL;
    AVFormatContext *ifmt_ctx = NULL; // Input AVFormatContext
    AVFormatContext *ofmt_ctx = NULL; // Output AVFormatContext
    AVDictionaryEntry *tag = NULL;
    AVStream *in_stream = NULL;
    AVStream *out_stream = NULL;
    AVFrame *pFrames[FRAMENUM];

    static FILE*fp=NULL;
    static AVCodecContext *dec_ctx;
    static AVCodecContext *enc_ctx;

    AVCodec *dec;
    AVCodec *enc;
    AVPacket pkt;

    int ret = 0;
    int i = 0;
    int j = 0;
    int video_index = 0;
    int framecnt = 0;
    int got_output = 0;
    int64_t frame_pts[FRAMENUM];
    uint8_t endcode[] = { 0, 0, 1, 0xb7 };

    memset(&pkt, 0, sizeof(pkt));
    av_register_all();
    avformat_network_init();

    // Input
    if ((ret = avformat_open_input(&ifmt_ctx, in_filename, 0, 0)) < 0) {
        LOG("Open input file failed.");
        goto end;
    }
    if ((ret = avformat_find_stream_info(ifmt_ctx, 0)) < 0) {
        LOG("Retrieve input stream info failed");
        goto end;
    }

    // Find the first video stream
    video_index = -1;
    for (i = 0; i < ifmt_ctx->nb_streams; ++i) {
        if (AVMEDIA_TYPE_VIDEO == ifmt_ctx->streams[i]->codec->codec_type) {
            video_index = i;
            break;
        }
    }
    if (-1 == video_index) {
        LOG("Didn't find a video stream.");
        goto end;
    }

    av_dump_format(ifmt_ctx, 0, in_filename, 0);

    // Create stream
    if ((video_index = av_find_best_stream(ifmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &dec, 0)) >= 0) {
        in_stream = ifmt_ctx->streams[video_index];

        dec_ctx = avcodec_alloc_context3(dec);
        if (!dec_ctx)
            return AVERROR(ENOMEM);

        /* init the video decoder */
        if ((ret = avcodec_open2(dec_ctx, dec, NULL)) < 0) {
            LOG( "Cannot open video decoder\n");
            return ret;
        }
        ///////////////////////////////////////////////////////////////////////
        // if (!(enc = avcodec_find_encoder(AV_CODEC_ID_H264))) {  //bug
        if (!(enc = avcodec_find_encoder_by_name("mpeg1video"))) {  //libx264 mpeg1video
            av_log(NULL, AV_LOG_ERROR, "Snow encoder is not enabled in libavcodec\n");
            return AVERROR(EINVAL);
        }
        enc_ctx = avcodec_alloc_context3(enc);
        if (!enc_ctx)
            return AVERROR(ENOMEM);

        AVCodecContext *icodec = in_stream->codec;
        LOG("icodec->width=%d,  icodec->codec_type=%d, gop_size=%d\n", icodec->width,  icodec->codec_type, icodec->gop_size);
        enc_ctx->width  = icodec->width;
        enc_ctx->height = icodec->height;
        enc_ctx->time_base =  (AVRational){1, FRAME_RATE};
        enc_ctx->framerate =  (AVRational){FRAME_RATE, 1};
        enc_ctx->max_b_frames = 0;
        enc_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
        enc_ctx->bit_rate = icodec->bit_rate;
        enc_ctx->gop_size = icodec->gop_size;
        enc_ctx->codec_type = icodec->codec_type;

        //ultrafast, superfast, veryfast, faster, fast, medium, slow, slower, veryslow, placebo.
        av_opt_set(enc_ctx->priv_data, "preset", "slow", 0);

        LOG("enc_ctx->bit_rate=%ld, enc_ctx->codec_type=%d\n", enc_ctx->bit_rate, enc_ctx->codec_type);

        if (avcodec_open2(enc_ctx, enc, NULL) < 0) {
            LOG("Could not open codec\n");
            return -1;
        }

        for(i=0; i<FRAMENUM; i++){
            pFrames[i] = av_frame_alloc();
            if (!pFrames[i]) {
                LOG("Could not allocate video frame\n");
                return -1;
            }
            pFrames[i]->format = enc_ctx->pix_fmt;
            pFrames[i]->width  = enc_ctx->width;
            pFrames[i]->height = enc_ctx->height;

            ret = av_frame_get_buffer(pFrames[i], 32);
            if (ret < 0) {
                LOG( "Could not allocate the video frame data\n");
                return -1;
            }
        }
    }
    av_init_packet(&pkt);

    // 读文件
    while (1) {
        ret = av_read_frame(ifmt_ctx, &pkt);
        if(ret < 0){
            av_free_packet(&pkt);
            break;
        }
        // decode
        ret = avcodec_send_packet(dec_ctx, &pkt);
        av_free_packet(&pkt);

        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Error while sending a packet to the decoder\n");
            break;
        }

        while (ret >= 0) {
            //  get frame
            ret = avcodec_receive_frame(dec_ctx, pFrames[framecnt]);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    break;
                } else if (ret < 0) {
                    av_log(NULL, AV_LOG_ERROR, "Error while receiving a frame from the decoder\n");
                    goto end;
                }

                if (ret >= 0) {
                    pFrames[framecnt]->pts = av_frame_get_best_effort_timestamp(pFrames[framecnt]);
                    framecnt ++;
                }
            }
    }

    // 交换首尾frame的pts
    for(i = framecnt-1; i>=0; i--) {
        frame_pts[j] = pFrames[i]->pts;
        j++;
    }
    for(i=0; i<framecnt; i++) {
        pFrames[i]->pts = frame_pts[i];

    }

    // 开始编码  写文件
    for(i = framecnt-1; i>=0; i--) {
        av_init_packet(&pkt);
        pkt.data = NULL;    // packet data will be allocated by the encoder
        pkt.size = 0;

        ret = av_frame_make_writable(pFrames[i]);
        if (ret < 0){
             LOG("Error av_frame_make_writable\n");
        }
        // encode the image
        ret = avcodec_encode_video2(enc_ctx, &pkt, pFrames[i], &got_output);
        if (ret < 0) {
            LOG("Error encoding frame\n");
            goto end;
        }

        if(pkt.pts < 0 || pkt.dts < 0){
            av_packet_unref(&pkt);
            continue;
        }

        if (got_output) {
            LOG("Succeed to encode frame: %5d\t size:%5d, flags=%d, pts=%ld\n", i, pkt.size, pkt.flags, pkt.pts);
            if(NULL==fp)
                fp=fopen(out_filename,"wb");
            if(fp)
            {
                fwrite(pkt.data, 1, pkt.size, fp);
            }
        }

        av_packet_unref(&pkt);
    }

    fwrite(endcode, 1, sizeof(endcode), fp);
    fclose(fp);

end:
        avformat_close_input(&ifmt_ctx);
        for(i=0; i<FRAMENUM; i++){
            av_frame_free(&pFrames[i]);
        }
        if (ret < 0 && ret != AVERROR_EOF) {
            LOG("Error occurred.");
            return -1;
        }

    return 0;
}