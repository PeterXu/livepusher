LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/libs/faac/include
ifeq ($(TARGET_ARCH_ABI), armeabi-v7a)
   LOCAL_SRC_FILES := libs/faac/$(TARGET_ARCH_ABI)-neon/lib/libfaac.a
else
   LOCAL_SRC_FILES := libs/faac/$(TARGET_ARCH_ABI)/lib/libfaac.a
endif
$(info $(LOCAL_SRC_FILES))
LOCAL_MODULE:= faac
LOCAL_CFLAGS := -fPIC
include $(PREBUILT_STATIC_LIBRARY)


include $(CLEAR_VARS)
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/libs/x264/include
ifeq ($(TARGET_ARCH_ABI), armeabi-v7a)
   LOCAL_SRC_FILES := libs/x264/$(TARGET_ARCH_ABI)-neon/lib/libx264.a
else
   LOCAL_SRC_FILES := libs/x264/$(TARGET_ARCH_ABI)/lib/libx264.a
endif
$(info $(LOCAL_SRC_FILES))
LOCAL_MODULE:= x264
LOCAL_CFLAGS := -fPIC
include $(PREBUILT_STATIC_LIBRARY)


include $(CLEAR_VARS)
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/libs/rtmpdump/include
ifeq ($(TARGET_ARCH_ABI), armeabi-v7a)
   LOCAL_SRC_FILES := libs/rtmpdump/$(TARGET_ARCH_ABI)-neon/lib/librtmp.a
else
   LOCAL_SRC_FILES := libs/rtmpdump/$(TARGET_ARCH_ABI)/lib/librtmp.a
endif
$(info $(LOCAL_SRC_FILES))
LOCAL_MODULE:= rtmp
LOCAL_CFLAGS := -fPIC
include $(PREBUILT_STATIC_LIBRARY)
