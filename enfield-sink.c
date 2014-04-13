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
static struct GROOT_SENSORS sensor_support = {0, 0, 0, 0};
static uint16_t query_id = 0;
static uint8_t numb_clicks = 0;
//Initialize Process information
/*---------------------------------------------*/
PROCESS(enfield_sink, "Enfield Sink");
AUTOSTART_PROCESSES(&enfield_sink);
/*---------------------------------------------*/

PROCESS_THREAD(enfield_sink, ev, data){
	PROCESS_EXITHANDLER(;)
	PROCESS_BEGIN();
	
	static uint16_t sample_rate = 13*CLOCK_SECOND;
	static uint8_t aggregation = GROOT_MAX;
	static struct GROOT_SENSORS data_required = {1, 0, 1, 0};
	static int is_subscribed;

	printf("SINK!! \n");
	
	sink_bootstrap(&sensor_support);

	SENSORS_ACTIVATE(button_sensor);
	while(1){

		PROCESS_WAIT_EVENT_UNTIL(ev == sensors_event &&
									data == &button_sensor);
		numb_clicks += 1;
		if(numb_clicks == 6){
			printf("UNSUBSCRIBE CLICK \n");
			is_subscribed = sink_unsubscribe(4);
		} else if(numb_clicks == 7){
			printf("UPDATE QUERY CLICK \n");
			data_required.no = 1;
			is_subscribed = sink_send(1, 30*CLOCK_SECOND, &data_required, GROOT_MAX);
		} else {
			printf("PRESSED BUTTON!! \n");
			//Increment Query ID
			query_id += 1;
			is_subscribed = sink_subscribe(query_id, sample_rate, &data_required, aggregation);
		}
	}

	PROCESS_END();
}