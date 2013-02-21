//====================================================================
// File Name : option.h
// Function  : S3C6400 
// Program   : Lee Jae Yong
// Date      : February 22, 2007
// Version   : 0.0
// History
//   0.0 : Programming start (February 22,2007)
//====================================================================

#ifndef __OPTION_H__
#define __OPTION_H__

#define FCLK           400000000  // 400MHz
#define HCLK           (FCLK/4)   // divisor 4
#define PCLK           (FCLK/16)   // divisor 16

// BUSWIDTH : 16,32
#define BUSWIDTH    (32)


#define SB_NEED_EXT_ADDR				1
#define LB_NEED_EXT_ADDR				1

//If you use ADS1.x, please define ADS10
#define ADS10 TRUE

void Uart_Init(void);
void Uart_SendByte(int data);
void Uart_SendString(char *pt);
void Uart_SendDWORD(DWORD d, BOOL cr);
char *hex2char(unsigned int val);



// note: makefile,option.a should be changed

#endif /*__OPTION_H__*/
