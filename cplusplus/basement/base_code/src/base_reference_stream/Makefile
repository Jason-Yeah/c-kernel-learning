.PHONY: all clean rebuild run

BUILD_DIR := build

all: $(BUILD_DIR)/Makefile
	$(MAKE) -C $(BUILD_DIR)

$(BUILD_DIR)/Makefile: CMakeLists.txt ref_demo.cpp ref_demo.h
	cmake -S . -B $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)

rebuild: clean all

run: all
	./ref_demo
