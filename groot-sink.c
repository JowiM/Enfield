#include "groot-sink.h"
#include "contiki.h"
#include <stdio.h>

static GROOT_CHANNELS sink_chan;
static uint16_t query_id = 0;

/*------------------------------ Callbacks ---------------------------------*/
static void
recv_data(struct broadcast_conn *c, const rimeaddr_t *from){
	printf("Recevied Query!!");
}

static void
recv_routing(struct broadcast_conn *c, const rimeaddr_t *from){
	printf("Received Published data!!");
}

static const struct broadcast_callbacks sink_data_bcast = {recv_data};
static const struct broadcast_callbacks sink_routing_bcast = {recv_routing};

/*------------------------------- Main Function -----------------------------*/
void
sink_bootstrap(GROOT_SENSORS *supported_sensors){
	printf("Sensor Starting.....\n");
	//Open Channels needed
	broadcast_open(&sink_chan.dc, GROOT_DATA_CHANNEL, &sink_data_bcast);
	broadcast_open(&sink_chan.rc, GROOT_ROUTING_CHANNEL, &sink_routing_bcast);
}

void
sink_destroy(){
	broadcast_close(&sink_chan.dc);
	broadcast_close(&sink_chan.rc);
}

int
sink_subscribe(uint16_t sample_rate, GROOT_SENSORS *data_required, uint8_t aggregation){
	//Start Subscribtion phase
	groot_subscribe_snd(&sink_chan.dc, query_id, sample_rate, data_required, aggregation);
}

int
sink_unsubscribe(){

}

int
sink_send(){

}