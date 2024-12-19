ccflags-y += -I$(srctree)/drivers/xenomai/net/stack/include
ccflags-y += -I$(src)

obj-$(CONFIG_XENO_DRIVERS_NET_DRV_E1000E) += rt_e1000e.o

rt_e1000e-y := \
	82571.o \
	80003es2lan.o \
	ich8lan.o \
 	mac.o \
	manage.o \
	nvm.o \
	netdev.o \
	param.o \
	phy.o
# Deleted 	ptp.o \
		ethtool.o from the above list
