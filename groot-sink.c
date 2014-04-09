#include "groot-sink.h"
#include "contiki.h"
#include <stdio.h>
#include "net/rime.h"

static struct GROOT_CHANNELS sink_chan;
/*------------------------------ Callbacks ---------------------------------*/

static void
recv_routing(struct broadcast_conn *c, const rimeaddr_t *from){
	printf("Received Published data!!");
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

static const struct broadcast_callbacks sink_routing_bcast = {recv_routing};
static const struct runicast_callbacks sink_data_rcast = {
	recv_runic,
	sent_runic,
	timedout_runic
};

/*------------------------------- Main Function -----------------------------*/
void
sink_bootstrap(struct GROOT_SENSORS *supported_sensors){
	printf("Sink Starting.....\n");
	//Open Channels needed
	broadcast_open(&sink_chan.bc, GROOT_ROUTING_CHANNEL, &sink_routing_bcast);
	runicast_open(&sink_chan.rc, GROOT_DATA_CHANNEL, &sink_data_rcast);
	//Initialize protocol library
	groot_prot_init(supported_sensors, &sink_chan);
}

void
sink_destroy(){
	broadcast_close(&sink_chan.bc);
	runicast_close(&sink_chan.rc);
}

int
sink_subscribe(uint16_t query_id, uint16_t sample_rate, struct GROOT_SENSORS *data_required, uint8_t aggregation){
	//Send Subscribtion
	groot_subscribe_snd(query_id, sample_rate, data_required, aggregation);
}

int
sink_unsubscribe(uint16_t query_id){
	groot_unsubscribe_snd(query_id);
}

int
sink_send(){

}