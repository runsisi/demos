extern "C" {
#include <libavdevice/avdevice.h>
#include <libavfilter/avfilter.h>
#include <libavcodec/avcodec.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
#include <libavutil/log.h>
}

// https://github.com/FFmpeg/FFmpeg/blob/master/doc/APIchanges
#if LIBAVFORMAT_VERSION_MAJOR > 59 || \
        LIBAVFORMAT_VERSION_MAJOR == 59 && LIBAVFORMAT_VERSION_MINOR > 0 || \
        LIBAVFORMAT_VERSION_MAJOR == 59 && LIBAVFORMAT_VERSION_MINOR == 0 && LIBAVFORMAT_VERSION_MICRO >= 100
#define FFMPEG_AVFORMAT_CONSTIFIED
#endif

#include <stdio.h>

#include <nanobench.h>

namespace bench = ankerl::nanobench;

int running = 1000;
int64_t pts = 0;

int main() {
    bench::Bench bench;
    // default output is &std::cout, so .output(&std::cout) can be omitted
    bench.epochs(running).epochIterations(1);
    bench::detail::IterationLogic epoch(bench);

    av_log_set_level(AV_LOG_ERROR);

    avdevice_register_all();

    // input

#ifdef FFMPEG_AVFORMAT_CONSTIFIED
    const
#endif
    AVInputFormat *iformat = av_find_input_format("kmsgrab");
    if (!iformat) {
        fprintf(stderr, "kmsgrab format not find\n");
        exit(1);
    }

    AVDictionary *iformat_opts = NULL;
    av_dict_set(&iformat_opts, "device", "/dev/dri/card0", 0);
    av_dict_set_int(&iformat_opts, "framerate", 60, 0);

    AVFormatContext *ifctx = NULL;
    int r = avformat_open_input(&ifctx, NULL, iformat, &iformat_opts);
    if (r < 0) {
        fprintf(stderr, "avformat_open_input failed: %d\n", r);
        exit(1);
    }

    av_dict_free(&iformat_opts);

    // decoder

    AVStream *ist = ifctx->streams[0];
    const AVCodec *dec = avcodec_find_decoder(ist->codecpar->codec_id);

    AVCodecContext *decoder = avcodec_alloc_context3(dec);
    if (!decoder) {
        fprintf(stderr, "avcodec_alloc_context3 failed\n");
        exit(1);
    }

    r = avcodec_parameters_to_context(decoder, ist->codecpar);
    if (r < 0) {
        fprintf(stderr, "avcodec_parameters_to_context failed: %d\n", r);
        exit(1);
    }

    r = avcodec_open2(decoder, dec, NULL);
    if (r < 0) {
        fprintf(stderr, "avcodec_open2 failed: %d\n", r);
        exit(1);
    }

    // encoder

    const AVCodec *enc = avcodec_find_encoder_by_name("h264_vaapi");
    if (!enc) {
        fprintf(stderr, "avcodec_find_encoder_by_name failed\n");
        exit(1);
    }

    AVCodecContext *encoder = avcodec_alloc_context3(enc);
    if (!encoder) {
        fprintf(stderr, "avcodec_alloc_context3 failed\n");
        exit(1);
    }

    encoder->codec_type = AVMEDIA_TYPE_VIDEO;

    // output

    AVFormatContext *ofctx = NULL;
    r = avformat_alloc_output_context2(&ofctx, NULL, "h264", NULL);
    if (r < 0) {
        fprintf(stderr, "avformat_alloc_output_context2 failed: %d\n", r);
        exit(1);
    }

    AVStream *ost = avformat_new_stream(ofctx, NULL);
    if (!ost) {
        fprintf(stderr, "avformat_new_stream failed\n");
        exit(1);
    }

    ost->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
    ost->codecpar->codec_id = enc->id;

    r = avio_open2(&ofctx->pb, "x.h264", AVIO_FLAG_WRITE, NULL, NULL);
    if (r < 0) {
        fprintf(stderr, "avio_open2 failed: %d\n", r);
        exit(1);
    }

    AVBufferRef *hw_frames_ref = NULL;
    AVFilterGraph *filter_graph = NULL;
    AVFilterContext* filter_in;
    AVFilterContext* filter_out;

    // kmsgrab pkt
    AVPacket *input_pkt = av_packet_alloc();
    if (!input_pkt) {
        fprintf(stderr, "av_packet_alloc failed\n");
        exit(1);
    }
    // decoded frame
    AVFrame *decoded_frame = av_frame_alloc();
    if (!decoded_frame) {
        fprintf(stderr, "av_frame_alloc failed\n");
        exit(1);
    }
    // filtered frame
    AVFrame *filtered_frame = av_frame_alloc();
    if (!filtered_frame) {
        fprintf(stderr, "av_frame_alloc failed\n");
        exit(1);
    }
    // encoded pkt
    AVPacket *encoded_pkt = av_packet_alloc();
    if (!encoded_pkt) {
        fprintf(stderr, "av_packet_alloc failed\n");
        exit(1);
    }

    while (running--) {

        // get input pkt

        printf("<");
        fflush(stdout);

        r = av_read_frame(ifctx, input_pkt);
        if (r < 0) {
            if (r == AVERROR(EAGAIN)) {
                continue;
            }

            fprintf(stderr, "av_read_frame failed: %d\n", r);
            exit(1);
        }

        auto &pc = bench::detail::performanceCounters();
        pc.beginMeasure();
        bench::Clock::time_point const before = bench::Clock::now();

        // decode

        r = avcodec_send_packet(decoder, input_pkt);
        if (r < 0) {
            fprintf(stderr, "avcodec_send_packet failed: %d\n", r);
            exit(1);
        }

        while (1) {
            r = avcodec_receive_frame(decoder, decoded_frame);
            if (r < 0) {
                if (r != AVERROR(EAGAIN)) {
                    fprintf(stderr, "avcodec_receive_frame failed: %d\n", r);
                    exit(1);
                }

                break;
            }

            decoded_frame->pts = pts++;

            // filter graph init & encoder finalize & output finalize

            if (!filter_graph) {
                if (decoded_frame->hw_frames_ctx) {
                    hw_frames_ref = av_buffer_ref(decoded_frame->hw_frames_ctx);
                    if (!hw_frames_ref) {
                        fprintf(stderr, "av_buffer_ref failed\n");
                        exit(1);
                    }
                }

                filter_graph = avfilter_graph_alloc();
                if (!filter_graph) {
                    fprintf(stderr, "avfilter_graph_alloc failed\n");
                    exit(1);
                }

                // buffersrc

                r = avfilter_graph_create_filter(&filter_in,
                    avfilter_get_by_name("buffer"), "in",
                    "width=1:height=1:pix_fmt=drm_prime:time_base=1/1", NULL,
                    filter_graph);
                if (r < 0) {
                    fprintf(stderr, "avfilter_graph_create_filter failed: %d\n", r);
                    exit(1);
                }

                AVBufferSrcParameters *params = av_buffersrc_parameters_alloc();
                if (!params) {
                    fprintf(stderr, "av_buffersrc_parameters_alloc failed\n");
                    exit(1);
                }

                params->format = decoded_frame->format;
                params->width = decoded_frame->width;
                params->height = decoded_frame->height;
                params->hw_frames_ctx = hw_frames_ref;

                r = av_buffersrc_parameters_set(filter_in, params);
                if (r < 0) {
                    fprintf(stderr, "av_buffersrc_parameters_set failed: %d\n", r);
                    exit(1);
                }

                av_free(params);

                // buffersink

                r = avfilter_graph_create_filter(&filter_out,
                    avfilter_get_by_name("buffersink"), "out", NULL,
                    NULL, filter_graph);
                if (r < 0) {
                    fprintf(stderr, "avfilter_graph_create_filter failed: %d\n", r);
                    exit(1);
                }

                // filter config

                AVFilterInOut *inputs = avfilter_inout_alloc();
                if (!inputs) {
                    fprintf(stderr, "avfilter_inout_alloc failed\n");
                    exit(1);
                }

                inputs->name = av_strdup("in");
                inputs->filter_ctx = filter_in;
                inputs->pad_idx = 0;
                inputs->next = NULL;

                AVFilterInOut *outputs = avfilter_inout_alloc();
                if (!outputs) {
                    fprintf(stderr, "avfilter_inout_alloc failed\n");
                    exit(1);
                }

                outputs->name = av_strdup("out");
                outputs->filter_ctx = filter_out;
                outputs->pad_idx = 0;
                outputs->next = NULL;

                r = avfilter_graph_parse(filter_graph,
                    "hwmap=mode=direct:derive_device=vaapi"
                    ",scale_vaapi=format=nv12:mode=fast",
                    outputs, inputs, NULL);
                if (r < 0) {
                    fprintf(stderr, "avfilter_graph_parse failed: %d\n", r);
                    exit(1);
                }

                r = avfilter_graph_config(filter_graph, NULL);
                if (r < 0) {
                    fprintf(stderr, "avfilter_graph_config failed: %d\n", r);
                    exit(1);
                }

                // encoder finalize

                encoder->time_base = (AVRational){1, 60};
                encoder->framerate = (AVRational){60, 1};

                encoder->width = av_buffersink_get_w(filter_out);
                encoder->height = av_buffersink_get_h(filter_out);
                encoder->pix_fmt = (AVPixelFormat)av_buffersink_get_format(filter_out);
                encoder->hw_frames_ctx = av_buffer_ref(av_buffersink_get_hw_frames_ctx(filter_out));

                AVDictionary *encoder_opts = NULL;
                av_dict_set_int(&encoder_opts, "profile", FF_PROFILE_H264_CONSTRAINED_BASELINE, 0);
                av_dict_set_int(&encoder_opts, "g", 60, 0);
                // 920 does not support B-frames
                av_dict_set_int(&encoder_opts, "bf", 0, 0);
                av_dict_set_int(&encoder_opts, "async_depth", 11, 0);

                r = avcodec_open2(encoder, enc, &encoder_opts);
                if (r < 0) {
                    fprintf(stderr, "avcodec_open2 failed: %d\n", r);
                    exit(1);
                }

                av_dict_free(&encoder_opts);

                r = avcodec_parameters_from_context(ost->codecpar, encoder);
                if (r < 0) {
                    fprintf(stderr, "avcodec_parameters_from_context failed: %d\n", r);
                    exit(1);
                }

                ost->time_base = av_add_q(encoder->time_base, (AVRational){0, 1});

                // output finalize

                r = avformat_write_header(ofctx, NULL);
                if (r < 0) {
                    fprintf(stderr, "avformat_write_header failed: %d\n", r);
                    exit(1);
                }
            }

            // filter in

            r = av_buffersrc_add_frame_flags(filter_in, decoded_frame, 0);
            if (r < 0) {
                fprintf(stderr, "av_buffersrc_add_frame_flags failed: %d\n", r);
                exit(1);
            }

            // filter out

            r = av_buffersink_get_frame(filter_out, filtered_frame);
            if (r < 0) {
                fprintf(stderr, "av_buffersink_get_frame failed: %d\n", r);
                exit(1);
            }

            // encode

            filtered_frame->pts = pts;

            r = avcodec_send_frame(encoder, filtered_frame);
            if (r < 0) {
                fprintf(stderr, "avcodec_send_frame failed: %d\n", r);
                exit(1);
            }

            while (1) {
                r = avcodec_receive_packet(encoder, encoded_pkt);
                if (r < 0) {
                    if (r != AVERROR(EAGAIN)) {
                        fprintf(stderr, "avcodec_receive_packet failed: %d", r);
                        exit(1);
                    }

                    // if frame has been issued to vaapi by avcodec_send_frame then
                    // avcodec_receive_packet also returns EAGAIN

                    printf(".");
                    fflush(stdout);

                    break;
                }

                bench::Clock::time_point const after = bench::Clock::now();
                pc.endMeasure();
                pc.updateResults(epoch.numIters());
                // complete 1 epoch with 1 iteration, print result when all epochs completed
                epoch.add(after - before, pc);

                printf("-");
                fflush(stdout);

                // output
                r = av_interleaved_write_frame(ofctx, encoded_pkt);
                if (r < 0) {
                    fprintf(stderr, "av_interleaved_write_frame failed: %d", r);
                    exit(1);
                }

                av_packet_unref(encoded_pkt);
            } // encode -> output

            av_frame_unref(filtered_frame);
            av_frame_unref(decoded_frame);
        } // decode

        av_packet_unref(input_pkt);

        printf(">");
        fflush(stdout);
    } // while (running)

    printf("\n");

    return 0;
}
