# vim: ft=make :
# How to invoke this:
#
# - Copy the IBM_XL binaries to scripts/ci/images directory:
# - Binaries are located at KW SMB dir `Downloads/Compilers/IBM/` or IBM site
# - Run this: make -f power8-el7-xl.Makefile
# - push built images
TARGETS = config_binfmt-stamp emu-stamp xl-stamp
BUILD_ARGS = --squash
all: $(TARGETS)

config_binfmt-stamp:
	touch $@
	sudo docker run --rm --privileged multiarch/qemu-user-static:register --reset

emu-stamp: BUILD_ARGS+= --build-arg TARGET_CPU=power8
emu-stamp: emu-el7-base/Dockerfile config_binfmt-stamp
	touch $@
	sudo docker build $(BUILD_ARGS) -f $< -t ornladios/adios2:ci-x86_64-power8-el7-base . || (rm $@; false)

xl-stamp: BUILD_ARGS+= --build-arg COMPILER=xl
xl-stamp: power8-el7/power8-el7-xl.dockerfile emu-stamp
	touch $@
	sudo docker build $(BUILD_ARGS) -f $< -t ornladios/adios2:ci-x86_64-power8-el7-xl . || (rm $@; false)

clean:
	rm $(TARGETS) || true
