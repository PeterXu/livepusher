LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE:= libPusher

ifeq ($(TARGET_ARCH_ABI), armeabi-v7a)  
    LOCAL_ARM_NEON := true  
endif

LOCAL_SRC_FILES :=  \
	rtmp_faac.c	\
	queue.c
	
LOCAL_CFLAGS := -std=c99 -fPIC
LOCAL_LDLIBS := -Wl,--no-warn-shared-textrel -llog -lz

LOCAL_STATIC_LIBRARIES := faac x264 rtmp

include $(BUILD_SHARED_LIBRARY)
