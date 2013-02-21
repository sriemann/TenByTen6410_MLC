#if !defined(EXTPOWERCTL_DRV_H)
#define EXTPOWERCTL_DRV_H

typedef enum
{
	LCD_ON_SET,
    USB_ON_SET,
    CAMERA_ON_SET,
    DAC0_ON_SET,
    DAC1_ON_SET,
	ETH_ON_SET,
	WIFI_ON_SET,
	LCD_OFF_SET,
    USB_OFF_SET,
    CAMERA_OFF_SET,
    DAC0_OFF_SET,
    DAC1_OFF_SET,
	ETH_OFF_SET,
	WIFI_OFF_SET,
	ALL_ON_SET,
	ALL_OFF_SET,
	AWAKE_SET,
	SLEEP_SET,
    EPCTL_MAX
} EPCTL_SET;

typedef enum
{
    EPCTL_SUCCESS,  
    EPCTL_ERROR_XXX
} EPCTL_ERROR;



#endif