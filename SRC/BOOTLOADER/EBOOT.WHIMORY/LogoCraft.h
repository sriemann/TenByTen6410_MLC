#ifndef LOGOCRAFT_H
#define LOGOCRAFT_H

#include <bsp.h>


typedef struct 
{
	unsigned width;
	unsigned height;
	unsigned BPP;
	unsigned background;
	unsigned char data[];
}BOOT_LOGO;



void displayLogo();


#endif