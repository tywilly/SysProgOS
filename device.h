#include <types.h>

typedef struct device_driver_s {
	char* name;
	int(*write)(int,const void*,uint32);
	int(*read)(int,void*,uint32);
	int(*lseek)(int,int,int);
} DeviceDriver;

