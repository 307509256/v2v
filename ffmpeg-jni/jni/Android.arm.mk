LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE:= ijkffmpeg
LOCAL_SRC_FILES:= prebuilt/armeabi-v7a/libijkffmpeg.so
include $(PREBUILT_SHARED_LIBRARY)


include $(CLEAR_VARS)

LOCAL_MODULE := ffmpegjni

LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES := FFmpegJni.c \
                   ffmpeg.c \
                   cmdutils.c \
                   ffmpeg_opt.c \
                   ffmpeg_filter.c \
                   kp_ffmpeg_api.c

LOCAL_LDLIBS := -L$(SYSROOT)/usr/lib -llog -lz

LOCAL_SHARED_LIBRARIES := ijkffmpeg 

LOCAL_C_INCLUDES += -L$(SYSROOT)/usr/include
LOCAL_C_INCLUDES += $(LOCAL_PATH)/include

# -mfpu=neon -O3 -ffast-math -funroll-loops
LOCAL_CFLAGS := -DUSE_ARM_CONFIG

include $(BUILD_SHARED_LIBRARY)



