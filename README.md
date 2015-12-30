# Requirements

* ARM Cross Compiler: GCC

# ARM Cross Compiler: GCC

This is a pre-built (32bit) version of Linaro GCC that runs on generic linux, so 64bit users need to make sure they have installed the 32bit libraries for their distribution.

# Debian OS 7,8

    dpkg --add-architecture i386
    apt-get update
    apt-get install libstdc++6:i386 libgcc1:i386 zlib1g:i386 libncurses5:i386

Download and extract:

    wget -c https://releases.linaro.org/14.09/components/toolchain/binaries/gcc-linaro-arm-linux-gnueabihf-4.9-2014.09_linux.tar.xz
    tar xf gcc-linaro-arm-linux-gnueabihf-4.9-2014.09_linux.tar.xz
    export CC=`pwd`/gcc-linaro-arm-linux-gnueabihf-4.9-2014.09_linux/bin/arm-linux-gnueabihf-

# First Configure:

    make ARCH=arm CROSS_COMPILE=${CC} distclean
    make ARCH=arm CROSS_COMPILE=${CC} singlecore-omap2plus_defconfig

The ***.config*** file will be generated.   

# Build kernel image, modules, device tree:

    make ARCH=arm CROSS_COMPILE=${CC} -j4 LOADADDR=0x80008000 uImage dtbs modules   

# Install kernel image:

    sudo cp -v ./arch/arm/boot/zImage /media/rootfs/boot/vmlinuz-3.14.17   

# Install modules

    mkdir -p /tmp/deploy
    make -s ARCH=arm CROSS_COMPILE="${CC}" modules_install INSTALL_MOD_PATH=/tmp/deploy

    cd /tmp/deploy
    tar --create --gzip --file 3.14.17-modules.tar.gz *
    sudo tar vxzf 3.14.17-modules.tar.gz -C /media/rootfs/

    cd ..
    rm -rf /tmp/deploy   

# Install Firmware

**Note: this step seems not necessary for BotBone.**

    mkdir -p /tmp/deploy
    make -s ARCH=arm CROSS_COMPILE="${CC}" firmware_install INSTALL_FW_PATH=/tmp/deploy

    cd /tmp/deploy
    tar --create --gzip --file 3.14.17-firmware.tar.gz *
    sudo tar vxzf 3.14.17-firmware.tar.gz -C /media/rootfs/lib/firmware

    cd ..
    rm -rf /tmp/deploy   

# Install Device Tree

    sudo cp -v ./arch/arm/boot/dts/am335x-botbone.dtb /media/rootfs/boot/