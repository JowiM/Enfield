#include "contiki.h"
#include "groot-sink.h"
#include <stdio.h>
#if CONTIKI_TARGET_ORISENPRIME
	#include "button-sensors.h"
#else
	#include "dev/button-sensor.h"
#endif

/**
 * @brief Assuming that sink does not sample!
 * @details The following method shows that the sink does not support any sensing capabilites. 
 *          This assumption will be used through out the project due to restrictions of time. In
 *          future it can easily be added
 * 
 */
struct GROOT_SENSORS sensor_support = {.co2 = 0, .no = 0, .temp = 0, .humidity = 0};
static uint16_t query_id = 0;
//Initialize Process information
/*---------------------------------------------*/
PROCESS(enfield_sink, "Enfield Sink");
AUTOSTART_PROCESSES(&enfield_sink);
/*---------------------------------------------*/

PROCESS_THREAD(enfield_sink, ev, data){
	PROCESS_EXITHANDLER(;)

	PROCESS_BEGIN();
	

	static uint16_t sample_rate = 1*CLOCK_SECOND;
	static uint8_t aggregation = GROOT_NO_AGGREGATION;
	static struct GROOT_SENSORS data_required = {.co2 = 1, .no = 0, .temp = 1, .humidity = 0};
	static int is_subscribed;

	printf("SINK!! \n");
	
	sink_bootstrap(&sensor_support);

	SENSORS_ACTIVATE(button_sensor);
	while(1){

		PROCESS_WAIT_EVENT_UNTIL(ev == sensors_event &&
									data == &button_sensor);
		printf("PRESSED BUTTON!! \n");
		//Increment Query ID
		query_id += 1;
		is_subscribed = sink_subscribe(query_id, sample_rate, &data_required, aggregation);
		//is_subscribed = sink_unsubscribe(4);
	}

	PROCESS_END();
}