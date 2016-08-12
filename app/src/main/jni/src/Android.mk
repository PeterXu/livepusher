LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE:= libPusher

ifeq ($(TARGET_ARCH_ABI), armeabi-v7a)  
    LOCAL_ARM_NEON := true  
endif

LOCAL_SRC_FILES :=  \
	audio_stream.c	\
	video_stream.c	\
	rtmp_stream.c	\
	queue.c
	
LOCAL_CFLAGS := -std=c99 -fPIC
LOCAL_LDLIBS := -Wl,--no-warn-shared-textrel -Wl,-Bdynamic -llog -lz

LOCAL_STATIC_LIBRARIES := faac x264 rtmp
LOCAL_STATIC_LIBRARIES += cpufeatures

include $(BUILD_SHARED_LIBRARY)

$(call import-module, android/cpufeatures)
