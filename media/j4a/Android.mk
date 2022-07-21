# copyright (c) 2016 Zhang Rui <bbcallen@gmail.com>
#
# This file is part of ijkPlayer.
#
# ijkPlayer is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# ijkPlayer is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with ijkPlayer; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_CFLAGS += -std=c99

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH) \
    $(LOCAL_PATH)/j4a

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_C_INCLUDES)

LOCAL_SRC_FILES := \
	$(wildcard $(LOCAL_PATH)/j4a/*.c) \
	$(wildcard $(LOCAL_PATH)/j4a/class/android/media/*.c) \
	$(wildcard $(LOCAL_PATH)/j4a/class/android/os/*.c) \
	$(wildcard $(LOCAL_PATH)/j4a/class/java/nio/*.c) \
	$(wildcard $(LOCAL_PATH)/j4a/class/java/util/*.c) \
	$(wildcard $(LOCAL_PATH)/j4a/class/tv/danmaku/ijk/media/player/misc/*.c) \
	$(wildcard $(LOCAL_PATH)/j4a/class/tv/danmaku/ijk/media/player/*.c)

LOCAL_MODULE := j4a

include $(BUILD_STATIC_LIBRARY)

$(call import-module,android/cpufeatures)
