extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/hwcontext.h>
#include <libavutil/pixfmt.h>
#include <libavutil/pixdesc.h>
}

#include <stdio.h>

int main() {
  AVBufferRef *hwdevice_ref = NULL;
  int r = av_hwdevice_ctx_create(&hwdevice_ref, AV_HWDEVICE_TYPE_VAAPI, "/dev/dri/card0", NULL, 0);
  if (r < 0) {
    fprintf(stderr, "av_hwdevice_ctx_create failed: %d\n", r);
    exit(1);
  }
  AVBufferRef *hwframes_ref = av_hwframe_ctx_alloc(hwdevice_ref);
  if (!hwframes_ref) {
    fprintf(stderr, "failed to alloc hwframes\n");
    exit(1);
  }
  AVHWFramesContext *hwframes = (AVHWFramesContext *)hwframes_ref->data;
  // must be AV_PIX_FMT_VAAPI, see av_hwframe_ctx_init
  hwframes->format = AV_PIX_FMT_VAAPI;
  // must be set, see vaapi_frames_init
  hwframes->sw_format = AV_PIX_FMT_BGR0;
  hwframes->width = 1024;
  hwframes->height = 768;
  r = av_hwframe_ctx_init(hwframes_ref);
  if (r < 0) {
    fprintf(stderr, "av_hwframe_ctx_init failed: %d\n", r);
    exit(1);
  }

  enum AVPixelFormat *formats;
  r = av_hwframe_transfer_get_formats(hwframes_ref, AV_HWFRAME_TRANSFER_DIRECTION_FROM, &formats, 0);
  if (r < 0) {
    fprintf(stderr, "av_hwframe_transfer_get_formats failed: %d\n", r);
    exit(1);
  }

  bool comma = false;
  for (int i = 0; ; i++) {
    if (formats[i] == AV_PIX_FMT_NONE) {
      break;
    }

    if (comma) {
      printf(", ");
    } else {
      comma = true;
    }

    printf("%s", av_get_pix_fmt_name(formats[i]));
  }

  if (comma) {
    printf("\n");
  }

  av_freep(&formats);
  av_freep(&hwframes_ref);
  av_freep(&hwdevice_ref);

  return 0;
}
