#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <xf86drm.h>

#include <iostream>
using namespace std;

// g++ -o egl egl.cc -I/usr/include/libdrm -ldrm -lEGL

/* Compute the size of an array */
#ifndef ARRAY_SIZE
#  define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

int main() {
    auto eglQueryDevicesEXT = (PFNEGLQUERYDEVICESEXTPROC)eglGetProcAddress("eglQueryDevicesEXT");
    auto eglQueryDeviceStringEXT = (PFNEGLQUERYDEVICESTRINGEXTPROC) eglGetProcAddress("eglQueryDeviceStringEXT");

    const char *exts = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
    if (!exts) {
        cout << "eglQueryString failed\n";
        return 1;
    }

    cout << "egl extensions:\n";
    cout << exts << endl;

//    EGL_EXT_platform_base
//    EGL_EXT_device_base
//    EGL_EXT_device_enumeration
//    EGL_EXT_device_query
//    EGL_KHR_client_get_all_proc_addresses
//    EGL_EXT_client_extensions
//    EGL_KHR_debug
//    EGL_KHR_platform_x11
//    EGL_EXT_platform_x11
//    EGL_EXT_platform_device
//    EGL_KHR_platform_wayland
//    EGL_EXT_platform_wayland
//    EGL_KHR_platform_gbm
//    EGL_MESA_platform_gbm
//    EGL_MESA_platform_xcb
//    EGL_MESA_platform_surfaceless

    {
        drmDevicePtr devices[64];
        int num_devs;
        num_devs = drmGetDevices2(0, devices, ARRAY_SIZE(devices));
        cout << "drm device count: " << num_devs << endl;
    }

    const int MAX_DEVICES = 16;
    EGLDeviceEXT eglDevs[MAX_DEVICES] = {nullptr};
    EGLint eglDeviceNum = 0;
    auto r = eglQueryDevicesEXT(MAX_DEVICES, eglDevs, &eglDeviceNum);
    if (r != EGL_TRUE) {
        cout << "eglQueryDevicesEXT failed\n";
        return 1;
    }

    cout << "egl device count: " << eglDeviceNum << endl;

    for (int i = 0; i < eglDeviceNum; i++) {
        const char * devStr = eglQueryDeviceStringEXT(eglDevs[i], EGL_DRM_DEVICE_FILE_EXT);
        if (devStr) {
            cout << "egl device[" << i << "]: " << devStr << endl;
        }
    }

    return 0;
}
