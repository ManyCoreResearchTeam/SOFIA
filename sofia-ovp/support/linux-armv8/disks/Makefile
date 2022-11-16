# Build Linux arm64 kernel and DTBs

# GCC Crosscompiler
#  (from: http://releases.linaro.org/archive/13.11/components/toolchain/binaries/$(GCC_VERSION).tar.xz)
GCC_HOME=/home/tools/local/Linux32
GCC_VERSION=gcc-linaro-aarch64-linux-gnu-4.8-2013.11
_CROSS_COMPILE=aarch64-linux-gnu-

# Set up path to use normal host compiler and cross compiler
PATH:=/usr/bin:$(GCC_HOME)/$(GCC_VERSION)/bin:$(PATH)

#TAG?=lsk-v3.18-15.10
#TAG?=lsk-v3.14-15.10
#TAG?=ll_20140321.0
#LINARO?=linaro-stable

TAG?=linux-linaro-4.3-2015.11
LINARO?=linaro-tracking

KERNEL_VERSION=$(LINARO)-$(TAG)

# Device tree blobs
DTBS=foundation-v8-big_little-GICv3.dts  foundation-v8.dts  foundation-v8-gicv3compatibility.dts  foundation-v8-gicv3.dts  multicluster-57x2_53x4-GICv2.dts

all: Image-$(KERNEL_VERSION) vmlinux-$(KERNEL_VERSION) $(DTBS)
dtb: $(DTBS)

# Linaro kernel
Image-$(KERNEL_VERSION): linux-$(KERNEL_VERSION)/arch/arm64/boot/Image
	cp $< $@

vmlinux-$(KERNEL_VERSION): linux-$(KERNEL_VERSION)/vmlinux
	cp $< $@

linux-$(KERNEL_VERSION)/arch/arm64/boot/Image linux-$(KERNEL_VERSION)/vmlinux: linux-$(KERNEL_VERSION)/.config 
	make -C linux-$(KERNEL_VERSION) ARCH=arm64 CROSS_COMPILE=$(_CROSS_COMPILE) CONFIG_DEBUG_INFO=1 Image -j 8 $(OPTIONS)

linux-$(KERNEL_VERSION)/.config: | linux-$(KERNEL_VERSION)
	(cd linux-$(KERNEL_VERSION); ARCH=arm64 scripts/kconfig/merge_config.sh \
		linaro/configs/linaro-base.conf \
		linaro/configs/linaro-base64.conf \
		linaro/configs/distribution.conf \
		linaro/configs/vexpress64.conf \
		linaro/configs/kvm-host.conf \
		linaro/configs/kvm-guest.conf )

linux-$(KERNEL_VERSION):
	git clone http://git.linaro.org/kernel/linux-$(LINARO).git
	mv linux-$(LINARO) linux-$(KERNEL_VERSION)
	(cd linux-$(KERNEL_VERSION); git checkout $(TAG))

# Device tree blobs - assumes device tree source files are in current directroy
%.dtb: %.dts
	linux-$(KERNEL_VERSION)/scripts/dtc/dtc -I dts -O dtb $< > $@

# clean Linux kernel directory
clean:
	make -C linux-$(KERNEL_VERSION) ARCH=arm64 clean

# Clean all sources, leaving only images
realclean:
	rm -r linux-$(KERNEL_VERSION) 

