/******************* (c) Marvell Semiconductor, Inc., 2004 ********************
 *
 *
 *  Purpose:    Contains commonalized error codes for networking.
 *
 *
 *	$Author: schiu $
 *
 *	$Date: 2004/08/23 $
 *
 *	$Revision: #2 $
 *
 *****************************************************************************/

#ifndef IX_STAUTS_H
#define IX_STATUS_H


/* Status/ error codes returned by the abstracted interfaces */
typedef enum 
{
    IX_STATUS_SUCCESS = 0,
    IX_STATUS_DEVICE_BUSY,				// Device is busy... must wait.
    IX_STATUS_NO_RESP,					// No response is waiting from the device.
    IX_STATUS_NO_DATA,					// No data available at the device.
    IX_STATUS_RESOURCES,            	// Out of whatever resource is being asked for
    IX_STATUS_PENDING,              	// Unable to complete immediately
    IX_STATUS_FAILURE,              	// Function call unsuccessful
    IX_STATUS_INVALID_ARG,          	// Invalid argument passed by the caller
    IX_STATUS_ERROR_UNKNOWN,        	// Unknown system error occurred
    IX_STATUS_HWINIT_FAIL,          	// Hardware failed to initialize
    IX_STATUS_BOOT_READY,               // boot loader ready
	IX_STATUS_HOST_BOOT_READY,

    /* Add new entries above this */
	IX_STATUS_INVALID_STATUS_VALUE		// Anything equal or above this is invalid status.
} IX_STATUS;

#define IX_SUCCESS(stat) ((stat)==IX_STATUS_SUCCESS)
#endif  /* IX_STATUS_H */

