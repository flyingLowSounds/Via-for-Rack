EXTERNAL_LIBRARY_DIR = ./
# EXTERNAL_LIBRARY_DIR = /Users/willmitchell/via-dev-environment/via_hardware_executables/hardware_drivers/


# FLAGS will be passed to both the C and C++ compiler
FLAGS += \
	-DBUILD_VIRTUAL \
	-I$(EXTERNAL_LIBRARY_DIR)Via/modules/inc \
	-I$(EXTERNAL_LIBRARY_DIR)Via/synthesis/inc \
	-I$(EXTERNAL_LIBRARY_DIR)Via/io/inc \
	-I$(EXTERNAL_LIBRARY_DIR)Via/ui/inc \

CFLAGS +=
CXXFLAGS +=

# Careful about linking to shared libraries, since you can't assume much about the user's environment and library search path.
# Static libraries are fine.
LDFLAGS +=

# Add .cpp and .c files to the build
SOURCES += $(wildcard src/*.cpp)


SOURCES += $(wildcard $(EXTERNAL_LIBRARY_DIR)Via/io/src/*.cpp)
SOURCES += $(wildcard $(EXTERNAL_LIBRARY_DIR)Via/ui/src/*.cpp)
SOURCES += $(wildcard $(EXTERNAL_LIBRARY_DIR)Via/modules/*/*.cpp)

# SOURCES += $(wildcard $(patsubst %, %Via/io/src/*.cpp, $(EXTERNAL_LIBRARY_DIR)))
# SOURCES += $(wildcard $(patsubst %, %Via/ui/src/*.cpp, $(EXTERNAL_LIBRARY_DIR)))
# SOURCES += $(wildcard $(patsubst %, %Via/modules/*/*.cpp, $(EXTERNAL_LIBRARY_DIR)))

# Add files to the ZIP package when running `make dist`
# The compiled plugin is automatically added.
DISTRIBUTABLES += $(wildcard LICENSE*) res

RACK_DIR ?= ../..

# Include the VCV plugin Makefile framework
include $(RACK_DIR)/plugin.mk