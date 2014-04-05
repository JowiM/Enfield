#include "contiki.h"
#include "groot.h"
#include <stdio.h>

#define PRINT2ADDR(addr) printf("%02x%02x:%02x%02x",(addr)->u8[3], (addr)->u8[2], (addr)->u8[1], (addr)->u8[0])

static void
print_raw_packetbuf(void){
  uint16_t i;
  for(i = 0; i < packetbuf_hdrlen(); i++) {
    printf("%02x ", *(char *)(packetbuf_hdrptr() + i));
  }
  printf("| ");
  for(i = 0; i < packetbuf_datalen(); i++) {
    printf("%02x ", *(char *)(packetbuf_dataptr() + i));
  }
  printf("[%d]\n", (int16_t) (packetbuf_dataptr() - packetbuf_hdrptr()));
}

static void
print_hdr(GROOT_HEADER *hdr){
	printf("!------ PRINTING:");
	PRINT2ADDR(&rimeaddr_node_addr);
	printf(" ------! \n");
	printf("{ Source: ");
	PRINT2ADDR(&hdr->esender);
	printf(" Version: %d MAGIC: %c%c ", hdr->protocol.version, hdr->protocol.magic[0], hdr->protocol.magic[1]);
	printf("Type: %02x Query ID: %d Sample ID: %d Sample Rate: %d Aggregator: %d }", hdr->type, hdr->query_id, hdr->sample_id ,hdr->sample_rate, hdr->aggregator);
}

static uint8_t
is_groot_protocol(GROOT_HEADER *hdr){
	if(hdr->protocol.version != GROOT_VERSION){
		return 0;
	}
	if(hdr->protocol.magic[0] != 'G' || hdr->protocol.magic[1] != 'T'){
		return 0;
	}

	return 1;
}

static void
packet_loader_qry(GROOT_HEADER *hdr){
	packetbuf_clear();
	packetbuf_copyfrom(hdr, sizeof(GROOT_HEADER));
}
/*--------------------------------------------- Main Methods ------------------------------------------------------------*/
int
groot_subscribe_snd(struct broadcast_conn *dc, uint16_t query_id, uint16_t sample_rate, GROOT_SENSORS *data_required, uint8_t aggregator){
	GROOT_HEADER hdr;

	//Initialise query header
	hdr.protocol.version = GROOT_VERSION;
	hdr.protocol.magic[0] = 'G';
	hdr.protocol.magic[1] = 'T';
	
	//Main Variables
	hdr.type = GROOT_SUBSCRIBE_TYPE;
	hdr.query_id = query_id;
	hdr.sample_id = 0;
	hdr.sample_rate = sample_rate;
	hdr.aggregator = aggregator;
	//@todo find better way
	hdr.sensors_required.co2 = data_required->co2;
	hdr.sensors_required.no = data_required->no;
	hdr.sensors_required.temp = data_required->temp;
	hdr.sensors_required.humidity = data_required->humidity;

	//Current Address
	rimeaddr_copy(&hdr.esender, &rimeaddr_node_addr);

	//Create Query Packet
	packet_loader_qry(&hdr);
	//Send packet
	broadcast_send(dc);
	return 0;
}

GROOT_QUERY_RW
*groot_subscribe_rcv(GROOT_QUERY_RW *first_query, struct broadcast_conn *c, const rimeaddr_t *from){
	GROOT_QUERY_RW *rw = NULL;
	GROOT_HEADER *hdr;
	uint8_t type;

	hdr = (GROOT_HEADER*) packetbuf_dataptr();
	print_hdr(hdr);
	//Is not correct protocol
	if(!is_groot_protocol(hdr)){
		return rw;
	}

	type = hdr->type;
	printf("TYpe: %02x - %02x\n", type, GROOT_SUBSCRIBE_TYPE);
	switch(type){
		case GROOT_SUBSCRIBE_TYPE:
			printf("SUBSCRIBING!! \n");
			break;
		case GROOT_UNSUBSCRIBE_TYPE:
			printf("UNSUBSRIBING!! \n");
			break;
		case GROOT_NEW_MOTE_TYPE:
			printf("Im a new fella!! \n");
			break;
		case GROOT_PUBLISH_TYPE:
			printf("Im publishing!! \n");
			break;
		default:
			return rw;
	}

	return rw;
}

void
groot_intent_snd(){

}

void
groot_intent_rcv(){

}

void
groot_unsubscribe_snd(){

}

void
groot_unsubscribe_rcv(){

}

void
groot_alternate_snd(){

}

void
groot_alternate_rcv(){

}

void
groot_publish_rcv(){

}

void
groot_publish_snd(){

}