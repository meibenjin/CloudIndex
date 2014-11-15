/*
 * utils.c
 *
 *  Created on: Aug 1, 2014
 *      Author: meibenjin
 */

#include "utils.h"

int             RUNNING_MODE;
int             NUM_REPLICAS;
uint32_t        CPU_CORE;
uint32_t        DEFAULT_CAPACITY;
int             HEARTBEAT_INTERVAL;
int             MAX_ROUTE_STEP;
double          SIGMA; 
int             SAMPLE_TIME_POINTS;
int             SAMPLE_SPATIAL_POINTS;
double          SAMPLE_TIME_RATE;
int             MAX_RESPONSE_TIME;
int             TOLERABLE_RESPONSE_TIME;
double          EXCHANGE_RATE_RANGE_QUERY;
double          EXCHANGE_RATE_NN_QUERY;
double          EXCHANGE_RATE_PACKAGE_DATA;
double          ESTIMATE_NN_QUERY_COEFFICIENT;
int             ACTIVE_LEADER_NUM;
int             FIXED_IDLE_NODE_NUM;

