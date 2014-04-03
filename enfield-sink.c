#include "contiki.h"
#include "groot-sink.h"

//Initialize Process information
/*---------------------------------------------*/
PROCESS(enfield_sink, "Enfield Sink");
AUTOSTART_PROCESS(&enfield_sink);
/*---------------------------------------------*/

PROCESS_THREAD(enfield_sink, ev, data){
	PROCESS_BEGIN();

	PROCESS_END();
}