//
// Created by runsisi on 23-4-4.
//

#ifndef __KMSGRAB_H__
#define __KMSGRAB_H__

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/hwcontext.h>
#include <libavutil/hwcontext_drm.h>
}

namespace kmsgrab {

typedef struct KMSGrabContext {
  AVBufferRef *device_ref;
  AVHWDeviceContext *device;
  AVDRMDeviceContext *hwctx;
  int fb2_available;

  AVBufferRef *frames_ref;
  AVHWFramesContext *frames;

  uint32_t plane_id;
  uint32_t drm_format;
  unsigned int width;
  unsigned int height;

  int64_t frame_delay;
  int64_t frame_last;

  const char *device_path;
  enum AVPixelFormat format;
  int64_t drm_format_modifier;
  int64_t source_plane;
  int64_t source_crtc;
  AVRational framerate;
} KMSGrabContext;

int kmsgrab_read_frame(KMSGrabContext *ctx, AVFrame *frame);
int kmsgrab_read_header(KMSGrabContext *ctx);
int kmsgrab_read_close(KMSGrabContext *ctx);

} // namespace kmsgrab

#endif // __KMSGRAB_H__
