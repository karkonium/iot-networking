#include <stdio.h>
#include "controller.h"


/* Checks if the message has a valid type. The gateway should only receive
 * HANDSHAKE or UPDATE messages from sensors
 */
int is_valid_type(struct cignal *cig) {

    if ((cig->hdr.type == HANDSHAKE) || (cig->hdr.type == UPDATE)) {
        return 1;
    }
    return -1;
}

/* Returns 1 if the gateway seen this device before
 */
int is_registered(int id, int *device_record) {

    if (device_record[id - LOWEST_ID] == 1) {
        return 1;
    }
    return -1;
}

/* Add a new device to the device_record.  Return the new device id.
 * Note that device ids will never be "de-registered" so they cannot be reused.
 */
int register_device(int *device_record) {

    for (int i = 0; i < MAXDEV; i++) {
        if (device_record[i] == 0) {
            device_record[i] = 1;
            return i + LOWEST_ID;
        }
    }
    return -1;
}

/* Turns on or off the cooler or dehumidifier based on the
 * current temperature or humidity.j
 */
void adjust_fan(struct cignal *cig) {

    cig->hdr.type = 3;
    if (cig->hdr.device_type == TEMPERATURE) {
        if (cig->value >= TEMPSET) {
            cig->cooler = ON;
        } else {
            cig->cooler = OFF;
        }	
    } else if (cig->hdr.device_type == HUMIDITY) {
        if (cig->value >= HUMSET) {
            cig->dehumid = ON;
        } else {
            cig->dehumid = OFF;
        }
    }
}


/* Function checks each field of the incoming header to ensure that it is valid
 * and adjust the fan.
 */
int process_message(struct cignal *cig, int *device_record) {

    if (is_valid_type(cig) != -1) {

		// process handshakes and updates differently
		if (cig->hdr.device_id == -1 && cig->hdr.type == HANDSHAKE) {

			// so this is a handshake signal
			// device isnt register -- we must register it
			cig->hdr.device_id = register_device(device_record);

		} else if (is_registered(cig->hdr.device_id, device_record) && cig->hdr.type == UPDATE){
            
			// so its not a handshake signal -- therefore update signal
			if (cig->hdr.device_type == HUMIDITY) {
				printf("Humidity: %.4f --> Device_ID: %d\n", cig->value, cig->hdr.device_id);
			} else if (cig->hdr.device_type == TEMPERATURE){
				printf("Temperature: %.4f --> Device_ID: %d\n", cig->value,  cig->hdr.device_id);
			} else {
				fprintf(stderr, "Received corrupted cignal! The message is discarded...\n");
				return -1;
			}

			// for updates, update the cig with the stuff about adjust_fan
			adjust_fan(cig);
		} else {
			fprintf(stderr, "Received corrupted cignal! The message is discarded...\n");
			return -1;
		}
		printf("********************END EVENT********************\n\n");
		return 0;
	}
    
    fprintf(stderr, "Received corrupted cignal! The message is discarded...\n");
    return -1;
}
