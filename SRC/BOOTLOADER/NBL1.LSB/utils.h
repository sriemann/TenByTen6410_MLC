#ifndef __UTILS_H__
#define __UTILS_H__

void Port_Init(void);
void Led_Display(int);

void Uart_Init(void);
void Uart_SendByte(int data);
void Uart_SendString(char *pt);
void Uart_SendDWORD(DWORD d, BOOL cr);
void Uart_SendBYTE(BYTE d, BOOL cr);
char *hex2char(unsigned int val);

#endif	// __UTILS_H__
