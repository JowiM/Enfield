/**
 * @file
 * 	GROOT main code. Holds all the functionality needed to create an Ad-Hoc network.
 * @author 
 * 	Johann Mifsud <johann.mifsud.13@ucl.ac.uk>
 */

#include "contiki.h"
#include "groot.h"
#include "lib/list.h"
#include "lib/memb.h"
#include "stdio.h"
#include "string.h"

#define PRINT2ADDR(addr) printf("%02x%02x", (addr)->u8[1], (addr)->u8[0])

LIST(groot_qry_table);
MEMB(groot_qrys, struct GROOT_QUERY_ITEM, GROOT_QUERY_LIMIT);
MEMB(groot_children, struct GROOT_SRT_CHILD, GROOT_QUERY_LIMIT*(GROOT_CHILD_LIMIT+1));

static void cb_publish_aggregate(void *i);

static struct GROOT_LOCAL glocal;
/*------------------------------------------------- Debug Methods -------------------------------------------------------*/
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
print_hdr(struct GROOT_HEADER *hdr){
	printf("HEADER ");
	PRINT2ADDR(&rimeaddr_node_addr);
	printf(" - { Source: ");
	PRINT2ADDR(&hdr->ereceiver);
	printf(" Destination: ");
	PRINT2ADDR(&hdr->ereceiver);
	printf(" Version: %d MAGIC: %c%c ", hdr->protocol.version, hdr->protocol.magic[0], hdr->protocol.magic[1]);
	printf("Type: %02x Query ID: %d } \n", hdr->type, hdr->query_id);
}

static void
print_qry(struct GROOT_QUERY *qry){
	printf("QUERY ");
	PRINT2ADDR(&rimeaddr_node_addr);
	printf(" - { Sample Rate: %d Aggregator: %d } - ", qry->sample_rate, qry->aggregator);
	printf(" { CO2 - %d NO - %d TEMP - %d HUMIDITY - %d } \n", qry->sensors_required.co2, qry->sensors_required.no, qry->sensors_required.temp, qry->sensors_required.humidity);
}

static void
print_data(struct GROOT_SENSORS_DATA *data){
	printf("DATA ");
	PRINT2ADDR(&rimeaddr_node_addr);
	printf(" - { CO2 %.2f NO - %.2f TEMP - %.2f HUMIDITY - %.2f } \n", data->co2, data->no, data->temp, data->humidity);
}

static void
print_qrys(){
	struct GROOT_QUERY_ITEM *qry_itm;

	qry_itm = list_head(groot_qry_table);
	while(qry_itm != NULL){
		printf(" QUERY ITEM - [ ");
		printf(" QUERY ID: %d ESENDER: ", qry_itm->query_id);
		PRINT2ADDR(&qry_itm->ereceiver);
		printf(" Parent: ");
		PRINT2ADDR(&qry_itm->parent);
		printf(" ] \n");

		qry_itm = qry_itm->next;
	}
}

static void
print_children(struct GROOT_SRT_CHILD *first_child){
	struct GROOT_SRT_CHILD *tmp_child = first_child;

	while(tmp_child != NULL){
		printf("CHILD ");
		PRINT2ADDR(&tmp_child->address);
		printf(" {Last set: %d }\n", tmp_child->last_set);

		tmp_child = tmp_child->next;
	}
}

/*------------------------------------------------- Other Methods ----------------------------------------------------------*/
/**
 * @brief Calculate the minimum a child can be idle before removing
 * @details Calculate the minimumum a child can be idle before removing
 * 
 * @param GROOT_QUERY_ITEM A query reference
 * @param retries Number of retries before removing
 * @param leeway Random leeway
 */
static int
least_idle_time(struct GROOT_QUERY_ITEM *qry_itm, int retries, int leeway){
	return clock_seconds()-(((qry_itm->query.sample_rate/CLOCK_SECOND) + leeway) * retries);
}

/**
 * @brief Copy a query struct
 * @details Copy a query struct
 * 
 * @param GROOT_QUERY Struct reference the object will be copied to
 * @param GROOT_QUERY Struct reference the object will be copied from
 */
static void
copy_qry(struct GROOT_QUERY *target, struct GROOT_QUERY *src){
	target->sample_id = src->sample_id;
	target->sample_rate = src->sample_rate;
	target->aggregator = src->aggregator;
	memcpy(&target->sensors_required, &src->sensors_required, sizeof(struct GROOT_SENSORS));
}

/**
 * @brief Get Query from packet buffer
 * @details Get Query from packet buffer
 * @return pointer to query in packet buf
 */
static struct GROOT_QUERY
*packetbuf_get_qry(){
	return (struct GROOT_QUERY*) (packetbuf_dataptr() + sizeof(struct GROOT_HEADER));
}

/**
 * @brief Get Data from packet buffer
 * @details Get Data from packet buffer
 * @return pointer to query in packet buf
 */
static struct GROOT_SENSORS_DATA
*packetbuf_get_sensor_data(){
	return (struct GROOT_SENSORS_DATA *) (packetbuf_dataptr() + sizeof(struct GROOT_HEADER) + sizeof(struct GROOT_QUERY));
}

/**
 * @brief Randomly generate values for sensors. 0 signals empty
 * @details Since we do not know what sensors the sensor has, this method simulates results.
 * 
 * @param GROOT_SENSORS Reference to the sensors data requried
 * @param GROOT_SENSORS_DATA Where the actual data should be stored
 */
static void
random_sensor_readings(struct GROOT_SENSORS *required, struct GROOT_SENSORS_DATA *data){
	if(required->co2 == 1){
		data->co2 = (rand()%(90))+10;
	} else {
		data->co2 = 0;
	}

	if(required->no == 1){
		data->no = (rand()%(90))+10;
	} else {
		data->no = 0;
	}

	if(required->temp == 1){
		data->temp = (rand()%(90))+10;
	} else {
		data->temp = 0;
	}

	if(required->humidity == 1){
		data->humidity = (rand()%(90))+10;
	} else {
		data->humidity = 0;
	}
}

/**
 * @brief Parse through children and get child associated with address
 * @details Parse through children and get child associated with address
 * 
 * @param GROOT_SRT_CHILD pointer for start of list
 * @param address pointer to address of child needed
 * 
 * @return child poirnter or NULL
 */
static struct GROOT_SRT_CHILD
*get_child(struct GROOT_SRT_CHILD *first_child, const rimeaddr_t *address){
	struct GROOT_SRT_CHILD *tmp_child = first_child;

	while(tmp_child != NULL){
		if(rimeaddr_cmp(&tmp_child->address, address)){
			return tmp_child;
		}
		tmp_child = tmp_child->next;
	}

	return NULL;
}

/**
 * @brief Calculate list length
 * @details Calculate list length
 * 
 * @param GROOT_SRT_CHILD child beginning pointer
 */
static int
child_length(struct GROOT_SRT_CHILD *first_child){
	struct GROOT_SRT_CHILD *tmp_children = first_child;
	int numb_childs = 0;

	while(tmp_children != NULL){
		numb_childs += 1;
		tmp_children = tmp_children->next;
	}

	return numb_childs;
}

/**
 * @brief Get end of list
 * @details Get end of list
 * 
 * @param GROOT_SRT_CHILD List pointer
 * @return pointer to end of list
 */
static struct GROOT_SRT_CHILD
*child_lst_tail(struct GROOT_SRT_CHILD *first_child){
	struct GROOT_SRT_CHILD *tmp_children = first_child;

	while(tmp_children != NULL){
		if(tmp_children->next == NULL){
			return tmp_children;
		}
		tmp_children = tmp_children->next;
	}

	return NULL;
}

/**
 * @brief Add child to list
 * @details Add child to list
 * 
 * @param GROOT_SRT_CHILD Pointer to child list
 * @param GROOT_SRT_CHILD child to add
 * 
 * @return pointer to new child
 */
static struct GROOT_SRT_CHILD
*add_child(struct GROOT_SRT_CHILD *lst_child, struct GROOT_SRT_CHILD *new_child){
	struct GROOT_SRT_CHILD *tmp_child = child_lst_tail(lst_child);
	if(tmp_child == NULL){
		return NULL;
	}

	tmp_child->next = new_child;
	return new_child;
}

/**
 * @brief Remove Child from list
 * @details Remove Child from list
 * 
 * @param GROOT_QUERY_ITEM Pointer to list
 * @param GROOT_SRT_CHILD Child to remove
 */
static void
rm_child(struct GROOT_QUERY_ITEM *list, struct GROOT_SRT_CHILD *child){
	struct GROOT_SRT_CHILD *prev_child = NULL, *tmp_child = NULL;
	uint8_t isFound = 0;

	tmp_child = list->children;
	while(tmp_child != NULL){
		if(tmp_child != child){
			prev_child = tmp_child;
			tmp_child = tmp_child->next;
			continue;
		}
		isFound = 1;
		//If root child update list
		if(prev_child == NULL){
			list->children = child->next;
		} else { // update previous
			prev_child->next = tmp_child->next;
		}
		
		prev_child = tmp_child;
		tmp_child = tmp_child->next;
	}
	
	if(isFound == 1){
		memset(child, 0, sizeof(struct GROOT_SRT_CHILD));
		memb_free(&groot_children, child);
	}
}

/**
 * @brief Update the time the parent was last seen
 * @details Every time a broadcast is sent update all queries that have sender as parent.
 *          Used to trekk whether the parent is still alive
 * 
 * @param parent address of parent
 */
static void
update_parent_last_seen(const rimeaddr_t *parent){
	struct GROOT_QUERY_ITEM *qry_itm = NULL;
	int least_time;
	qry_itm = list_head(groot_qry_table);
	while(qry_itm != NULL){
		if(rimeaddr_cmp(&qry_itm->parent, parent)){
			qry_itm->parent_last_seen = clock_seconds();
			qry_itm = qry_itm->next;
			continue; 
		}

		least_time = least_idle_time(qry_itm, GROOT_RETRIES_PARENT, 2);
		if(least_time > 0 && qry_itm->parent_last_seen > 0 && qry_itm->parent_last_seen <= least_time){
			if(!ctimer_expired(&qry_itm->query_timer)){
				//Stop Sampe timer
				ctimer_stop(&qry_itm->query_timer);
			}
			rimeaddr_copy(&qry_itm->parent, &rimeaddr_null);
		}

		qry_itm = qry_itm->next;
	}
}

/**
 * @brief Remove Query from query list
 * @details Remove Query from query list
 * 
 * @param i query list item
 */
static void
cb_rm_query(void *i){
	printf("REMOVING QUERY!! \n");
	struct GROOT_QUERY_ITEM *lst_itm = (struct GROOT_QUERY_ITEM *)i;

	if(!ctimer_expired(&lst_itm->query_timer)){
		//Stop Sampe timer
		ctimer_stop(&lst_itm->query_timer);
	}

	list_remove(groot_qry_table, lst_itm);
	memset(lst_itm, 0, sizeof(struct GROOT_QUERY_ITEM));
	memb_free(&groot_qrys, lst_itm);
}

/**
 * @brief Check if the packet received is groot
 * @details Check if the packet received is groot
 * 
 * @param GROOT_HEADER 0 false 1 true
 */
static uint8_t
is_groot_protocol(struct GROOT_HEADER *hdr){
	if(hdr->protocol.version != GROOT_VERSION){
		return 0;
	}
	if(hdr->protocol.magic[0] != 'G' || hdr->protocol.magic[1] != 'T'){
		return 0;
	}
	return 1;
}

/**
 * @brief Check if the node has all the sensors needed
 * @details Check if the node has all the sensors needed
 * 
 * @param GROOT_SENSORS 0 false 1 true
 */
static uint8_t
is_capable(struct GROOT_SENSORS *qry_sensors){
		if((qry_sensors->co2 == 1 && glocal.sensors.co2 == 0) ||
			(qry_sensors->no == 1 && glocal.sensors.no == 0) ||
			(qry_sensors->temp == 1 && glocal.sensors.temp == 0) ||
			(qry_sensors->humidity == 1 && glocal.sensors.humidity == 0)){
			return 0;
	}
	return 1;
}

/**
 * @brief Load a packet into buff
 * @details Load: Header, query and data if available into buffer
 * 
 * @param GROOT_HEADER header to load
 * @param GROOT_QUERY query to load or NULL
 * @param GROOT_SENSORS_DATA data to load or NULL
 */
static void
packet_loader_qry(struct GROOT_HEADER *hdr, struct GROOT_QUERY *qry, struct GROOT_SENSORS_DATA *sensors_data){
	int data_l = sizeof(struct GROOT_HEADER);
	struct GROOT_QUERY *pkt_qry = NULL;
	struct GROOT_SENSORS_DATA *pkt_data = NULL;

	//Adding query?
	if(qry != NULL){
		data_l += sizeof(struct GROOT_QUERY);
	}
	//Adding sensor data?
	if(sensors_data != NULL){
		data_l += sizeof(struct GROOT_SENSORS_DATA);
	}
	//Clean buffer and set length
	packetbuf_clear();
	packetbuf_set_datalen(data_l);

	//Copy hdr in buffer
	struct GROOT_HEADER *pkt_hdr = packetbuf_dataptr();
	memcpy(pkt_hdr, hdr, sizeof(struct GROOT_HEADER));
	//If need be copy qry
	if(qry != NULL){
		pkt_qry = (packetbuf_dataptr() + sizeof(struct GROOT_HEADER));
		memcpy(pkt_qry, qry, sizeof(struct GROOT_QUERY));
	}
	//if need be copy sensor data
	if(sensors_data != NULL){
		if(qry == NULL){
			pkt_data = (packetbuf_dataptr()+sizeof(struct GROOT_HEADER));
		} else {
			pkt_data = (packetbuf_dataptr()+sizeof(struct GROOT_HEADER)+sizeof(struct GROOT_QUERY));
		}
		memcpy(pkt_data, sensors_data, sizeof(struct GROOT_SENSORS_DATA));
	}
}

/**
 * @brief Get sensor data from struct
 * @details Get sensor data from struct 
 * 
 * @param sensor sensor id
 * @param GROOT_SENSORS_DATA Data struct
 */
static float
get_sensor_data(uint8_t sensor, struct GROOT_SENSORS_DATA *data){
	int saved = 0;
	switch(sensor){
		case SENSOR_CO2:
			saved = data->co2;
			break;
		case SENSOR_NO:
			saved = data->no;
			break;
		case SENSOR_HUMIDITY:
			saved = data->humidity;
			break;
		case SENSOR_TEMP:
			saved = data->temp;
			break;
	}

	return saved;
}

/**
 * @brief Aggregate Calcualtion
 * @details Aggregate Calculation
 * 
 * @param GROOT_SRT_CHILD List of Children
 * @param sensor Sensors needed
 * @param aggregator Aggregation type
 */
static float
aggregate_calc(struct GROOT_SRT_CHILD *children, uint8_t sensor, uint8_t aggregator){
	struct GROOT_SRT_CHILD *tmp_child;
	float result = 0, tmp_data = 0, tmp_result = 0, count = 0;

	switch(aggregator){
		case GROOT_MAX:
			tmp_child = children;
			while(tmp_child != NULL){
				if(&tmp_child->data <= 0){
					continue;
				}
				tmp_data = get_sensor_data(sensor, &tmp_child->data);
				if(tmp_result < tmp_data){
					tmp_result = tmp_data;
				}
				tmp_child = tmp_child->next;
			}
			result = tmp_result;
			break;
		case GROOT_AVG:
			tmp_child = children;
			while(tmp_child != NULL){
				if(&tmp_child->data <= 0){
					continue;
				}
				tmp_result += get_sensor_data(sensor, &tmp_child->data);
				count += 1;
				tmp_child = tmp_child->next;
			}
			result = tmp_result / count;
			break;
		case GROOT_MIN:
			tmp_child = children;
			while(tmp_child != NULL){
				if(&tmp_child->data <= 0){
					continue;
				}
				tmp_data = get_sensor_data(sensor, &tmp_child->data);
				if(tmp_result > tmp_data){
					tmp_result = tmp_data;
				}
				result = tmp_result;
				tmp_child = tmp_child->next;
			}
			break;
	}
	return result;
}

/**
 * @brief Send the actual data sample
 * @details Send the actual data sample
 * 
 * @param GROOT_QUERY_ITEM Query list item
 * @param GROOT_SENSORS_DATA Sensor data calclated
 */
static void
send_sample(struct GROOT_QUERY_ITEM *qry_itm, struct GROOT_SENSORS_DATA *sensors_data){
	struct GROOT_HEADER hdr;
	struct GROOT_QUERY qry;

	if(rimeaddr_cmp(&qry_itm->parent, &rimeaddr_null) > 0){
		return;
	}

	hdr.protocol.version = GROOT_VERSION;
	hdr.protocol.magic[0] = 'G';
	hdr.protocol.magic[1] = 'T';
	rimeaddr_copy(&hdr.to, &qry_itm->parent);
	rimeaddr_copy(&hdr.ereceiver, &qry_itm->ereceiver);
	rimeaddr_copy(&hdr.received_from, &rimeaddr_null);

	hdr.is_cluster_head = qry_itm->is_serviced;
	hdr.type = GROOT_PUBLISH_TYPE;
	hdr.query_id = qry_itm->query_id;

	//Increment Sample Id
	qry_itm->query.sample_id += 1;
	copy_qry(&qry, &qry_itm->query);

	PRINT2ADDR(&rimeaddr_node_addr);
	printf("- { SENDING SAMPLE %d - QID - %d } ", qry.sample_id, qry_itm->query_id);
	printf("- [ CO2 - %.2f NO - %.2f TEMP - %.2f HUMIDITY - %.2f ] \n", sensors_data->co2, 
			sensors_data->no, sensors_data->temp, sensors_data->humidity);

	qry_itm->last_published = clock_seconds();
	packet_loader_qry(&hdr, &qry, sensors_data);
	broadcast_send(&glocal.channels->bc);
}

/**
 * @brief Check whether aggregates can be sent
 * @details Check whether children have replied and aggregation can be sent
 * 
 * @param GROOT_QUERY_ITEM query item to handle
 */
static uint8_t
can_send_aggregate(struct GROOT_QUERY_ITEM *qry_itm){
	struct GROOT_SRT_CHILD *tmp_child = qry_itm->children;
	struct GROOT_SRT_CHILD *nxt_child;
	int lst_time;

	while(tmp_child != NULL){

		if(qry_itm->last_published != 0){
			//Passed through 3 times without setting. Node is dead!
		 	lst_time = least_idle_time(qry_itm, GROOT_RETRIES_AGGREGATION, 2);
			if(lst_time > 0 && tmp_child->last_set <= lst_time){
				nxt_child = tmp_child->next;
				rm_child(qry_itm, tmp_child);
				tmp_child = nxt_child;
				continue;
			}
		}

		if((tmp_child->last_set <= qry_itm->last_published && qry_itm->agg_passes < 3) && 
			ctimer_expired(&qry_itm->maintainer_t))
		{
			//Increment Passes
			qry_itm->agg_passes += 1;
			ctimer_set(&qry_itm->maintainer_t, (rand()%(1*CLOCK_SECOND)), cb_publish_aggregate, qry_itm);
			return 0;
		}

		tmp_child = tmp_child->next;
	}

	return 1;
}

/**
 * @brief Get the actual aggregate data
 * @details Get the actual aggregate data
 * 
 * @param i List item
 */
static void
cb_publish_aggregate(void *i){
	struct GROOT_QUERY_ITEM *lst_itm = (struct GROOT_QUERY_ITEM *)i;
	struct GROOT_SENSORS_DATA data;
	struct GROOT_SENSORS *req = NULL;

	if(can_send_aggregate(lst_itm) == 0){
		return;
	}
	
	//Sending so aggregation pass is done!
	lst_itm->agg_passes = 0;
	req = &lst_itm->query.sensors_required;

	if(req->co2 == 1){
		data.co2 = aggregate_calc(lst_itm->children, SENSOR_CO2, lst_itm->query.aggregator);
	}

	if(req->no == 1){
		data.no = aggregate_calc(lst_itm->children, SENSOR_NO, lst_itm->query.aggregator);
	}

	if(req->temp == 1){
		data.temp = aggregate_calc(lst_itm->children, SENSOR_TEMP, lst_itm->query.aggregator);
	}

	if(req->humidity == 1){
		data.humidity = aggregate_calc(lst_itm->children, SENSOR_HUMIDITY, lst_itm->query.aggregator);
	}
	
	printf("Aggregating - ");
	print_data(&data);

	//Send the data
	send_sample(lst_itm, &data);
}

/**
 * @brief Call back called to periodically set/send sample
 * @details Call back called according to the sample rate. 
 *          If no Aggregation is given send the data if aggregation save data as child.
 *          This method also triggers the send aggregation.
 * 
 * @param i Query Item that sample needs
 */
static void
cb_sampler(void *i){
	struct GROOT_QUERY_ITEM *qry_itm = (struct GROOT_QUERY_ITEM *)i;
	struct GROOT_SENSORS_DATA sensors_data;
	struct GROOT_SRT_CHILD *child;

	//Get Sensor readings - in this case random numbers due to the use of a simulator
	random_sensor_readings(&qry_itm->query.sensors_required, &sensors_data);
	
	//Send the data
	if(qry_itm->query.aggregator == GROOT_NO_AGGREGATION){
		send_sample(qry_itm, &sensors_data);
	} else {
		child = get_child(qry_itm->children, &rimeaddr_node_addr);
		if(child != NULL){
			memcpy(&child->data, &sensors_data, sizeof(struct GROOT_SENSORS_DATA));
			print_data(&child->data);
			child->last_set = clock_seconds();
		}

		////Initialize ctimer to check if all children arrived or check with every sample
		if(ctimer_expired(&qry_itm->maintainer_t)){
			ctimer_set(&qry_itm->maintainer_t, (rand()%(1*CLOCK_SECOND)), cb_publish_aggregate, qry_itm);
		}
	}

	//Check if timer is being used by someone else
	if(ctimer_expired(&qry_itm->query_timer)){
		printf("QUERY TIMER: %d \n", qry_itm->query.sample_rate);
		ctimer_set(&qry_itm->query_timer, qry_itm->query.sample_rate, cb_sampler, qry_itm);
	}
}

/**
 * @brief Used to reboradcast alteration
 * @details Used to rebroadcast alteration. Used to reduce collision by delay
 * 
 * @param lst_itm List Query item
 */
static void
rebroadcast_alter(void *lst_itm){
	struct GROOT_HEADER hdr;
	struct GROOT_QUERY_ITEM *itm = (struct GROOT_QUERY_ITEM *)lst_itm;

	hdr.protocol.version = GROOT_VERSION;
	hdr.protocol.magic[0] = 'G';
	hdr.protocol.magic[1] = 'T';

	rimeaddr_copy(&hdr.to, &rimeaddr_null);
	rimeaddr_copy(&hdr.ereceiver, &itm->ereceiver);
	rimeaddr_copy(&hdr.received_from, &itm->rcv_alter);

	hdr.is_cluster_head = itm->is_serviced;
	hdr.type = GROOT_ALTERATION_TYPE;
	hdr.query_id = itm->query_id;

	printf("Re-Broadcast ALTERATION - { QID: %d } \n",itm->query_id);

	packet_loader_qry(&hdr, &itm->query, NULL);
	broadcast_send(&glocal.channels->bc);
}

/**
 * @brief Rebroadcast unsubscribe
 * @details Used to delay broadcast to reduce collisions.
 * 
 * @param lst_itm Query list item
 */
static void
rebroadcast_unsubscribe(void *lst_itm){
	struct GROOT_HEADER hdr;
	struct GROOT_QUERY_ITEM *itm = (struct GROOT_QUERY_ITEM *)lst_itm;

	hdr.protocol.version = GROOT_VERSION;
	hdr.protocol.magic[0] = 'G';
	hdr.protocol.magic[1] = 'T';

	rimeaddr_copy(&hdr.to, &rimeaddr_null);
	rimeaddr_copy(&hdr.ereceiver, &itm->ereceiver);
	rimeaddr_copy(&hdr.received_from, &itm->parent);

	hdr.is_cluster_head = itm->is_serviced;
	hdr.type = GROOT_UNSUBSCRIBE_TYPE;
	hdr.query_id = itm->query_id;

	printf("Re-Broadcast UNSUBSRIBE - { QID: %d } \n",itm->query_id);

	packet_loader_qry(&hdr, &itm->query, NULL);
	broadcast_send(&glocal.channels->bc);
	ctimer_set(&itm->maintainer_t, GROOT_RM_UNSUBSCRIBE, cb_rm_query, itm);
}

/**
 * @brief Rebroadcast subscribe
 * @details Used to delay rebroadcast to reduce collisions
 * 
 * @param lst_itm List item query
 */
static void
rebroadcast_subscribe(void *lst_itm){
	struct GROOT_HEADER hdr;
	struct GROOT_QUERY_ITEM *itm = (struct GROOT_QUERY_ITEM *)lst_itm;

	hdr.protocol.version = GROOT_VERSION;
	hdr.protocol.magic[0] = 'G';
	hdr.protocol.magic[1] = 'T';

	rimeaddr_copy(&hdr.to, &rimeaddr_null);
	rimeaddr_copy(&hdr.ereceiver, &itm->ereceiver);
	rimeaddr_copy(&hdr.received_from, &itm->parent);

	hdr.is_cluster_head = itm->is_serviced;
	hdr.type = GROOT_SUBSCRIBE_TYPE;
	hdr.query_id = itm->query_id;

	printf("Re-Broadcast SUBSCRIBE - { QID: %d } \n",itm->query_id);

	packet_loader_qry(&hdr, &itm->query, NULL);
	broadcast_send(&glocal.channels->bc);
}

/**
 * @brief Send runicast to join cluster
 * @details Send runicast to join cluster
 * 
 * @param lst_item Query list item
 */
static void
cluster_join_send(void *lst_item){
	struct GROOT_HEADER hdr;
	struct GROOT_QUERY_ITEM *itm = (struct GROOT_QUERY_ITEM *)lst_item;

	rebroadcast_subscribe(lst_item);

	hdr.protocol.version = GROOT_VERSION;
	hdr.protocol.magic[0] = 'G';
	hdr.protocol.magic[1] = 'T';

	rimeaddr_copy(&hdr.to, &rimeaddr_null);
	rimeaddr_copy(&hdr.ereceiver, &itm->ereceiver);
	rimeaddr_copy(&hdr.received_from, &rimeaddr_null);

	hdr.is_cluster_head = 1;
	hdr.type = GROOT_CLUSTER_JOIN_TYPE;
	hdr.query_id = itm->query_id;

	printf("JOIN REQUEST - { ");
	PRINT2ADDR(&itm->parent);
	printf(" } \n");

	packet_loader_qry(&hdr, NULL, NULL);
	runicast_send(&glocal.channels->rc, &itm->parent, MAX_RETRANSMISSION);
}

/**
 * @brief Add query to list
 * @details Add query to list
 * 
 * @param GROOT_HEADER header
 * @param GROOT_QUERY Query
 * @param from From
 * @return porinter to add query
 */
static struct  GROOT_QUERY_ITEM
*qry_to_list(struct GROOT_HEADER *hdr, struct GROOT_QUERY *qry_bdy, const rimeaddr_t *from){
	struct GROOT_QUERY_ITEM *new_item;
	struct GROOT_SRT_CHILD *new_child;
	
	//LIST and all MEMORY USED
	if(list_length(groot_qry_table) >= GROOT_QUERY_LIMIT){
		return NULL;
	}

	new_item = memb_alloc(&groot_qrys);
	list_add(groot_qry_table, new_item);

	new_item->query_id = hdr->query_id;
	rimeaddr_copy(&new_item->ereceiver, &hdr->ereceiver);
	rimeaddr_copy(&new_item->parent, from);
	new_item->parent_is_cluster = hdr->is_cluster_head;
	//Check that I have all the sensors needed
	new_item->is_serviced = is_capable(&qry_bdy->sensors_required);
	//When unsubscribe received set this time
	new_item->unsubscribed = 0;
	new_item->last_published = 0;
	//Copy Query Values into row
	copy_qry(&new_item->query, qry_bdy);
	
	new_item->children = NULL;

	//Add node as child to keep data in it
	new_child = memb_alloc(&groot_children);
	rimeaddr_copy(&new_child->address, &rimeaddr_node_addr);
	new_child->last_set = 0;
	new_child->next = NULL;
	new_item->children = new_child;
	
	print_qrys();
	return new_item;
}

/**
 * @brief Find query in list item
 * @details Find query in list item
 * 
 * @param query_id ID
 * @param ereceiver Query owner
 * 
 * @return Pointer of query location
 */
static struct GROOT_QUERY_ITEM
*find_query(uint16_t query_id, const rimeaddr_t *ereceiver){
	struct GROOT_QUERY_ITEM *i;
	//If empty return
	if(list_length(groot_qry_table) == 0){
		return NULL;
	}
	//Get top of head a pass through all queries to find query
	i = list_head(groot_qry_table);
	while(i != NULL){
		if(i->query_id == query_id && rimeaddr_cmp(&i->ereceiver, ereceiver)){
			return i;
		}
		i = i->next;
	}
	return NULL;
}

/*--------------------------------------------- RCV METHODS -------------------------------------------------------------*/
static int
rcv_subscribe(struct GROOT_HEADER *hdr, const rimeaddr_t *from){
	struct GROOT_QUERY *qry_bdy;	
	struct GROOT_QUERY_ITEM *lst_itm = NULL;
	struct GROOT_QUERY snd_bdy;
	struct GROOT_HEADER snd_hdr;
	
	//Already Saved
	lst_itm = find_query(hdr->query_id, &hdr->ereceiver);
	if(lst_itm != NULL){
		return 0;
	}

	//Get Query from buffer
	qry_bdy = packetbuf_get_qry();
	
	//Add the new qry to the table
	lst_itm = qry_to_list(hdr, qry_bdy, from);
	//NOT added to the table do nothing. FOLLOWS ASSUMPTION ALL QUERIES WILL BE SAVED
	if(lst_itm == NULL){
		return 0;
	}

	//If the query is serviced by this node create callback function to send samples
	if(lst_itm->is_serviced == 1){
		//Timer for sampling
		ctimer_set(&lst_itm->query_timer, qry_bdy->sample_rate+(rand()%(1*CLOCK_SECOND)), cb_sampler, lst_itm);
	}

	if(lst_itm->parent_is_cluster == 1){
		//Timer to send out join cluster
		ctimer_set(&lst_itm->maintainer_t, rand()%(CLOCK_SECOND/2), cluster_join_send, lst_itm);
	} else {
		//Rebroadcast even not cluster
		ctimer_set(&lst_itm->maintainer_t, rand()%(CLOCK_SECOND/2), rebroadcast_subscribe, lst_itm);
	}
	return 1;
}

static int
rcv_unsubscribe(struct GROOT_HEADER *hdr, const rimeaddr_t *from){
	struct GROOT_QUERY_ITEM *lst_itm = NULL;

	lst_itm = find_query(hdr->query_id, &hdr->ereceiver);
	//Return if item already deleted or already bcast the unsubscribed
	if(lst_itm == NULL || lst_itm->unsubscribed != 0){
		return 0;
	}

	//Keep the query in the file for a few seconds to stop rebroadcasting
	lst_itm->unsubscribed = clock_seconds();
	rimeaddr_copy(&lst_itm->parent, &rimeaddr_null);
	rimeaddr_copy(&lst_itm->parent, from);

	//Rebroadcast and then initialize removal
	ctimer_set(&lst_itm->maintainer_t, rand()%(CLOCK_SECOND/2), rebroadcast_unsubscribe, lst_itm);
	return 1;
}

static int
rcv_publish(struct GROOT_HEADER *hdr, const rimeaddr_t *from){
	struct GROOT_QUERY_ITEM *lst_itm = NULL, *nm_itm = NULL;
	struct GROOT_SENSORS_DATA *sns_data = NULL;
	struct GROOT_SENSORS_DATA *sns_tmp;
	struct GROOT_SRT_CHILD *child = NULL;
	struct GROOT_QUERY *qry_bdy = NULL;
	
	if(rimeaddr_cmp(&hdr->ereceiver, &rimeaddr_node_addr) > 0){
		printf("------- RECEIVED DATA SUCCESS ------\n");
		print_hdr((struct GROOT_HEADER*)packetbuf_dataptr());
		sns_tmp = (struct GROOT_SENSORS_DATA*)(packetbuf_dataptr()+sizeof(struct GROOT_HEADER)+sizeof(struct GROOT_QUERY));
		print_data(sns_tmp);
		printf("------------------------------------\n");
		return 0;
	}

	//Sink can only accept Data
	if(glocal.is_sink == 1){
		return 0;
	}

	//Not for me!
	if(rimeaddr_cmp(&hdr->to, &rimeaddr_node_addr) == 0){
		update_parent_last_seen(from);
		//Check if I have query. If not add!
		nm_itm = find_query(hdr->query_id, &hdr->ereceiver);
		if(nm_itm == NULL){
			//Get Query from buffer
			qry_bdy = packetbuf_get_qry();
			//Add the new qry to the table
			lst_itm = qry_to_list(hdr, qry_bdy, from);
			if(lst_itm != NULL){
				if(lst_itm->is_serviced == 1){
					printf("QUERY TIMER: %d \n", qry_bdy->sample_rate);
					//Timer for sampling
					ctimer_set(&lst_itm->query_timer, qry_bdy->sample_rate+(rand()%(1*CLOCK_SECOND)), cb_sampler, lst_itm);
				}

				if(lst_itm->parent_is_cluster == 1){
					ctimer_set(&lst_itm->maintainer_t, (rand()%(1*CLOCK_SECOND)), cluster_join_send, lst_itm);	
				}
			}
		} else {
			//If query has no parent update parent
			if(hdr->is_cluster_head == 1 && rimeaddr_cmp(&nm_itm->parent, &rimeaddr_null) > 0){
				rimeaddr_copy(&nm_itm->parent, from);
				nm_itm->parent_last_seen = clock_seconds();
				if(nm_itm->is_serviced > 0){
					ctimer_set(&nm_itm->maintainer_t, rand()%(1*CLOCK_SECOND), cluster_join_send, nm_itm);
					printf("QUERY TIMER: %d \n", nm_itm->query.sample_rate);
					ctimer_set(&nm_itm->query_timer, nm_itm->query.sample_rate, cb_sampler, nm_itm);
				}
				
			}
		}

		return 0;
	}

	lst_itm = find_query(hdr->query_id, &hdr->ereceiver);
	if(lst_itm == NULL){
		return 0;
	}

	//Does not have aggregation just send
	if(lst_itm->query.aggregator == GROOT_NO_AGGREGATION){
		rimeaddr_copy(&hdr->to, &lst_itm->parent);
		broadcast_send(&glocal.channels->bc);
		return 1;
	}

	//Is aggretated store data locally until all data has arrived
	sns_data = packetbuf_get_sensor_data();
	
	PRINT2ADDR(&rimeaddr_node_addr);
	printf(" Sensor Data - ");
	print_data(sns_data);

	child = get_child(lst_itm->children, from);
	//If child set to aggregate. If not Child send bcast
	if(child != NULL){
		memcpy(&child->data, sns_data, sizeof(struct GROOT_SENSORS_DATA));
		child->last_set = clock_seconds();
	} else {
		rimeaddr_copy(&hdr->to, &lst_itm->parent);
		broadcast_send(&glocal.channels->bc);
	}

	print_children(lst_itm->children);
}

static int
rcv_alterate(struct GROOT_HEADER *hdr, const rimeaddr_t *from){
	struct GROOT_QUERY_ITEM *lst_itm = NULL;
	struct GROOT_QUERY *qry_bdy = NULL;
	
	//Get Query from buffer
	qry_bdy = (struct GROOT_QUERY*) (packetbuf_dataptr() + sizeof(struct GROOT_HEADER));

	//Already Saved
	lst_itm = find_query(hdr->query_id, &hdr->ereceiver);
	//already handled
	printf("SAMPLE ID: %d \n", qry_bdy->sample_id);
	if(lst_itm->query.sample_id >= qry_bdy->sample_id){
		printf("Already handled \n");
		return;
	}

	if(lst_itm == NULL){
		//Add the new qry to the table
		lst_itm = qry_to_list(hdr, qry_bdy, from);
	} else {
		//Update Query details
		memcpy(&lst_itm->query.sensors_required, &qry_bdy->sensors_required, sizeof(struct GROOT_SENSORS));
		lst_itm->query.sample_id = qry_bdy->sample_id;
		lst_itm->query.sample_rate = qry_bdy->sample_rate;
		lst_itm->query.aggregator = qry_bdy->aggregator;
		lst_itm->is_serviced = is_capable(&lst_itm->query.sensors_required);
	}

	//Changed Packet Received from
	rimeaddr_copy(&lst_itm->rcv_alter, from);

	//If the query is serviced by this node create callback function to send samples
	if(lst_itm->is_serviced > 0){
		ctimer_set(&lst_itm->query_timer, qry_bdy->sample_rate+(rand()%(1*CLOCK_SECOND)), cb_sampler, lst_itm);
	}

	ctimer_set(&lst_itm->maintainer_t, rand()%(CLOCK_SECOND/4), rebroadcast_alter, lst_itm);
	return 1;
}

static int
rcv_cluster_join(struct GROOT_HEADER *hdr, const rimeaddr_t *from){
	struct GROOT_QUERY_ITEM *lst_itm = NULL;
	struct GROOT_SRT_CHILD *child;

	lst_itm = find_query(hdr->query_id, &hdr->ereceiver);
	//Item not in table or cannot accept more children
	if(lst_itm == NULL || 
		(child_length(lst_itm->children) >= GROOT_CHILD_LIMIT)){
		return 0;
	}

	//@todo check if child is in the children list already

	//Get Child memory
	child = memb_alloc(&groot_children);
	rimeaddr_copy(&child->address, from);
	child->last_set = clock_seconds();
	child->next = NULL;
	//Add Child
	if(lst_itm->children == NULL){
		lst_itm->children = child;
	} else {
		add_child(lst_itm->children, child);
	}

	return 1;
}
/*--------------------------------------------- Main Methods ------------------------------------------------------------*/
void
groot_prot_init(struct GROOT_SENSORS *sensors, struct GROOT_CHANNELS *channels, uint8_t is_sink){
	//Initialise data structures
	list_init(groot_qry_table);
	memb_init(&groot_qrys);
	memb_init(&groot_children);

	//Copy Current Sensors
	memcpy(&glocal.sensors, sensors, sizeof(struct GROOT_SENSORS));
	//Set local channels
	glocal.channels = channels;
	glocal.is_sink = is_sink;
}

int
groot_qry_snd(uint16_t query_id, uint8_t type, uint16_t sample_rate, struct GROOT_SENSORS *data_required, uint8_t aggregator){
	struct GROOT_HEADER hdr;
	struct GROOT_QUERY qry;
	struct GROOT_QUERY_ITEM *lst_itm;

	qry.sample_id = 0;
	qry.sample_rate = sample_rate;
	qry.aggregator = aggregator;
	memcpy(&qry.sensors_required, data_required, sizeof(struct GROOT_SENSORS));

	//Initialise query header
	hdr.protocol.version = GROOT_VERSION;
	hdr.protocol.magic[0] = 'G';
	hdr.protocol.magic[1] = 'T';
	
	//Main Variables
	rimeaddr_copy(&hdr.ereceiver, &rimeaddr_node_addr);
	rimeaddr_copy(&hdr.received_from, &rimeaddr_null);
	hdr.is_cluster_head = 0;
	hdr.type = type;
	hdr.query_id = query_id;

	if(type == GROOT_SUBSCRIBE_TYPE){
		qry_to_list(&hdr, &qry, &rimeaddr_node_addr);
	} else if(type == GROOT_ALTERATION_TYPE){
		lst_itm = find_query(query_id, &rimeaddr_node_addr);
		qry.sample_id = lst_itm->query.sample_id + 1;
	}

	//Create Query Packet
	packet_loader_qry(&hdr, &qry, NULL);
	
	PRINT2ADDR(&rimeaddr_node_addr);
	printf("- { SENDING QUERY ID: %d }\n", query_id);

	//Send packet
	if(!broadcast_send(&glocal.channels->bc)){
		return 0;
	}
	return 1;
}

int
groot_unsubscribe_snd(uint16_t query_id){
	struct GROOT_HEADER hdr;
	
	hdr.protocol.version = GROOT_VERSION;
	hdr.protocol.magic[0] = 'G';
	hdr.protocol.magic[1] = 'T';

	rimeaddr_copy(&hdr.to, &rimeaddr_null);
	rimeaddr_copy(&hdr.received_from, &rimeaddr_null);
	rimeaddr_copy(&hdr.ereceiver, &rimeaddr_node_addr);

	hdr.is_cluster_head = 0;
	hdr.type = GROOT_UNSUBSCRIBE_TYPE;
	hdr.query_id = query_id;

	PRINT2ADDR(&rimeaddr_node_addr);
	printf("- { SENDING UNSUBSRIBE }\n");

	packet_loader_qry(&hdr, NULL, NULL);
	broadcast_send(&glocal.channels->bc);
	return 1;
}

int
groot_rcv(const rimeaddr_t *from){
	struct GROOT_HEADER *hdr = NULL;
	uint8_t is_success = 0;

	hdr = (struct GROOT_HEADER*) packetbuf_dataptr();

	//I just sent this packet ignore
	if(rimeaddr_cmp(&hdr->received_from, &rimeaddr_node_addr) == 1){
		return is_success;
	}

	//Is not correct protocol
	if(!is_groot_protocol(hdr)){
		return is_success;
	}

	printf("RECEIVE FROM - ");
	PRINT2ADDR(from);
	printf("- { RECEIVED - %02x }\n", hdr->type);

	if(glocal.is_sink == 0){
		switch(hdr->type){
			case GROOT_SUBSCRIBE_TYPE:
				printf("SUBSCRIBING \n");
				is_success = rcv_subscribe(hdr, from);
				break;
			case GROOT_UNSUBSCRIBE_TYPE:
				//Do I have this query?
				printf("UNSUBSRIBING!! \n");
				is_success = rcv_unsubscribe(hdr, from);
				break;
			case GROOT_ALTERATION_TYPE:
				printf("ALTERATION!! \n");
				is_success = rcv_alterate(hdr, from);
				break;
			case GROOT_CLUSTER_JOIN_TYPE:
				printf("JOIN CLUSER!! \n");
				is_success = rcv_cluster_join(hdr, from);
				break;
		}
	} 

	//Both Sink and Sensor have this functionality
	if(hdr->type == GROOT_PUBLISH_TYPE){
		//Publish Sensed data
		is_success = rcv_publish(hdr, from);
	}
	return is_success;
}