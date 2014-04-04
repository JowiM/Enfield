#include "contiki.h"
#include "groot-sensor.h"
#include <stdio.h>

GROOT_TOC sensor_support = {.co2 = 1, .no = 1, .temp = 1, .humidity = 1};

//Initialize Process information
/*---------------------------------------------*/
PROCESS(enfield_sensor, "Enfield Sensor");
AUTOSTART_PROCESSES(&enfield_sensor);
/*---------------------------------------------*/

PROCESS_THREAD(enfield_sensor, ev, data){
	PROCESS_BEGIN();
	PROCESS_EXITHANDLER(sensor_destroy());
	printf("SENSOR!!\n");

	sensor_bootstrap(&sensor_support);

	PROCESS_END();
}