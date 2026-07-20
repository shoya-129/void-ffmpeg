#include <void/void.hpp>
#include <map>
#include <string>
#include <sstream>
#include <vector>
#include <stdexcept>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdarg.h>

extern "C" {
// Declare JS imports
__attribute__((import_module("env"))) int host_open(const char* pathname, int flags, int mode);
__attribute__((import_module("env"))) int host_close(int fd);
__attribute__((import_module("env"))) int host_read(int fd, void* buf, int count);
__attribute__((import_module("env"))) int host_write(int fd, const void* buf, int count);
__attribute__((import_module("env"))) long long host_lseek(int fd, long long offset, int whence);
__attribute__((import_module("env"))) int host_fstat(int fd, struct stat* buf);
__attribute__((import_module("env"))) int host_pread(int fd, void* buf, int count, long long offset);
__attribute__((import_module("env"))) int host_pwrite(int fd, const void* buf, int count, long long offset);
__attribute__((import_module("env"))) int host_stat(const char* pathname, struct stat* buf);

// Override POSIX functions
int open(const char* pathname, int flags, ...) {
    mode_t mode = 0;
    if (flags & O_CREAT) {
        va_list args;
        va_start(args, flags);
        mode = va_arg(args, mode_t);
        va_end(args);
    }
    return host_open(pathname, flags, mode);
}

int close(int fd) {
    return host_close(fd);
}

ssize_t read(int fd, void* buf, size_t count) {
    return host_read(fd, buf, count);
}

ssize_t write(int fd, const void* buf, size_t count) {
    return host_write(fd, buf, count);
}

off_t lseek(int fd, off_t offset, int whence) {
    return (off_t)host_lseek(fd, offset, whence);
}

ssize_t pread(int fd, void *buf, size_t count, off_t offset) {
    return host_pread(fd, buf, count, offset);
}

ssize_t pwrite(int fd, const void *buf, size_t count, off_t offset) {
    return host_pwrite(fd, buf, count, offset);
}

int fstat(int fd, struct stat* buf) {
    return host_fstat(fd, buf);
}

int stat(const char* pathname, struct stat* buf) {
    return host_stat(pathname, buf);
}
}

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
}

using json = nlohmann::json;

struct VideoState {
    std::string path;
    int width = 0;
    int height = 0;
    double duration = 0.0;
    
    // Resize settings
    bool has_resize = false;
    int resize_w = 0;
    int resize_h = 0;
    
    // Crop settings
    bool has_crop = false;
    int crop_x = 0;
    int crop_y = 0;
    int crop_w = 0;
    int crop_h = 0;
    
    // Trim settings
    bool has_trim = false;
    double trim_start = 0.0;
    double trim_end = 0.0;
    
    // Target format target extension (e.g. mp4, mkv)
    std::string target_format = "";
};

static std::map<std::string, VideoState> g_videos;
static int g_next_handle_id = 1;

std::string generate_handle() {
    std::stringstream ss;
    ss << "video_" << g_next_handle_id++;
    return ss.str();
}

// 1. open(path)
json plugin_open(const void_sdk::ArgsMap& args) {
    std::string path = void_sdk::get_string(args, "path");
    
    AVFormatContext* format_ctx = nullptr;
    if (avformat_open_input(&format_ctx, path.c_str(), nullptr, nullptr) < 0) {
        throw std::runtime_error("Could not open input file: " + path);
    }
    if (avformat_find_stream_info(format_ctx, nullptr) < 0) {
        avformat_close_input(&format_ctx);
        throw std::runtime_error("Could not find stream information for: " + path);
    }
    
    int video_stream_idx = -1;
    AVCodecParameters* codec_par = nullptr;
    for (unsigned int i = 0; i < format_ctx->nb_streams; i++) {
        if (format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_idx = i;
            codec_par = format_ctx->streams[i]->codecpar;
            break;
        }
    }
    
    if (video_stream_idx == -1) {
        avformat_close_input(&format_ctx);
        throw std::runtime_error("No video stream found in: " + path);
    }
    
    int width = codec_par->width;
    int height = codec_par->height;
    double duration = 0.0;
    if (format_ctx->duration != AV_NOPTS_VALUE) {
        duration = (double)format_ctx->duration / AV_TIME_BASE;
    }
    
    avformat_close_input(&format_ctx);
    
    std::string handle = generate_handle();
    VideoState state;
    state.path = path;
    state.width = width;
    state.height = height;
    state.duration = duration;
    
    g_videos[handle] = state;
    
    return json{
        {"handle", handle},
        {"width", width},
        {"height", height},
        {"duration", duration}
    };
}

// 2. resize(handle, width, height)
json resize(const void_sdk::ArgsMap& args) {
    std::string handle = void_sdk::get_string(args, "handle");
    int width = (int)void_sdk::get_int(args, "width");
    int height = (int)void_sdk::get_int(args, "height");
    
    auto it = g_videos.find(handle);
    if (it == g_videos.end()) {
        throw std::runtime_error("Invalid video handle: " + handle);
    }
    
    it->second.has_resize = true;
    it->second.resize_w = width;
    it->second.resize_h = height;
    
    return json{{"success", true}};
}

// 3. trim(handle, start, end)
json trim(const void_sdk::ArgsMap& args) {
    std::string handle = void_sdk::get_string(args, "handle");
    double start = void_sdk::get_float(args, "start");
    double end = void_sdk::get_float(args, "end");
    
    auto it = g_videos.find(handle);
    if (it == g_videos.end()) {
        throw std::runtime_error("Invalid video handle: " + handle);
    }
    
    it->second.has_trim = true;
    it->second.trim_start = start;
    it->second.trim_end = end;
    
    return json{{"success", true}};
}

// 4. crop(handle, x, y, width, height)
json crop(const void_sdk::ArgsMap& args) {
    std::string handle = void_sdk::get_string(args, "handle");
    int x = (int)void_sdk::get_int(args, "x");
    int y = (int)void_sdk::get_int(args, "y");
    int width = (int)void_sdk::get_int(args, "width");
    int height = (int)void_sdk::get_int(args, "height");
    
    auto it = g_videos.find(handle);
    if (it == g_videos.end()) {
        throw std::runtime_error("Invalid video handle: " + handle);
    }
    
    it->second.has_crop = true;
    it->second.crop_x = x;
    it->second.crop_y = y;
    it->second.crop_w = width;
    it->second.crop_h = height;
    
    return json{{"success", true}};
}

// 5. extractAudio(handle, outputPath)
json extractAudio(const void_sdk::ArgsMap& args) {
    std::string input_path;
    if (args.count("handle") > 0) {
        std::string handle = void_sdk::get_string(args, "handle");
        auto it = g_videos.find(handle);
        if (it == g_videos.end()) {
            throw std::runtime_error("Invalid video handle: " + handle);
        }
        input_path = it->second.path;
    } else if (args.count("input") > 0) {
        input_path = void_sdk::get_string(args, "input");
    } else {
        throw std::runtime_error("missing required field 'handle' or 'input'");
    }
    
    std::string output_path;
    if (args.count("outputPath") > 0) {
        output_path = void_sdk::get_string(args, "outputPath");
    } else if (args.count("output") > 0) {
        output_path = void_sdk::get_string(args, "output");
    } else {
        throw std::runtime_error("missing required field 'outputPath' or 'output'");
    }
    
    AVFormatContext* in_fmt_ctx = nullptr;
    AVFormatContext* out_fmt_ctx = nullptr;
    
    int err = avformat_open_input(&in_fmt_ctx, input_path.c_str(), nullptr, nullptr);
    if (err < 0) {
        char err_buf[256];
        av_strerror(err, err_buf, sizeof(err_buf));
        throw std::runtime_error("Could not open input file: " + input_path + " (FFmpeg error: " + std::string(err_buf) + ")");
    }
    if (avformat_find_stream_info(in_fmt_ctx, nullptr) < 0) {
        avformat_close_input(&in_fmt_ctx);
        throw std::runtime_error("Could not find stream info for: " + input_path);
    }
    
    int audio_stream_idx = -1;
    for (unsigned int i = 0; i < in_fmt_ctx->nb_streams; i++) {
        if (in_fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_idx = i;
            break;
        }
    }
    if (audio_stream_idx == -1) {
        avformat_close_input(&in_fmt_ctx);
        throw std::runtime_error("No audio stream found in: " + input_path);
    }
    
    AVStream* in_stream = in_fmt_ctx->streams[audio_stream_idx];
    const char* format_name = nullptr;
    if (in_stream->codecpar->codec_id == AV_CODEC_ID_AAC) {
        format_name = "mp4";
    }
    
    avformat_alloc_output_context2(&out_fmt_ctx, nullptr, format_name, output_path.c_str());
    if (!out_fmt_ctx) {
        avformat_close_input(&in_fmt_ctx);
        throw std::runtime_error("Could not create output context for: " + output_path);
    }
    
    AVStream* out_stream = avformat_new_stream(out_fmt_ctx, nullptr);
    if (!out_stream) {
        avformat_close_input(&in_fmt_ctx);
        avformat_free_context(out_fmt_ctx);
        throw std::runtime_error("Failed allocating output stream");
    }
    
    avcodec_parameters_copy(out_stream->codecpar, in_stream->codecpar);
    out_stream->codecpar->codec_tag = 0;
    
    if (!(out_fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&out_fmt_ctx->pb, output_path.c_str(), AVIO_FLAG_WRITE) < 0) {
            avformat_close_input(&in_fmt_ctx);
            avformat_free_context(out_fmt_ctx);
            throw std::runtime_error("Could not open output file: " + output_path);
        }
    }
    
    if (avformat_write_header(out_fmt_ctx, nullptr) < 0) {
        avformat_close_input(&in_fmt_ctx);
        if (out_fmt_ctx->pb) avio_closep(&out_fmt_ctx->pb);
        avformat_free_context(out_fmt_ctx);
        throw std::runtime_error("Error writing header");
    }
    
    AVPacket* pkt = av_packet_alloc();
    while (av_read_frame(in_fmt_ctx, pkt) >= 0) {
        if (pkt->stream_index == audio_stream_idx) {
            pkt->pts = av_rescale_q_rnd(pkt->pts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
            pkt->dts = av_rescale_q_rnd(pkt->dts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
            pkt->duration = av_rescale_q(pkt->duration, in_stream->time_base, out_stream->time_base);
            pkt->pos = -1;
            pkt->stream_index = 0;
            av_interleaved_write_frame(out_fmt_ctx, pkt);
        }
        av_packet_unref(pkt);
    }
    
    av_write_trailer(out_fmt_ctx);
    
    av_packet_free(&pkt);
    avformat_close_input(&in_fmt_ctx);
    if (out_fmt_ctx && !(out_fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        avio_closep(&out_fmt_ctx->pb);
    }
    avformat_free_context(out_fmt_ctx);
    
    return json{{"success", true}};
}

// 6. merge(handle, otherPath)
json merge(const void_sdk::ArgsMap& args) {
    std::string handle = void_sdk::get_string(args, "handle");
    std::string other_path = void_sdk::get_string(args, "otherPath");
    
    auto it = g_videos.find(handle);
    if (it == g_videos.end()) {
        throw std::runtime_error("Invalid video handle: " + handle);
    }
    
    it->second.target_format = "merge"; // flag that we are doing merge
    it->second.has_resize = false; // reset crop/resize/trim for merge
    it->second.has_crop = false;
    it->second.has_trim = false;
    it->second.target_format = "merge:" + other_path;
    
    return json{{"success", true}};
}

// 7. convert(handle, format)
json convert(const void_sdk::ArgsMap& args) {
    std::string handle = void_sdk::get_string(args, "handle");
    std::string format = void_sdk::get_string(args, "format");
    
    auto it = g_videos.find(handle);
    if (it == g_videos.end()) {
        throw std::runtime_error("Invalid video handle: " + handle);
    }
    
    it->second.target_format = format;
    return json{{"success", true}};
}

// 8. thumbnail(handle, time, outputPath)
json thumbnail(const void_sdk::ArgsMap& args) {
    std::string handle = void_sdk::get_string(args, "handle");
    double time_sec = void_sdk::get_float(args, "time");
    std::string output_path = void_sdk::get_string(args, "outputPath");
    
    auto it = g_videos.find(handle);
    if (it == g_videos.end()) {
        throw std::runtime_error("Invalid video handle: " + handle);
    }
    
    AVFormatContext* format_ctx = nullptr;
    if (avformat_open_input(&format_ctx, it->second.path.c_str(), nullptr, nullptr) < 0) {
        throw std::runtime_error("Could not open input file: " + it->second.path);
    }
    if (avformat_find_stream_info(format_ctx, nullptr) < 0) {
        avformat_close_input(&format_ctx);
        throw std::runtime_error("Could not find stream info");
    }
    
    int video_stream_idx = -1;
    for (unsigned int i = 0; i < format_ctx->nb_streams; i++) {
        if (format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_idx = i;
            break;
        }
    }
    if (video_stream_idx == -1) {
        avformat_close_input(&format_ctx);
        throw std::runtime_error("No video stream found");
    }
    
    AVStream* stream = format_ctx->streams[video_stream_idx];
    const AVCodec* decoder = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!decoder) {
        avformat_close_input(&format_ctx);
        throw std::runtime_error("Decoder not found");
    }
    AVCodecContext* dec_ctx = avcodec_alloc_context3(decoder);
    avcodec_parameters_to_context(dec_ctx, stream->codecpar);
    if (avcodec_open2(dec_ctx, decoder, nullptr) < 0) {
        avcodec_free_context(&dec_ctx);
        avformat_close_input(&format_ctx);
        throw std::runtime_error("Could not open decoder");
    }
    
    // Seek to frame
    int64_t seek_time = time_sec * AV_TIME_BASE;
    av_seek_frame(format_ctx, -1, seek_time, AVSEEK_FLAG_BACKWARD);
    avcodec_flush_buffers(dec_ctx);
    
    AVFrame* frame = av_frame_alloc();
    AVPacket* pkt = av_packet_alloc();
    bool frame_found = false;
    
    while (av_read_frame(format_ctx, pkt) >= 0) {
        if (pkt->stream_index == video_stream_idx) {
            if (avcodec_send_packet(dec_ctx, pkt) >= 0) {
                if (avcodec_receive_frame(dec_ctx, frame) >= 0) {
                    frame_found = true;
                    break;
                }
            }
        }
        av_packet_unref(pkt);
    }
    
    if (!frame_found) {
        av_packet_free(&pkt);
        av_frame_free(&frame);
        avcodec_free_context(&dec_ctx);
        avformat_close_input(&format_ctx);
        throw std::runtime_error("Could not decode frame at target time");
    }
    
    // Encode as JPEG using the native mjpeg encoder
    const AVCodec* encoder = avcodec_find_encoder(AV_CODEC_ID_MJPEG);
    if (!encoder) {
        av_packet_free(&pkt);
        av_frame_free(&frame);
        avcodec_free_context(&dec_ctx);
        avformat_close_input(&format_ctx);
        throw std::runtime_error("MJPEG encoder not found");
    }
    
    AVCodecContext* enc_ctx = avcodec_alloc_context3(encoder);
    enc_ctx->width = frame->width;
    enc_ctx->height = frame->height;
    enc_ctx->pix_fmt = AV_PIX_FMT_YUVJ420P;
    enc_ctx->time_base = {1, 25};
    
    if (avcodec_open2(enc_ctx, encoder, nullptr) < 0) {
        avcodec_free_context(&enc_ctx);
        av_packet_free(&pkt);
        av_frame_free(&frame);
        avcodec_free_context(&dec_ctx);
        avformat_close_input(&format_ctx);
        throw std::runtime_error("Could not open MJPEG encoder");
    }
    
    AVFrame* yuv_frame = av_frame_alloc();
    yuv_frame->width = frame->width;
    yuv_frame->height = frame->height;
    yuv_frame->format = enc_ctx->pix_fmt;
    av_frame_get_buffer(yuv_frame, 0);
    
    SwsContext* sws_ctx = sws_getContext(
        frame->width, frame->height, (AVPixelFormat)frame->format,
        frame->width, frame->height, enc_ctx->pix_fmt,
        SWS_BICUBIC, nullptr, nullptr, nullptr
    );
    sws_scale(sws_ctx, frame->data, frame->linesize, 0, frame->height, yuv_frame->data, yuv_frame->linesize);
    sws_freeContext(sws_ctx);
    
    AVPacket* enc_pkt = av_packet_alloc();
    if (avcodec_send_frame(enc_ctx, yuv_frame) >= 0) {
        if (avcodec_receive_packet(enc_ctx, enc_pkt) >= 0) {
            int fd = open(output_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
            if (fd >= 0) {
                write(fd, enc_pkt->data, enc_pkt->size);
                close(fd);
            }
        }
    }
    
    av_packet_free(&enc_pkt);
    av_frame_free(&yuv_frame);
    avcodec_free_context(&enc_ctx);
    av_packet_free(&pkt);
    av_frame_free(&frame);
    avcodec_free_context(&dec_ctx);
    avformat_close_input(&format_ctx);
    
    return json{{"success", true}};
}

// Helper: direct concatenate
void merge_impl(const std::string& first_path, const std::string& second_path, const std::string& output_path) {
    std::vector<std::string> inputs = {first_path, second_path};
    AVFormatContext* out_fmt_ctx = nullptr;
    avformat_alloc_output_context2(&out_fmt_ctx, nullptr, nullptr, output_path.c_str());
    if (!out_fmt_ctx) throw std::runtime_error("Could not allocate output context");
    
    std::vector<AVFormatContext*> in_fmt_ctxs;
    
    for (int i = 0; i < 2; i++) {
        AVFormatContext* in_ctx = nullptr;
        if (avformat_open_input(&in_ctx, inputs[i].c_str(), nullptr, nullptr) < 0) {
            for (auto* c : in_fmt_ctxs) avformat_close_input(&c);
            avformat_free_context(out_fmt_ctx);
            throw std::runtime_error("Could not open input: " + inputs[i]);
        }
        if (avformat_find_stream_info(in_ctx, nullptr) < 0) {
            avformat_close_input(&in_ctx);
            for (auto* c : in_fmt_ctxs) avformat_close_input(&c);
            avformat_free_context(out_fmt_ctx);
            throw std::runtime_error("Could not find stream info for: " + inputs[i]);
        }
        in_fmt_ctxs.push_back(in_ctx);
    }
    
    int out_video_stream_idx = -1;
    int out_audio_stream_idx = -1;
    
    for (unsigned int i = 0; i < in_fmt_ctxs[0]->nb_streams; i++) {
        auto* in_stream = in_fmt_ctxs[0]->streams[i];
        if (in_stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && out_video_stream_idx == -1) {
            auto* out_stream = avformat_new_stream(out_fmt_ctx, nullptr);
            avcodec_parameters_copy(out_stream->codecpar, in_stream->codecpar);
            out_stream->codecpar->codec_tag = 0;
            out_video_stream_idx = out_stream->index;
        } else if (in_stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && out_audio_stream_idx == -1) {
            auto* out_stream = avformat_new_stream(out_fmt_ctx, nullptr);
            avcodec_parameters_copy(out_stream->codecpar, in_stream->codecpar);
            out_stream->codecpar->codec_tag = 0;
            out_audio_stream_idx = out_stream->index;
        }
    }
    
    if (out_video_stream_idx == -1) {
        for (auto* c : in_fmt_ctxs) avformat_close_input(&c);
        avformat_free_context(out_fmt_ctx);
        throw std::runtime_error("No video stream found in first file");
    }
    
    if (!(out_fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&out_fmt_ctx->pb, output_path.c_str(), AVIO_FLAG_WRITE) < 0) {
            for (auto* c : in_fmt_ctxs) avformat_close_input(&c);
            avformat_free_context(out_fmt_ctx);
            throw std::runtime_error("Could not open output file: " + output_path);
        }
    }
    
    if (avformat_write_header(out_fmt_ctx, nullptr) < 0) {
        for (auto* c : in_fmt_ctxs) avformat_close_input(&c);
        if (out_fmt_ctx->pb) avio_closep(&out_fmt_ctx->pb);
        avformat_free_context(out_fmt_ctx);
        throw std::runtime_error("Could not write header");
    }
    
    int64_t time_offset_video = 0;
    int64_t time_offset_audio = 0;
    
    AVPacket* pkt = av_packet_alloc();
    
    for (int i = 0; i < 2; i++) {
        auto* in_ctx = in_fmt_ctxs[i];
        
        int local_video_idx = -1;
        int local_audio_idx = -1;
        for (unsigned int j = 0; j < in_ctx->nb_streams; j++) {
            if (in_ctx->streams[j]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                local_video_idx = j;
            } else if (in_ctx->streams[j]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
                local_audio_idx = j;
            }
        }
        
        int64_t start_pts_video = -1;
        int64_t start_pts_audio = -1;
        
        int64_t max_pts_video = 0;
        int64_t max_pts_audio = 0;
        
        while (av_read_frame(in_ctx, pkt) >= 0) {
            if (pkt->stream_index == local_video_idx && out_video_stream_idx != -1) {
                auto* in_stream = in_ctx->streams[local_video_idx];
                auto* out_stream = out_fmt_ctx->streams[out_video_stream_idx];
                
                int64_t raw_pts = av_rescale_q_rnd(pkt->pts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
                int64_t raw_dts = av_rescale_q_rnd(pkt->dts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
                
                if (start_pts_video == -1 && pkt->pts != AV_NOPTS_VALUE) {
                    start_pts_video = raw_dts != AV_NOPTS_VALUE ? raw_dts : raw_pts;
                }
                
                if (pkt->pts != AV_NOPTS_VALUE && start_pts_video != -1) {
                    pkt->pts = raw_pts - start_pts_video + time_offset_video;
                }
                if (pkt->dts != AV_NOPTS_VALUE && start_pts_video != -1) {
                    pkt->dts = raw_dts - start_pts_video + time_offset_video;
                }
                
                pkt->duration = av_rescale_q(pkt->duration, in_stream->time_base, out_stream->time_base);
                pkt->stream_index = out_video_stream_idx;
                
                int64_t out_val = pkt->pts;
                if (pkt->dts != AV_NOPTS_VALUE && pkt->dts > out_val) out_val = pkt->dts;
                if (out_val > max_pts_video) max_pts_video = out_val;
                
                av_interleaved_write_frame(out_fmt_ctx, pkt);
            } else if (pkt->stream_index == local_audio_idx && out_audio_stream_idx != -1) {
                auto* in_stream = in_ctx->streams[local_audio_idx];
                auto* out_stream = out_fmt_ctx->streams[out_audio_stream_idx];
                
                int64_t raw_pts = av_rescale_q_rnd(pkt->pts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
                int64_t raw_dts = av_rescale_q_rnd(pkt->dts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
                
                if (start_pts_audio == -1 && pkt->pts != AV_NOPTS_VALUE) {
                    start_pts_audio = raw_dts != AV_NOPTS_VALUE ? raw_dts : raw_pts;
                }
                
                if (pkt->pts != AV_NOPTS_VALUE && start_pts_audio != -1) {
                    pkt->pts = raw_pts - start_pts_audio + time_offset_audio;
                }
                if (pkt->dts != AV_NOPTS_VALUE && start_pts_audio != -1) {
                    pkt->dts = raw_dts - start_pts_audio + time_offset_audio;
                }
                
                pkt->duration = av_rescale_q(pkt->duration, in_stream->time_base, out_stream->time_base);
                pkt->stream_index = out_audio_stream_idx;
                
                int64_t out_val = pkt->pts;
                if (pkt->dts != AV_NOPTS_VALUE && pkt->dts > out_val) out_val = pkt->dts;
                if (out_val > max_pts_audio) max_pts_audio = out_val;
                
                av_interleaved_write_frame(out_fmt_ctx, pkt);
            }
            av_packet_unref(pkt);
        }
        
        time_offset_video = max_pts_video + 1;
        time_offset_audio = max_pts_audio + 1;
    }
    
    av_write_trailer(out_fmt_ctx);
    
    av_packet_free(&pkt);
    for (auto* c : in_fmt_ctxs) avformat_close_input(&c);
    if (out_fmt_ctx && !(out_fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        avio_closep(&out_fmt_ctx->pb);
    }
    avformat_free_context(out_fmt_ctx);
}

// 9. save(handle, outputPath)
json save(const void_sdk::ArgsMap& args) {
    std::string handle = void_sdk::get_string(args, "handle");
    std::string output_path = void_sdk::get_string(args, "outputPath");
    
    auto it = g_videos.find(handle);
    if (it == g_videos.end()) {
        throw std::runtime_error("Invalid video handle: " + handle);
    }
    
    const VideoState& state = it->second;
    
    // Check if it's a merge operation
    if (state.target_format.rfind("merge:", 0) == 0) {
        std::string other_path = state.target_format.substr(6);
        merge_impl(state.path, other_path, output_path);
        g_videos.erase(it);
        return json{{"success", true}};
    }
    
    // Otherwise, perform the full crop/resize/trim transcode pipeline
    AVFormatContext* in_fmt_ctx = nullptr;
    if (avformat_open_input(&in_fmt_ctx, state.path.c_str(), nullptr, nullptr) < 0) {
        throw std::runtime_error("Could not open input file: " + state.path);
    }
    if (avformat_find_stream_info(in_fmt_ctx, nullptr) < 0) {
        avformat_close_input(&in_fmt_ctx);
        throw std::runtime_error("Could not find stream info");
    }
    
    int video_stream_idx = -1;
    int audio_stream_idx = -1;
    for (unsigned int i = 0; i < in_fmt_ctx->nb_streams; i++) {
        if (in_fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && video_stream_idx == -1) {
            video_stream_idx = i;
        } else if (in_fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && audio_stream_idx == -1) {
            audio_stream_idx = i;
        }
    }
    if (video_stream_idx == -1) {
        avformat_close_input(&in_fmt_ctx);
        throw std::runtime_error("No video stream found");
    }
    
    AVStream* in_stream = in_fmt_ctx->streams[video_stream_idx];
    const AVCodec* decoder = avcodec_find_decoder(in_stream->codecpar->codec_id);
    if (!decoder) {
        avformat_close_input(&in_fmt_ctx);
        throw std::runtime_error("Decoder not found");
    }
    AVCodecContext* dec_ctx = avcodec_alloc_context3(decoder);
    avcodec_parameters_to_context(dec_ctx, in_stream->codecpar);
    dec_ctx->pkt_timebase = in_stream->time_base;
    if (avcodec_open2(dec_ctx, decoder, nullptr) < 0) {
        avcodec_free_context(&dec_ctx);
        avformat_close_input(&in_fmt_ctx);
        throw std::runtime_error("Could not open decoder");
    }
    
    AVFormatContext* out_fmt_ctx = nullptr;
    avformat_alloc_output_context2(&out_fmt_ctx, nullptr, nullptr, output_path.c_str());
    if (!out_fmt_ctx) {
        avcodec_free_context(&dec_ctx);
        avformat_close_input(&in_fmt_ctx);
        throw std::runtime_error("Could not allocate output context");
    }
    
    int out_width = state.has_resize ? state.resize_w : (state.has_crop ? state.crop_w : dec_ctx->width);
    int out_height = state.has_resize ? state.resize_h : (state.has_crop ? state.crop_h : dec_ctx->height);
    
    const AVCodec* encoder = avcodec_find_encoder(AV_CODEC_ID_MPEG4);
    if (!encoder) {
        encoder = avcodec_find_encoder(AV_CODEC_ID_MJPEG);
    }
    if (!encoder) {
        avcodec_free_context(&dec_ctx);
        avformat_close_input(&in_fmt_ctx);
        avformat_free_context(out_fmt_ctx);
        throw std::runtime_error("No video encoder found");
    }
    
    AVStream* out_stream = avformat_new_stream(out_fmt_ctx, nullptr);
    AVCodecContext* enc_ctx = avcodec_alloc_context3(encoder);
    enc_ctx->width = out_width;
    enc_ctx->height = out_height;
    enc_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    enc_ctx->gop_size = 12;
    enc_ctx->max_b_frames = 0;
    
    enc_ctx->framerate = av_guess_frame_rate(in_fmt_ctx, in_stream, nullptr);
    if (enc_ctx->framerate.num == 0 || enc_ctx->framerate.den == 0) {
        enc_ctx->framerate = {25, 1};
    }
    enc_ctx->time_base = av_inv_q(enc_ctx->framerate);
    enc_ctx->bit_rate = 2000000;
    
    if (out_fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER) {
        enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }
    
    enc_ctx->strict_std_compliance = FF_COMPLIANCE_UNOFFICIAL;
    if (avcodec_open2(enc_ctx, encoder, nullptr) < 0) {
        avcodec_free_context(&enc_ctx);
        avcodec_free_context(&dec_ctx);
        avformat_close_input(&in_fmt_ctx);
        avformat_free_context(out_fmt_ctx);
        throw std::runtime_error("Could not open encoder");
    }
    avcodec_parameters_from_context(out_stream->codecpar, enc_ctx);
    out_stream->codecpar->codec_tag = 0;
    out_stream->time_base = enc_ctx->time_base;
    out_stream->avg_frame_rate = enc_ctx->framerate;
    out_stream->r_frame_rate = enc_ctx->framerate;
    
    // Setup audio stream in output if input has audio
    int out_audio_stream_idx = -1;
    if (audio_stream_idx != -1) {
        AVStream* in_audio = in_fmt_ctx->streams[audio_stream_idx];
        AVStream* out_audio = avformat_new_stream(out_fmt_ctx, nullptr);
        if (out_audio) {
            avcodec_parameters_copy(out_audio->codecpar, in_audio->codecpar);
            out_audio->codecpar->codec_tag = 0;
            out_audio_stream_idx = out_audio->index;
            out_audio->time_base = in_audio->time_base;
        }
    }
    
    if (!(out_fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&out_fmt_ctx->pb, output_path.c_str(), AVIO_FLAG_WRITE) < 0) {
            avcodec_free_context(&enc_ctx);
            avcodec_free_context(&dec_ctx);
            avformat_close_input(&in_fmt_ctx);
            avformat_free_context(out_fmt_ctx);
            throw std::runtime_error("Could not open output file: " + output_path);
        }
    }
    
    if (avformat_write_header(out_fmt_ctx, nullptr) < 0) {
        avcodec_free_context(&enc_ctx);
        avcodec_free_context(&dec_ctx);
        avformat_close_input(&in_fmt_ctx);
        if (out_fmt_ctx->pb) avio_closep(&out_fmt_ctx->pb);
        avformat_free_context(out_fmt_ctx);
        throw std::runtime_error("Could not write header");
    }
    
    AVFrame* frame = av_frame_alloc();
    AVFrame* out_frame = av_frame_alloc();
    out_frame->width = out_width;
    out_frame->height = out_height;
    out_frame->format = enc_ctx->pix_fmt;
    av_frame_get_buffer(out_frame, 0);
    
    AVPacket* in_pkt = av_packet_alloc();
    AVPacket* out_pkt = av_packet_alloc();
    
    SwsContext* sws_ctx = nullptr;
    int64_t pts_counter = 0;
    
    int64_t start_audio_pts = -1;
    
    if (state.has_trim && state.trim_start > 0.0) {
        int64_t seek_target = state.trim_start * AV_TIME_BASE;
        av_seek_frame(in_fmt_ctx, -1, seek_target, AVSEEK_FLAG_BACKWARD);
        avcodec_flush_buffers(dec_ctx);
    }
    
    auto process_frame = [&]() {
        int64_t pts = frame->best_effort_timestamp;
        if (pts == AV_NOPTS_VALUE) pts = frame->pts;
        if (pts == AV_NOPTS_VALUE) pts = frame->pkt_dts;
        
        if (state.has_trim && pts != AV_NOPTS_VALUE) {
            double frame_time = pts * av_q2d(in_stream->time_base);
            if (frame_time < state.trim_start || frame_time > state.trim_end) return;
        }
        
        if (state.has_crop) {
            frame->crop_left = state.crop_x;
            frame->crop_top = state.crop_y;
            frame->crop_right = dec_ctx->width - (state.crop_x + state.crop_w);
            frame->crop_bottom = dec_ctx->height - (state.crop_y + state.crop_h);
            av_frame_apply_cropping(frame, 0);
        }
        
        int src_w = frame->width;
        int src_h = frame->height;
        
        if (!sws_ctx) {
            sws_ctx = sws_getContext(
                src_w, src_h, (AVPixelFormat)frame->format,
                out_width, out_height, enc_ctx->pix_fmt,
                SWS_BICUBIC, nullptr, nullptr, nullptr
            );
        }
        
        AVFrame* out_frame = av_frame_alloc();
        out_frame->width = out_width;
        out_frame->height = out_height;
        out_frame->format = enc_ctx->pix_fmt;
        av_frame_get_buffer(out_frame, 32);
        
        sws_scale(sws_ctx, frame->data, frame->linesize, 0, src_h, out_frame->data, out_frame->linesize);
        out_frame->pts = pts_counter++;
        
        if (avcodec_send_frame(enc_ctx, out_frame) >= 0) {
            while (avcodec_receive_packet(enc_ctx, out_pkt) >= 0) {
                out_pkt->pts = av_rescale_q(out_pkt->pts, enc_ctx->time_base, out_stream->time_base);
                out_pkt->dts = av_rescale_q(out_pkt->dts, enc_ctx->time_base, out_stream->time_base);
                out_pkt->duration = av_rescale_q(out_pkt->duration, enc_ctx->time_base, out_stream->time_base);
                out_pkt->stream_index = out_stream->index;
                av_interleaved_write_frame(out_fmt_ctx, out_pkt);
                av_packet_unref(out_pkt);
            }
        }
        av_frame_free(&out_frame);
    };
    
    while (av_read_frame(in_fmt_ctx, in_pkt) >= 0) {
        if (in_pkt->stream_index == video_stream_idx) {
            if (avcodec_send_packet(dec_ctx, in_pkt) >= 0) {
                while (avcodec_receive_frame(dec_ctx, frame) >= 0) {
                    process_frame();
                }
            }
        } else if (in_pkt->stream_index == audio_stream_idx && out_audio_stream_idx != -1) {
            AVStream* in_audio = in_fmt_ctx->streams[audio_stream_idx];
            AVStream* out_audio = out_fmt_ctx->streams[out_audio_stream_idx];
            int64_t audio_pts = in_pkt->pts != AV_NOPTS_VALUE ? in_pkt->pts : in_pkt->dts;
            double pkt_time = (audio_pts != AV_NOPTS_VALUE) ? (audio_pts * av_q2d(in_audio->time_base)) : 0.0;
            
            bool keep = true;
            if (state.has_trim) {
                if (pkt_time < state.trim_start) keep = false;
                if (pkt_time > state.trim_end) keep = false;
            }
            
            if (keep) {
                int64_t raw_pts = av_rescale_q_rnd(in_pkt->pts, in_audio->time_base, out_audio->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
                int64_t raw_dts = av_rescale_q_rnd(in_pkt->dts, in_audio->time_base, out_audio->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
                
                if (start_audio_pts == -1 && in_pkt->pts != AV_NOPTS_VALUE) {
                    start_audio_pts = raw_dts != AV_NOPTS_VALUE ? raw_dts : raw_pts;
                }
                
                if (in_pkt->pts != AV_NOPTS_VALUE && start_audio_pts != -1) {
                    in_pkt->pts = raw_pts - start_audio_pts;
                }
                if (in_pkt->dts != AV_NOPTS_VALUE && start_audio_pts != -1) {
                    in_pkt->dts = raw_dts - start_audio_pts;
                }
                
                in_pkt->duration = av_rescale_q(in_pkt->duration, in_audio->time_base, out_audio->time_base);
                in_pkt->stream_index = out_audio_stream_idx;
                
                av_interleaved_write_frame(out_fmt_ctx, in_pkt);
            }
        }
        av_packet_unref(in_pkt);
    }
    
    // Flush decoder buffer
    avcodec_send_packet(dec_ctx, nullptr);
    while (avcodec_receive_frame(dec_ctx, frame) >= 0) {
        process_frame();
    }
    
    if (avcodec_send_frame(enc_ctx, nullptr) >= 0) {
        while (avcodec_receive_packet(enc_ctx, out_pkt) >= 0) {
            out_pkt->pts = av_rescale_q(out_pkt->pts, enc_ctx->time_base, out_stream->time_base);
            out_pkt->dts = av_rescale_q(out_pkt->dts, enc_ctx->time_base, out_stream->time_base);
            out_pkt->duration = av_rescale_q(out_pkt->duration, enc_ctx->time_base, out_stream->time_base);
            out_pkt->stream_index = out_stream->index;
            av_interleaved_write_frame(out_fmt_ctx, out_pkt);
            av_packet_unref(out_pkt);
        }
    }
    
    av_write_trailer(out_fmt_ctx);
    
    if (sws_ctx) sws_freeContext(sws_ctx);
    av_frame_free(&frame);
    av_frame_free(&out_frame);
    av_packet_free(&in_pkt);
    av_packet_free(&out_pkt);
    avcodec_free_context(&enc_ctx);
    avcodec_free_context(&dec_ctx);
    avformat_close_input(&in_fmt_ctx);
    if (out_fmt_ctx && !(out_fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        avio_closep(&out_fmt_ctx->pb);
    }
    avformat_free_context(out_fmt_ctx);
    
    g_videos.erase(it);
    
    return json{{"success", true}};
}

void init_handlers() {
    void_sdk::register_handler("open", plugin_open);
    void_sdk::register_handler("resize", resize);
    void_sdk::register_handler("trim", trim);
    void_sdk::register_handler("crop", crop);
    void_sdk::register_handler("extractAudio", extractAudio);
    void_sdk::register_handler("merge", merge);
    void_sdk::register_handler("convert", convert);
    void_sdk::register_handler("thumbnail", thumbnail);
    void_sdk::register_handler("save", save);
}

VOID_PLUGIN(init_handlers);
