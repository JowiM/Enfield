/**
 * @file 
 * 	Header file for GROOT Middleware for sensors. A Semantic Routing Tree protocol to pass queries to nodes.
 * @author 
 * 		Johann Mifsud <johann.mifsud.13@ucl.ac.uk>
 */
#include "groot.h"

/**
 * @brief Start Sensor mote
 * @details Start Sensor mote
 * 
 * @param GROOT_SENSORS Sensors supported
 */
void
sensor_bootstrap(struct GROOT_SENSORS *support);

/**
 * @brief Kill sensor mote
 * @details Kill sensor mote
 */
int
sensor_destroy();