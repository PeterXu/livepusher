LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/libs/faac/include
LOCAL_SRC_FILES := libs/faac/$(TARGET_ARCH_ABI)/lib/libfaac.a
$(info $(LOCAL_SRC_FILES))
LOCAL_MODULE:= faac
LOCAL_CFLAGS := -fPIC
include $(PREBUILT_STATIC_LIBRARY)


include $(CLEAR_VARS)
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/libs/x264/include
LOCAL_SRC_FILES := libs/x264/$(TARGET_ARCH_ABI)/lib/libx264.a
$(info $(LOCAL_SRC_FILES))
LOCAL_MODULE:= x264
LOCAL_CFLAGS := -fPIC
include $(PREBUILT_STATIC_LIBRARY)


include $(CLEAR_VARS)
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/libs/rtmpdump/include
LOCAL_SRC_FILES := libs/rtmpdump/$(TARGET_ARCH_ABI)/lib/librtmp.a
$(info $(LOCAL_SRC_FILES))
LOCAL_MODULE:= rtmp
LOCAL_CFLAGS := -fPIC
include $(PREBUILT_STATIC_LIBRARY)
