TOPDIR	= ../../..
include ../../../Makefile.system

ifeq ($(DYNAMIC_ARCH), 1)
LASWP	= ../generic/laswp_k_4.c
ZLASWP	= ../generic/zlaswp_k_4.c
endif

ifndef LASWP
LASWP	= ../generic/laswp_k.c
endif

ifndef ZLASWP
ZLASWP	= ../generic/zlaswp_k.c
endif

include ../generic/Makefile

