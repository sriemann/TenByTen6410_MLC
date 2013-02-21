#if !defined(EXTPOWERCTL_H)
#define EXTPOWERCTL_H
#include <bsp.h>

typedef enum power_ctl
{
    USB=0,
    LCD=1,
    DAC0=2,
    DAC1=3,
    ETH=4,
    WIFI=5,
    CAM=6
} PWRCTL_POWERCTL;

typedef enum active_highlow_t
{
    LOW=0,
    HIGH=1,
} PWRCTL_HIGH_LOW;

#define max_devices 7

typedef struct power_module_t
{
    char *   name;
    PWRCTL_HIGH_LOW active;
    
} PWRCTL_POWER_MODULES;


#define PWRCTL_DEFAULT_CONFIG 2
#define PWRCTL_CUSTOM_CONFIG 3


void PWRCTL_setUSBPower(int state);
void PWRCTL_setLCDPower(int state);
void PWRCTL_setDAC1Power(int state);
void PWRCTL_setDAC0Power(int state);
void PWRCTL_setETHPower(int state);
void PWRCTL_setCAMPower(int state);
void PWRCTL_setWiFiPower(int state);

// used by eboot and driver
void PWRCTL_InitializePowerCTL(DWORD *power,volatile S3C6410_GPIO_REG *regs);
void PWRCTL_setAllTo(DWORD *power);
char *PWRCTL_isDefault(DWORD power);
DWORD PWRCTL_getStatus(DWORD *power, PWRCTL_POWERCTL bit);
char *PWRCTL_getStatusOnOff(DWORD power, PWRCTL_POWERCTL bit);
char *PWRCTL_getPowerModuleName(unsigned int n);
void PWRCTL_setDefaultPower(DWORD *power);
void PWRCTL_togglePower(DWORD *power, PWRCTL_POWERCTL bit);
void PWRCTL_setPower(DWORD *power, PWRCTL_POWERCTL bit);
void PWRCTL_clrPower(DWORD* power, PWRCTL_POWERCTL bit);
DWORD PWRCTL_getCurrentPower();
void PWRCTL_powerCTLInit(volatile S3C6410_GPIO_REG *regs);
void PWRCTL_deinit();
void PWRCTL_Sleep();
void PWRCTL_Awake();
#endif