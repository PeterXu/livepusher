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
LOCAL_LDLIBS := -llog -lz 

LOCAL_STATIC_LIBRARIES := rtmp faac x264  

include $(BUILD_SHARED_LIBRARY)