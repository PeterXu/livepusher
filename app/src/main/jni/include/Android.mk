LOCAL_PATH := $(call my-dir)
FFMPEG_ROOT := $(HOME)/wspace/offical/ijkplayer-android/android/contrib

include $(CLEAR_VARS)
LOCAL_MODULE:= ffmpeg
LOCAL_EXPORT_C_INCLUDES := $(FFMPEG_ROOT)/ffmpeg-armv7a
LOCAL_SRC_FILES := $(FFMPEG_ROOT)/build/ffmpeg-armv7a/output/libijkffmpeg.so
include $(PREBUILT_SHARED_LIBRARY)


include $(CLEAR_VARS)
LOCAL_MODULE:= x264
LOCAL_SRC_FILES := $(FFMPEG_ROOT)/build/ffmpeg-armv7a/output/libx264.so
include $(PREBUILT_SHARED_LIBRARY)


include $(CLEAR_VARS)
LOCAL_MODULE:= openh264
LOCAL_SRC_FILES := $(FFMPEG_ROOT)/build/ffmpeg-armv7a/output/libopenh264.so
include $(PREBUILT_SHARED_LIBRARY)

