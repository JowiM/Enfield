#include "contiki.h"
#include "groot-sensor.h"
#include <stdio.h>
#include "net/rime.h"

struct GROOT_CHANNELS sensor_chan;
/*------------------------------ Callbacks ---------------------------------*/
static void
recv_routing(struct broadcast_conn *c, const rimeaddr_t *from){
	printf("Recevied Query!!");

	//Handle new query
	int is_success = groot_rcv(from);
	if(!is_success){
		return;
	}

	//Do not own the query - Start callback timer and spread the love
}

static void 
recv_runic(struct runicast_conn *c, const rimeaddr_t *from, uint8_t seqno){
	//Handle new query
	int is_success = groot_rcv(from);
	if(!is_success){
		return;
	}
}

static void 
sent_runic(struct runicast_conn *c, const rimeaddr_t *from, uint8_t retransmissions){

}

static void 
timedout_runic(struct runicast_conn *c, const rimeaddr_t *from, uint8_t retransmissions){

}

static const struct broadcast_callbacks sensor_routing_bcast = {recv_routing};
static const struct runicast_callbacks sensor_data_rcast = {
	recv_runic,
	sent_runic,
	timedout_runic
};

/*------------------------------ Main Functions ---------------------------*/
void sensor_bootstrap(struct GROOT_SENSORS *support){	
	printf("Sensor Starting.....\n");
	//Open Channels needed
	broadcast_open(&sensor_chan.bc, GROOT_ROUTING_CHANNEL, &sensor_routing_bcast);
	runicast_open(&sensor_chan.rc, GROOT_DATA_CHANNEL, &sensor_data_rcast);
	
	//Initialize protocol library
	groot_prot_init(support, &sensor_chan);

	//@todo check if any neighbours has any queries good for me!!
}

int
sensor_destroy(){
	broadcast_close(&sensor_chan.bc);
	runicast_close(&sensor_chan.rc);
}