/**
 * @file
 * 		Header file for GROOT Middleware for sink. A Semantic Routing Tree protocol to pass queries to nodes
 * @author 
 * 		Johann Mifsud <johann.mifsud.13@ucl.ac.uk>
 */

#include "groot.h"

/**
 * @brief When sink is turned on this function is executed.
 * @details When sink is turned on this function has to be executed to prepare protocol for work.
 * 
 * @param supported_sensors Pass the sink sensor capabilities
 */
void
sink_bootstrap(struct GROOT_SENSORS *supported_sensors);

/**
 * @brief When calling this function, a query will be generated and sent to sensors.
 * @details When calling this function. A query is generated and will be proliferated to the other sensors.
 *          First the query is broadcasted to all the sensors. The sensors will then create aggregations and
 *          start sending data back to the sink.
 * 
 * @param sample_rate The rate at which the sensors should gather information from the sensor
 * @param data_required The sensor data required
 * @param aggregation The aggregation needed on the data
 */
int
sink_subscribe(uint16_t sample_rate, struct GROOT_SENSORS *data_required, uint8_t aggregation);

int
sink_unsubscribe(uint16_t query_id);

int
sink_send();

void
sink_destroy();