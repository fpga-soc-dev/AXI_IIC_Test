BUILD_DIR = $(abspath build)
SHELL = bash
.SHELLFLAGS += -e

.PHONY: build_yocto_project
build_yocto_project:
	@echo "Building Yocto Project..."
	cp ../Vivado/pynq_z1_top.xsa meta-user/recipes-bsp/hdf/files/pynq_z1_top.xsa
	kas build build.yaml
	rm meta-user/recipes-bsp/hdf/files/pynq_z1_top.xsa

.PHONY: clean
clean:
	@echo "Cleaning Yocto Project build directory..."
	rm -rf $(BUILD_DIR)
