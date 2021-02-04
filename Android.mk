LOCAL_PATH := $(call my-dir)
TARGET_ARCH_ABI := $(APP_ABI)

rwildcard=$(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2))

# Creating prebuilt for dependency: beatsaber-hook - version: 1.0.12
include $(CLEAR_VARS)
LOCAL_MODULE := beatsaber-hook_1_0_12
LOCAL_EXPORT_C_INCLUDES := extern/beatsaber-hook
LOCAL_SRC_FILES := extern/libbeatsaber-hook_1_0_12.so
LOCAL_CPP_FEATURES += exceptions
include $(PREBUILT_SHARED_LIBRARY)
# Creating prebuilt for dependency: modloader - version: 1.0.4
include $(CLEAR_VARS)
LOCAL_MODULE := modloader
LOCAL_EXPORT_C_INCLUDES := extern/modloader
LOCAL_SRC_FILES := extern/libmodloader.so
LOCAL_CPP_FEATURES += exceptions
include $(PREBUILT_SHARED_LIBRARY)
# Creating prebuilt for dependency: codegen - version: 0.6.2
include $(CLEAR_VARS)
LOCAL_MODULE := codegen_0_6_2
LOCAL_EXPORT_C_INCLUDES := extern/codegen
LOCAL_SRC_FILES := extern/libcodegen_0_6_2.so
LOCAL_CPP_FEATURES += exceptions
include $(PREBUILT_SHARED_LIBRARY)

# Pre-release song loader
include $(CLEAR_VARS)
LOCAL_MODULE := oldsongloader_dangerous_dontuse
LOCAL_EXPORT_C_INCLUDES := extern/codegen
LOCAL_SRC_FILES := extern/liboldsongloader_dangerous_dontuse.so
LOCAL_CPP_FEATURES += exceptions
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := multiplayer
LOCAL_SRC_FILES += $(call rwildcard,src/,*.cpp)
LOCAL_SRC_FILES += $(call rwildcard,extern/beatsaber-hook/src/inline-hook/,*.cpp)
LOCAL_SRC_FILES += $(call rwildcard,extern/beatsaber-hook/src/inline-hook/,*.c)
LOCAL_SRC_FILES += $(call rwildcard,extern/zip/src/,*.c)
LOCAL_SHARED_LIBRARIES += beatsaber-hook_1_0_12
LOCAL_SHARED_LIBRARIES += modloader
LOCAL_SHARED_LIBRARIES += codegen_0_6_2
LOCAL_SHARED_LIBRARIES += oldsongloader_dangerous_dontuse
LOCAL_LDLIBS += -llog
LOCAL_CFLAGS += -DVERSION='"1.0.0"' -DID='"multiplayer"' -isystem 'extern/libil2cpp/il2cpp/libil2cpp'
LOCAL_CPPFLAGS += -std=c++2a -frtti
LOCAL_C_INCLUDES += ./include ./src ./shared ./extern ./extern/beatsaber-hook/shared ./extern/modloader/shared ./extern/questui/shared ./extern/custom-types/shared ./extern/codegen/include ./extern/zip/src
LOCAL_CPP_FEATURES += exceptions
include $(BUILD_SHARED_LIBRARY)
