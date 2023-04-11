#include <iostream>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include <va/va.h>
#include <va/va_drm.h>

extern "C" {
#include <libavutil/pixfmt.h>
#include <libavutil/pixdesc.h>
}

using namespace std;

const char *dev = "/dev/dri/card0";

typedef struct VAAPIFormat {
    unsigned int fourcc;
    unsigned int rt_format;
    enum AVPixelFormat pix_fmt;
    int chroma_planes_swapped;
} VAAPIFormatDescriptor;

#define MAP(va, rt, av, swap_uv) { \
        VA_FOURCC_ ## va, \
        VA_RT_FORMAT_ ## rt, \
        AV_PIX_FMT_ ## av, \
        swap_uv, \
    }
// The map fourcc <-> pix_fmt isn't bijective because of the annoying U/V
// plane swap cases.  The frame handling below tries to hide these.
static const VAAPIFormatDescriptor vaapi_format_map[] = {
    MAP(NV12, YUV420,  NV12,    0),
#ifdef VA_FOURCC_I420
    MAP(I420, YUV420,  YUV420P, 0),
#endif
    MAP(YV12, YUV420,  YUV420P, 1),
    MAP(IYUV, YUV420,  YUV420P, 0),
    MAP(422H, YUV422,  YUV422P, 0),
#ifdef VA_FOURCC_YV16
    MAP(YV16, YUV422,  YUV422P, 1),
#endif
    MAP(UYVY, YUV422,  UYVY422, 0),
    MAP(YUY2, YUV422,  YUYV422, 0),
#ifdef VA_FOURCC_Y210
    MAP(Y210, YUV422_10,  Y210, 0),
#endif
    MAP(411P, YUV411,  YUV411P, 0),
    MAP(422V, YUV422,  YUV440P, 0),
    MAP(444P, YUV444,  YUV444P, 0),
    MAP(Y800, YUV400,  GRAY8,   0),
#ifdef VA_FOURCC_P010
    MAP(P010, YUV420_10BPP, P010, 0),
#endif
    MAP(BGRA, RGB32,   BGRA, 0),
    MAP(BGRX, RGB32,   BGR0, 0),
    MAP(RGBA, RGB32,   RGBA, 0),
    MAP(RGBX, RGB32,   RGB0, 0),
#ifdef VA_FOURCC_ABGR
    MAP(ABGR, RGB32,   ABGR, 0),
    MAP(XBGR, RGB32,   0BGR, 0),
#endif
    MAP(ARGB, RGB32,   ARGB, 0),
    MAP(XRGB, RGB32,   0RGB, 0),
#ifdef VA_FOURCC_X2R10G10B10
    MAP(X2R10G10B10, RGB32_10, X2RGB10, 0),
#endif
};
#undef MAP

static const VAAPIFormatDescriptor *
vaapi_format_from_fourcc(unsigned int fourcc)
{
    int i;
    for (i = 0; i < FF_ARRAY_ELEMS(vaapi_format_map); i++)
        if (vaapi_format_map[i].fourcc == fourcc)
            return &vaapi_format_map[i];
    return NULL;
}

static const VAAPIFormatDescriptor *
vaapi_format_from_pix_fmt(enum AVPixelFormat pix_fmt)
{
    int i;
    for (i = 0; i < FF_ARRAY_ELEMS(vaapi_format_map); i++)
        if (vaapi_format_map[i].pix_fmt == pix_fmt)
            return &vaapi_format_map[i];
    return NULL;
}

static enum AVPixelFormat vaapi_pix_fmt_from_fourcc(unsigned int fourcc)
{
    const VAAPIFormatDescriptor *desc;
    desc = vaapi_format_from_fourcc(fourcc);
    if (desc)
        return desc->pix_fmt;
    else
        return AV_PIX_FMT_NONE;
}

int main() {
    int fd = open(dev, O_RDWR);
    if (fd < 0) {
        int err = errno;
        fprintf(stderr, "open %s failed: %s\n", dev, strerror(err));
        exit(1);
    }

    VADisplay dpy = vaGetDisplayDRM(fd);
    if (!dpy) {
        fprintf(stderr, "can not open VA display\n");
        exit(1);
    }

    int major, minor;
    VAStatus r = vaInitialize(dpy, &major, &minor);
    if (r != VA_STATUS_SUCCESS) {
        fprintf(stderr, "vaInitialize failed: %s\n", vaErrorStr(r));
        exit(1);
    }

    VAConfigID config_id = VA_INVALID_ID;
    r = vaCreateConfig(dpy, VAProfileH264ConstrainedBaseline, VAEntrypointVLD, NULL, 0, &config_id);
    if (r != VA_STATUS_SUCCESS) {
        fprintf(stderr, "vaCreateConfig failed: %s\n", vaErrorStr(r));
        exit(1);
    }

    unsigned int attr_count;
    r = vaQuerySurfaceAttributes(dpy, config_id, NULL, &attr_count);
    if (r != VA_STATUS_SUCCESS) {
        fprintf(stderr, "vaQuerySurfaceAttributes failed: %s\n", vaErrorStr(r));
        exit(1);
    }

    VASurfaceAttrib *attr_list = (VASurfaceAttrib *)malloc(attr_count * sizeof(*attr_list));
    r = vaQuerySurfaceAttributes(dpy, config_id, attr_list, &attr_count);
    if (r != VA_STATUS_SUCCESS) {
        fprintf(stderr, "vaQuerySurfaceAttributes failed: %s\n", vaErrorStr(r));
        exit(1);
    }

    int pix_fmt_count = 0;
    unsigned fourcc;
    enum AVPixelFormat pix_fmt;
    enum AVPixelFormat *valid_sw_formats = NULL;
    int min_width = 0, min_height = 0, max_width = 0, max_height = 0;
    for (int i = 0; i < attr_count; i++) {
        switch (attr_list[i].type) {
        case VASurfaceAttribPixelFormat:
            fourcc = attr_list[i].value.value.i;
            pix_fmt = vaapi_pix_fmt_from_fourcc(fourcc);
            if (pix_fmt != AV_PIX_FMT_NONE) {
                ++pix_fmt_count;
            } else {
                // Something unsupported - ignore.
            }
            break;
        case VASurfaceAttribMinWidth:
            min_width = attr_list[i].value.value.i;
            break;
        case VASurfaceAttribMinHeight:
            min_height = attr_list[i].value.value.i;
            break;
        case VASurfaceAttribMaxWidth:
            max_width = attr_list[i].value.value.i;
            break;
        case VASurfaceAttribMaxHeight:
            max_height = attr_list[i].value.value.i;
            break;
        }
    }

    printf("min_width: %d, min_height: %d, max_width: %d, max_height: %d\n",
        min_width, min_height, max_width, max_height);

    if (pix_fmt_count == 0) {
        // Nothing usable found.  Presumably there exists something which
        // works, so leave the set null to indicate unknown.
        valid_sw_formats = NULL;
    } else {
        valid_sw_formats = (enum AVPixelFormat *)malloc((pix_fmt_count + 1) * sizeof(pix_fmt));
        if (!valid_sw_formats) {
            exit(1);
        }

        int i, j;
        for (i = j = 0; i < attr_count; i++) {
            int k;

            if (attr_list[i].type != VASurfaceAttribPixelFormat)
                continue;
            fourcc = attr_list[i].value.value.i;
            pix_fmt = vaapi_pix_fmt_from_fourcc(fourcc);

            if (pix_fmt == AV_PIX_FMT_NONE)
                continue;

            for (k = 0; k < j; k++) {
                if (valid_sw_formats[k] == pix_fmt)
                    break;
            }

            if (k == j)
                valid_sw_formats[j++] = pix_fmt;
        }
        valid_sw_formats[j] = AV_PIX_FMT_NONE;
    }

    bool comma = false;
    for (int i = 0; ; i++) {
        if (valid_sw_formats[i] == AV_PIX_FMT_NONE) {
            break;
        }

        if (comma) {
            printf(", ");
        } else {
            comma = true;
        }

        const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(valid_sw_formats[i]);
        if (desc) {
            printf("%s", desc->name);
        }
    }

    if (comma) {
        printf("\n");
    }

    vaDestroyConfig(dpy, config_id);
    vaTerminate(dpy);
    free(attr_list);
    close(fd);
    return 0;
}
