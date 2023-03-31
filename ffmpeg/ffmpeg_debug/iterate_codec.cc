#include <iostream>

extern "C" {
#include <libavcodec/avcodec.h>
}

using namespace std;

int main() {
    AVCodec *codec;
    void *i = 0;

    while ((codec = (AVCodec *)av_codec_iterate(&i))) {
        if (codec->type != AVMEDIA_TYPE_VIDEO || !av_codec_is_encoder(codec)) {
            continue;
        }

        if (!strcmp(codec->name, "h264_vaapi")) {
            cout << codec->name << endl;
            if (codec->capabilities & AV_CODEC_CAP_HARDWARE) {
                cout << "ha_accel" << endl;
            }
        }
    }

    return 0;
}
