#ifndef MEGADISPLAY_H
#define MEGADISPLAY_H

void identifyDisplay();
void backlight(unsigned char duty);
void beep(unsigned freq, unsigned delay);
void editDisplayMenu(unsigned type);


#endif