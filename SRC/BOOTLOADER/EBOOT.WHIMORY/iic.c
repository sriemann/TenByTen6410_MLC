//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this source code is subject to the terms of the Microsoft end-user
// license agreement (EULA) under which you licensed this SOFTWARE PRODUCT.
// If you did not accept the terms of the EULA, you are not authorized to use
// this source code. For a copy of the EULA, please see the LICENSE.RTF on your
// install media.
//

#include <windows.h>
#include <bsp.h>


#define  IICONACK   (0xaf)
#define  IICONNOACK (0x2f)


#ifndef WORD
typedef unsigned short WORD;
#endif

#ifndef DWORD
typedef unsigned long DWORD;
#endif


#define WRDATA      (1)
#define POLLACK     (2)
#define RDDATA      (3)
#define SETRDADDR   (4)

#define IICBUFSIZE 0x20

static unsigned char _iicData[IICBUFSIZE];
static volatile int _iicDataCount;
static volatile int _iicStatus;
static volatile int _iicMode;
static int _iicPt;

int dwtimeout;

void Delayus(unsigned dwCount)
{
	unsigned long dwTemp;
	for(dwTemp =0; dwTemp < dwCount*10; dwTemp++);
}


static void Delay(void)
{
    volatile int i,j,a;

    for(i=0 ; i<1000 ; i++)
      for(j=0 ; j<500 ; j++)
      { 
    	  ;
      }
}


void IICInit()
{
   
   volatile S3C6410_GPIO_REG *pGPIOReg = (S3C6410_GPIO_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_GPIO, FALSE);
   volatile S3C6410_IIC_REG * i2c = (S3C6410_IIC_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_IICBUS, FALSE);
   
		
   pGPIOReg->GPBCON =0x2200000;
  
  //Enable ACK, Prescaler IICCLK=PCLK/16, Enable interrupt, Transmit clock value Tx clock=IICCLK/16
   i2c->IICCON=IICONACK;//0xAF
}


int IICPoll( )
{
    unsigned int  iicSt,i;
    volatile S3C6410_IIC_REG * i2c = (S3C6410_IIC_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_IICBUS, FALSE);
   
   iicSt = i2c->IICSTAT; 
    if(iicSt & 0x8){}                   //When bus arbitration is failed.
    if(iicSt & 0x4){}                   //When a slave address is matched with IICADD
    if(iicSt & 0x2){}                   //When a slave address is 0000000b
    if(iicSt & 0x1){}                   //When ACK isn't received

    switch(_iicMode)
    {
        case POLLACK:
            _iicStatus = iicSt;
            break;

        case RDDATA:
            if((_iicDataCount--)==0)
            {
                _iicData[_iicPt++] =(unsigned char) i2c->IICDS;
            
                i2c->IICSTAT = 0x90;                //Stop MasRx condition 
                i2c->IICCON  = IICONACK;                //Resumes IIC operation.
                Delayus(1);                       //Wait until stop condtion is in effect.
                                                //Too long time... 
                                                //The pending bit will not be set after issuing stop condition.
                break;    
            }      
            _iicData[_iicPt++] =(unsigned char) i2c->IICDS;
                        //The last data has to be read with no ack.
            if((_iicDataCount)==0)
                i2c->IICCON = IICONNOACK;                 //Resumes IIC operation with NOACK.  
            else 
                i2c->IICCON = IICONACK;                 //Resumes IIC operation with ACK
            break;

        case WRDATA:
            if((_iicDataCount--)==0)
            {
                i2c->IICSTAT = 0xd0;                //stop MasTx condition 
                i2c->IICCON  = IICONACK;                //resumes IIC operation.
                Delayus(1);                       //wait until stop condtion is in effect.
                       //The pending bit will not be set after issuing stop condition.
				goto failure;
                break;    
            }
            i2c->IICDS = _iicData[_iicPt++];        //_iicData[0] has dummy.
            for(i=0;i<300;i++);                  //for setup time until rising edge of IICSCL
            i2c->IICCON = IICONACK;                     //resumes IIC operation.
            break;

        case SETRDADDR:
//          Uart_Printf("[S%d]",_iicDataCount);
            if((_iicDataCount--)==0)
            {
				goto failure;
                break;                  //IIC operation is stopped because of IICCON[4]    
            }
            i2c->IICDS = _iicData[_iicPt++];
            for(i=0;i<100;i++);          //for setup time until rising edge of IICSCL
           i2c->IICCON = IICONACK;             //resumes IIC operation.
            break;

        default:
            break;      
    }
	
	return 1;

failure:
	return 0;

}    

int Run_IICPoll(void)
{
  volatile S3C6410_IIC_REG * i2c = (S3C6410_IIC_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_IICBUS, FALSE);
  if((i2c->IICCON)& 0x10)                  //Tx/Rx Interrupt Enable
       		return IICPoll();
   
  return 0;
} 

int IICWritebyte(unsigned char iccaddr,unsigned char regaddr,unsigned char value)
{
    DWORD dwCount =0;   
     volatile S3C6410_IIC_REG * i2c = (S3C6410_IIC_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_IICBUS, FALSE);
  
    i2c->IICADD  = 0x10;                    //6410 slave address = [7:1]
    i2c->IICSTAT = 0x10;                    //IIC bus data output enable(Rx/Tx) 
 
    _iicMode      = WRDATA;
    _iicPt        = 0;
    _iicData[0]   = regaddr;
    _iicData[1]   = value;
    _iicDataCount = 2; 
  	//RETAILMSG(1,
	 //  (TEXT("IIC IICWritebyte A\r\n")));  
 
    i2c->IICDS   = iccaddr;                 
    i2c->IICSTAT = 0xf0;                    //MasTx,Start
    dwtimeout =0;
    while(_iicDataCount!=-1)
    {
		if(Run_IICPoll())
			Delayus(10);
		else
		  return 0;
     }
    Delay();
    //EdbgOutputDebugString ("Write Register end\r\n");
    return 1;

}


void InitMegaDisplay(unsigned char  device_addr) 
{

 IICWritebyte(device_addr,  0x02, 0x01 );
 IICWritebyte(device_addr,  0x02, 0x03 );
 IICWritebyte(device_addr,  0x07, 0x1B );
 IICWritebyte(device_addr,  0x08, 0x08 );
 IICWritebyte(device_addr,  0x09, 0x80 );
 IICWritebyte(device_addr,  0x0D, 0x08 );
 IICWritebyte(device_addr,  0x0F, 0x12 );
 IICWritebyte(device_addr,  0x10, 0x80 );
}





int probe_megadisplay( int device_addr)
{
	int j;
	EdbgOutputDebugString("Check for MegaDisplay\r\n");
	IICInit();
	if(IICWritebyte(device_addr,  0x02, 0x01 ) == 1)
	{
		InitMegaDisplay(60);
		return 1;
	}
	return 0;
}


