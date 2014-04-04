#include "groot-sink.h"
#include "contiki.h"
#include <stdio.h>

static GROOT_CHANNELS sink_chan;

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
void sink_bootrstrap(){
	printf("Sensor Starting.....\n");
	//Open Channels needed
	broadcast_open(&sink_chan.dc, GROOT_DATA_CHANNEL, &sink_data_bcast);
	broadcast_open(&sink_chan.rc, GROOT_ROUTING_CHANNEL, &sink_routing_bcast);
	//@todo check if any neighbours has any queries good for me!!
}

int sink_subscribe(){

}

int sink_unsubscribe(){

}

int sink_send(){

}