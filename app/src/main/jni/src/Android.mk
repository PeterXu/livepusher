LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE:= libPusher

ifeq ($(TARGET_ARCH_ABI), armeabi-v7a)  
    LOCAL_ARM_NEON := true  
endif

LOCAL_SRC_FILES :=  \
	cmdutils.c \
	ffmpeg_opt.c \
	ffmpeg_filter.c \
	ffmpeg.c \
	unix.c \
	japi.c
	
LOCAL_CFLAGS := -std=c99 -fPIC
LOCAL_LDLIBS := -Wl,--no-warn-shared-textrel -llog -lz

LOCAL_SHARED_LIBRARIES := ffmpeg x264 openh264
LOCAL_STATIC_LIBRARIES := cpufeatures

include $(BUILD_SHARED_LIBRARY)

$(call import-module, android/cpufeatures)
