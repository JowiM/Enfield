#include "contiki.h"
#include "groot-sink.h"
#include <stdio.h>
//Initialize Process information
/*---------------------------------------------*/
PROCESS(enfield_sink, "Enfield Sink");
AUTOSTART_PROCESSES(&enfield_sink);
/*---------------------------------------------*/

PROCESS_THREAD(enfield_sink, ev, data){
	PROCESS_BEGIN();

	printf("SINK!! \n");

	PROCESS_END();
}