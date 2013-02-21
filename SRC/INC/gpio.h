#ifndef __GPIO_H__
#define __GPIO_H__

typedef enum GPIO_PORT {
	PORT_A, PORT_B, PORT_C, PORT_D, PORT_E, PORT_F, PORT_G, PORT_H, PORT_J
} GPIO_PORT, *PGPIO_PORT;

typedef enum GPIO_PIN_CONFIGURATION {
	OUTPUT, INPUT_WITH_PULLUP, INPUT_WITHOUT_PULLUP
} GPIO_PIN_CONFIGURATION, *PGPIO_PIN_CONFIGURATION;

typedef enum GPIO_PIN_VALUE {
	ON, OFF
} GPIO_PIN_VALUE, *PGPIO_PIN_VALUE;

//
// I/O DESCRIPTOR for pin configuration
//
typedef struct GPIO_SET_PIN_CONFIGURATION {
	GPIO_PORT								portNumber;				// number of the port (PORT_A to PORT_J)
	UCHAR										pinNumber;				// number of the pin in the port
	GPIO_PIN_CONFIGURATION	pinConfiguration;	// configuration of the pin
	GPIO_PIN_VALUE					pinValue;					// value of the pin after configuration (only used if pin configured as output)
} GPIO_SET_PIN_CONFIGURATION, *PGPIO_SET_PIN_CONFIGURATION;

//
// I/O DESCRIPTOR for pin set value (for pins configured as output)
//
typedef struct GPIO_SET_PIN_OUTPUT_VALUE {
	GPIO_PORT								portNumber;				// number of the port (PORT_A to PORT_J)
	UCHAR										pinNumber;				// number of the pin in the port
	GPIO_PIN_VALUE					pinValue;					// value of the pin after configuration (only used if pin configured as output)
} GPIO_SET_PIN_OUTPUT_VALUE, *PGPIO_SET_PIN_OUTPUT_VALUE;

//
// I/O DESCRIPTOR for pin get value (for pins configured as input)
//
typedef struct GPIO_GET_PIN_INPUT_VALUE {
	GPIO_PORT								portNumber;				// number of the port (PORT_A to PORT_J)
	UCHAR										pinNumber;				// number of the pin in the port
	GPIO_PIN_VALUE					pinValue;					// value of the pin after configuration 
} GPIO_GET_PIN_INPUT_VALUE, *PGPIO_GET_PIN_INPUT_VALUE;

//
// SPI Bus Driver IOCTLS
//
#define FILE_DEVICE_GPIO     FILE_DEVICE_CONTROLLER

// IN:  PSPI_IO_DESC
#define IOCTL_GPIO_SET_PIN_CONFIGURATION	CTL_CODE(FILE_DEVICE_GPIO, 0, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_GPIO_SET_PIN_OUTPUT_VALUE 	CTL_CODE(FILE_DEVICE_GPIO, 1, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_GPIO_GET_PIN_INPUT_VALUE 		CTL_CODE(FILE_DEVICE_GPIO, 2, METHOD_BUFFERED, FILE_ANY_ACCESS)


#endif // __GPIO_H__
