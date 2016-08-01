LOCAL_PATH := $(call my-dir)
FFMPEG_ROOT := $(HOME)/wspace/offical/ijkplayer-android/android/contrib
ARCH=armv7a

include $(CLEAR_VARS)
LOCAL_MODULE:= ffmpeg
LOCAL_EXPORT_C_INCLUDES := $(FFMPEG_ROOT)/ffmpeg-$(ARCH)
LOCAL_SRC_FILES := $(FFMPEG_ROOT)/build/ffmpeg-$(ARCH)/output/libijkffmpeg.so
include $(PREBUILT_SHARED_LIBRARY)


include $(CLEAR_VARS)
LOCAL_MODULE:= x264
LOCAL_SRC_FILES := $(FFMPEG_ROOT)/build/ffmpeg-$(ARCH)/output/libx264.so
include $(PREBUILT_SHARED_LIBRARY)


include $(CLEAR_VARS)
LOCAL_MODULE:= openh264
LOCAL_SRC_FILES := $(FFMPEG_ROOT)/build/ffmpeg-$(ARCH)/output/libopenh264.so
include $(PREBUILT_SHARED_LIBRARY)

