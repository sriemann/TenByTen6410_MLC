#ifndef IIC_H
#define IIC_H

#include <windows.h>
#include <bsp.h>
#include <soc_cfg.h>
#include <bsp_cfg.h>
#include "s3c6410_base_regs.h"
#include "s3c6410_gpio.h"
#include "s3c6410_iic.h"
#include "s3c6410_syscon.h"


int probe_megadisplay( int device_addr);
int IICWritebyte(unsigned char iccaddr,unsigned char regaddr,unsigned char value);
void Delayus(unsigned dwCount);

#endif