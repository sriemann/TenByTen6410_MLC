#ifndef __UTILS_H__
#define __UTILS_H__

#define LED_ON		0xa
#define LED_OFF		0x0

#ifdef __cplusplus
	extern "C" {
#endif

void Port_Init(void);
void Led_Display(int data);
void Delay(void);

void Uart_Init(void);
void Uart_SendByte(int data);
void Uart_SendString(char *pt);
void Uart_SendDWORD(DWORD d, BOOL cr);
void Uart_SendBYTE(BYTE d, BOOL cr);

char *hex2char(unsigned int val);

#ifdef __cplusplus
}
#endif

#endif // __UTILS_H__
