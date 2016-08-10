LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := ffcodec

LOCAL_SRC_FILES:= \
	ffutil.cpp \
	ffdecoder.cpp \
	ffencoder.cpp

LOCAL_CFLAGS := -fPIC -D__STDC_CONSTANT_MACROS -DANDROID
LOCAL_LDLIBS := -Wl,--no-warn-shared-textrel -llog -lz

LOCAL_SHARED_LIBRARIES := ffmpeg x264 openh264


include $(BUILD_SHARED_LIBRARY)
