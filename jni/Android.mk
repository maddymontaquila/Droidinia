LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := JNIProcessor
LOCAL_ARM_MODE := arm
LOCAL_SRC_FILES := processor.cpp libopencl.cpp
LOCAL_CPPFLAGS	:= -DARM -DOS_LNX -DARCH_32 -fexceptions -fPIC 
LOCAL_CPPFLAGS	+= -I$(LOCAL_PATH)/include
LOCAL_CPPFLAGS 	+= -fexceptions
LOCAL_LDLIBS := -ljnigraphics -llog 
include $(BUILD_SHARED_LIBRARY)
