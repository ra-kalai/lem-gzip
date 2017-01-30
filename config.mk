CC := gcc

PKG_CONFIG_PATH := /usr/local/lib/pkgconfig/
PKG_CONFIG := pkg-config
cmoddir = $(shell \
            PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) $(PKG_CONFIG) --variable=INSTALL_CMOD lem)

CFLAGS := -fPIC -g -O2 -Wall -nostartfiles -shared \
       $(shell \
         PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) \
         $(PKG_CONFIG) --cflags lem)

LDFLAGS := -lz
