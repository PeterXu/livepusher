LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE:= libPusher

ifeq ($(TARGET_ARCH_ABI), armeabi-v7a)  
    LOCAL_ARM_NEON := true  
endif

LOCAL_SRC_FILES :=  \
	proto.c \
	japi.cc
	
LOCAL_CFLAGS := -fPIC -D__STDC_CONSTANT_MACROS
LOCAL_LDLIBS := -Wl,--no-warn-shared-textrel -llog -lz

LOCAL_SHARED_LIBRARIES := ffmpeg x264 openh264
LOCAL_STATIC_LIBRARIES := cpufeatures

include $(BUILD_SHARED_LIBRARY)

$(call import-module, android/cpufeatures)
