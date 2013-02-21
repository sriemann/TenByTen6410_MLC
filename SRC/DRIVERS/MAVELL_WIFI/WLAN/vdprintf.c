// vdPRINTF.C
//
#include "precomp.h"
#include <stdarg.h>
#include <conio.h>

unsigned int PmonTimedOut = FALSE;

#ifndef word
#define word unsigned short
#endif
#ifndef dword
#define dword unsigned long
#endif 
#define DIRECT_TO_PARALLEL      // hard coded output to parrallel port
//#define DIRECT_TO_COLOR

#if defined(DEBUG) || DBG==1
int Init = 0;
#define CRonLF

void lcputhex( long x, int shift );
void lcputdec( unsigned x );
void _cdecl dputs( char *str );
void _cdecl dputc( int ch );

#ifdef DIRECT_TO_PARALLEL
#pragma intrinsic( _outp )            
#pragma intrinsic( _inp )            
#define inp _inp
#define outp _outp
#define DEBUG_LPT_PORT  1
#define PBASE               0x3bc
//#define PBASE               0x378
//#define PBASE               0x278
#endif 

long locked = 0;

void dprintf(char *fmt, ...)
{
    char *ptr;
    va_list argp;
    unsigned tmp;

    if ( locked ) return;               // If locked, then loose debug till current thread is done.
    
    NdisInterlockedIncrement(&locked);      // take the lock.
    Init = 0;                           // reinit every time
    va_start(argp,fmt);

    ptr = fmt;                          //point to first char
    while( *ptr )                       //as long as we are not nulled
    {
        if ( *ptr == '%' )              //format char?
        {
            ptr++;                      //get to next char
            if ( *ptr == 'l' )          //double word ?
            {
                ptr++;                  // bump to next
                if ( *ptr == 'x' )
                {
                    lcputhex(va_arg(argp,long),28 );
                } 
                else if ( *ptr == 'd' || *ptr == 'u' )
                {
                    lcputdec(va_arg(argp,long));
                } else
                    continue;
            } else if ( *ptr == 'x' )   //q. use wants hex
            {
                lcputhex( (int)va_arg(argp,int),12 );            //send it as is
            } else if ( *ptr == 'p' )     //IO PORT (NEW ONE !!!!)
            {
                tmp =  va_arg(argp,unsigned); // get offset and save
                tmp &= 0x0fff;              // three bytes only
                lcputhex( tmp,8 );          // show offset
            } else if ( *ptr == 'v' )       // version, NEW ONE
            {
                tmp = va_arg(argp,unsigned short);    // get the whole thing
                lcputhex( tmp>>8,4);  // high word (bits+4)
                dputc('.');               // delimitor
                lcputhex( tmp&0x00ff,4);  // low word (bits+4)
            } else if ( *ptr == 'd'|| *ptr == 'u' )   //user wants integer decimal
            {
                lcputdec( (long)va_arg(argp,unsigned short) );            //send as is
            } else if ( *ptr == 's' )
            {
                dputs( va_arg(argp,char *) );
            } else if ( *ptr == 'c' )
            {
                dputc( va_arg(argp, char) );
            } else if ( *ptr == 'b' )
            {
                lcputhex( va_arg(argp,char),4 );
            }else if ( *ptr == 'g' )
            {
                int i;
                char *tmp = va_arg(argp, char *);
                lcputhex( (long)*((long *)tmp), 28);
                dputc(' ');
                tmp+=4;
                lcputhex( (unsigned short)*((unsigned short*)tmp), 12);
                dputc(' ');
                tmp+=2;
                lcputhex( (unsigned short)*((unsigned short*)tmp), 12);
                dputc(' ');
                tmp+=2;
                for(i=0;i<8;i++,tmp++)
                {
                    lcputhex( *tmp,4);
                    dputc(' ');
                }    
            } else
            {
                dputc(*ptr);
            }
        } else
        {
            dputc(*ptr);

#ifdef CRonLF
            if ( *ptr == '\n' )
                dputc('\r');
#endif
        }
        ptr++;
    }

    NdisInterlockedDecrement(&locked);      // take the lock.
}

void _cdecl dputs( char *str )
{
    while( *str )
    {
#ifdef CRonLF
        if ( *str == '\n')              // if character is new line
            dputc('\r');                // send a carridge return first
#endif CRonLF // end of Carridge return on Line Feed
        dputc( *str++ );
    }
            
}
void lcputhex( long hex, int shift )
{
    int val;
    for(;shift>=0;shift-=4)
    {
        val = (hex >> shift)&0x0f;      //get nibble shifted down
        if ( val < 10 )
            val += '0';                 //add offset to ascii zero
        else
            val += ('A'-10);            //add offset to get to ascii 'A'
        dputc((char)val);                      //output
    }
}
unsigned divisors[] ={ 1000000000,
                        100000000,
                         10000000,
                          1000000,
                           100000,
                            10000,
                             1000,
                              100,
                               10,
                                1,0 }; // zero terminates the list

void lcputdec( unsigned hex )
{
    unsigned i,leadzero = 0;
    unsigned digit;
    for(i=0;divisors[i];i++)
    {
        digit = hex / divisors[i];      // get the digit
        hex = hex % divisors[i];      // fix up remainder
        if ( digit )
        {
            dputc((char)(digit + '0'));
            leadzero = 1;
        } else if ( leadzero )
            dputc('0');
    }
    if (!leadzero)
        dputc('0');
}
void hd( unsigned char *hexdata, int count )
{
  int bytes=0;                              // bytes per line
  unsigned char *tmp;                        // tmp pointer

  if ( count > 256 )
    return;


  count = ( count + 16 ) & 0xfff0;           // round up to 8 words

  while( count ){                           // still got bytes left
    dprintf("\r\n%lx: ",hexdata);// print address of buffer
    tmp = hexdata;                          // get a copy of pointer
    for(bytes=0;bytes<16;bytes++)            //do a line
        dprintf("%b ",*tmp++);              //print hex value
    count -= 16;                             //8 words gone by
    hexdata += 16;                           //next paragraph
  }
    dprintf("\r\n");
  return;
}

__inline int _cdecl stillDead ( )
{
   if ( PmonTimedOut == 1 ) 
   {
      if ( (inp(PBASE+1) & 0x40) == 0) // see if ACK is negated
        return TRUE;      
   }
   if ( PmonTimedOut == 2 )
   {
      if ( (inp(PBASE+1) & 0x40) != 0) // see if ACK is asserted
        return TRUE;      
   }
   return FALSE;
}
    
#ifdef DIRECT_TO_PARALLEL
void _cdecl dputc( int ch )
{
    int cnt;
//    if ( PmonTimedOut ) return; 

    cnt = 500000;
    while( (inp(PBASE+1) & 0x40) == 0)  // wait for ACK to be negated
    {
        if ( --cnt == 0 )
        {
            PmonTimedOut++;             // dying
        }
    }

    outp(PBASE,(char)ch);               // put char in output latch

    outp(PBASE+2,0x01);                 // strobe character
    cnt = 500000;
    while( (inp(PBASE+1) & 0x40) != 0)  // wait for ACK to be asserted
    {
        if ( --cnt == 0 )
        {
            PmonTimedOut++;             // Dead waiting for != 0
        }
    }
    outp(PBASE+2,0x00);                 // negate strobe
}

#endif // end of direct_to_parallel



#else // not in DEBUG
void  dprintf( char * fmt,... )
{
;
}
void  dputc( int ch )
{
;
}
void  dputs( char *str )
{
;
}
#endif
