# Thanks to Job Vranish (https://spin.atomicobject.com/2016/08/26/makefile-c-projects/)
TARGET_EXEC := engine

BUILD_DIR := ./build
SRC_DIRS := ./src ext/volk ext/spirv_reflect/ ext/imgui/imgui.cpp ext/imgui/imgui_demo.cpp ext/imgui/imgui_draw.cpp ext/imgui/imgui_widgets.cpp ext/imgui/imgui_tables.cpp ext/imgui/backends/imgui_impl_vulkan.cpp ext/imgui/backends/imgui_impl_glfw.cpp

# Find all the C and C++ files we want to compile
# Note the single quotes around the * expressions. Make will incorrectly expand these otherwise.
SRCS := $(shell find $(SRC_DIRS) -name '*.cpp' -or -name '*.c' -or -name '*.s')

# String substitution for every C/C++ file.
# As an example, hello.cpp turns into ./build/hello.cpp.o
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)

# String substitution (suffix version without %).
# As an example, ./build/hello.cpp.o turns into ./build/hello.cpp.d
DEPS := $(OBJS:.o=.d)

# Every folder in ./src will need to be passed to GCC so that it can find header files
INC_DIRS := $(shell find $(SRC_DIRS) -type d)
# Add a prefix to INC_DIRS. So moduleA would become -ImoduleA. GCC understands this -I flag
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

# The -MMD and -MP flags together generate Makefiles for us!
# These files will have .d instead of .o as the output.
CPPFLAGS := $(INC_FLAGS) -MMD -MP

# The final build step.
$(BUILD_DIR)/$(TARGET_EXEC): $(OBJS)
	$(CC) $(OBJS)  -fstack-protector  -w -I./ext/ -L./ext/vulkan/lib/x64  -L./ext/glfw/build/src  -lglfw3 -L/usr/X11R6/lib  -lX11 -Wl,-Bstatic -lshaderc_combined  -Wl,-Bdynamic  -lstdc++ -lm -ldl -lpthread -o $@ $(LDFLAGS)
# Build step for C source
$(BUILD_DIR)/%.c.o: %.c
	mkdir -p $(dir $@)
	$(CC) -g $(CPPFLAGS) $(CFLAGS) -w -I./ext -c $<  -o $@

# Build step for C++ source
$(BUILD_DIR)/%.cpp.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) -g $(CPPFLAGS) $(CXXFLAGS) -w -I./ext -I./ext/imgui -c $< -o $@


.PHONY: clean
clean:
	rm -r $(BUILD_DIR)

# Include the .d makefiles. The - at the front suppresses the errors of missing
# Makefiles. Initially, all the .d files will be missing, and we don't want those
# errors to show up.
-include $(DEPS)
