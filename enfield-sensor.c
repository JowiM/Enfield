#include "contiki.h"
#include "groot-sensor.h"
#include <stdio.h>
#if CONTIKI_TARGET_ORISENPRIME
	#include "button-sensors.h"
#else
	#include "dev/button-sensor.h"
#endif

struct GROOT_SENSORS sensor_support = {.co2 = 1, .no = 1, .temp = 1, .humidity = 1};

//Initialize Process information
/*---------------------------------------------*/
PROCESS(enfield_sensor, "Enfield Sensor");
AUTOSTART_PROCESSES(&enfield_sensor);
/*---------------------------------------------*/

PROCESS_THREAD(enfield_sensor, ev, data){
	PROCESS_EXITHANDLER(;)

	PROCESS_BEGIN();
	
	printf("SENSOR!!\n");

	sensor_bootstrap(&sensor_support);

	SENSORS_ACTIVATE(button_sensor);
	while(1){

		PROCESS_WAIT_EVENT_UNTIL(ev == sensors_event &&
									data == &button_sensor);
		printf("TEST \n");
	}

	PROCESS_END();
}