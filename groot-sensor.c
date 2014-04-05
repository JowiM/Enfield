#include "groot-sensor.h"
#include "contiki.h"
#include <stdio.h>

static GROOT_CHANNELS sensor_chan;
static GROOT_QUERY_RW *first_query = NULL;
static GROOT_SRT_RW *tree_row = NULL;

/*------------------------------ Callbacks ---------------------------------*/
static void
recv_data(struct broadcast_conn *c, const rimeaddr_t *from){
	printf("Recevied Query!!");

	GROOT_HEADER *hdr;
	hdr = (GROOT_HEADER*) packetbuf_dataptr();
	printf("Protocol Version: %d \n", hdr->protocol.version);
	
	//Handle new query
	GROOT_QUERY_RW *new_query;
	new_query = groot_subscribe_rcv(first_query, c, from);

	if(new_query == NULL){
		return;
	}

	//Do not own the query - Start callback timer and spread the love
}

static void
recv_routing(struct broadcast_conn *c, const rimeaddr_t *from){
	printf("Received Published data!!");
}

static const struct broadcast_callbacks sensor_data_bcast = {recv_data};
static const struct broadcast_callbacks sensor_routing_bcast = {recv_routing};

/*------------------------------ Main Functions ---------------------------*/
void sensor_bootstrap(GROOT_SENSORS *support){
	printf("Sensor Starting.....\n");
	
	//Open Channels needed
	broadcast_open(&sensor_chan.dc, GROOT_DATA_CHANNEL, &sensor_data_bcast);
	broadcast_open(&sensor_chan.rc, GROOT_ROUTING_CHANNEL, &sensor_routing_bcast);
	
	//@todo check if any neighbours has any queries good for me!!
}

int
sensor_publish(){

}

int
sensor_destroy(){
	broadcast_close(&sensor_chan.dc);
	broadcast_close(&sensor_chan.rc);
}