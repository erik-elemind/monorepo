/*
 * compression_task.c
 *
 * Copyright (C) 2020 Elemind Technologies, Inc.
 *
 * Created: Feb, 2022
 * Author:  Dinesh Ribadiya
 */

#include "compression_test_task.h"
#include <stdlib.h>
#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "queue.h"
#include "message_buffer.h"

#include "utils.h"
#include "config.h"
#include "config_tracealyzer_isr.h"

#include "data_log.h"


#if (defined(ENABLE_COMPRESSION_TEST_TASK) && (ENABLE_COMPRESSION_TEST_TASK > 0U))

#include "examples/main_algo.h"

#define COMPRESSION_EVENT_QUEUE_SIZE 20


static const char *TAG = "compression_task";  // Logging prefix for this module

//
// Task events:
//
typedef enum
{
  COMPRESSION_EVENT_ENTER_STATE,	// (used for state transitions)
  COMPRESSION_EVENT_TEST1,
  COMPRESSION_EVENT_TEST2,
  COMPRESSION_EVENT_TEST3,
} compression_task_event_type_t;

// Events are passed to the g_event_queue with an optional
// void *user_data pointer (which may be NULL).
typedef struct
{
  compression_task_event_type_t type;
} compression_task_event_t;

//
// State machine states:
//
typedef enum
{
  COMPRESSION_STATE_STANDBY,

} compression_task_state_t;


// Global context data:
//
typedef struct
{
  compression_task_state_t state;

} compression_task_context_t;

static compression_task_context_t g_compression_task_context;
static TaskHandle_t g_compression_task_task_handle = NULL;

// Global event queue and handler:
static uint8_t g_event_queue_array[COMPRESSION_EVENT_QUEUE_SIZE*sizeof(compression_task_event_t)];
static StaticQueue_t g_event_queue_struct;
static QueueHandle_t g_event_queue;
static void handle_event(compression_task_event_t *event);


// For logging and debug:
static const char *
compression_task_state_name(compression_task_state_t state)
{
  switch (state) {
    case COMPRESSION_STATE_STANDBY:  return "COMPRESSION_STATE_STANDBY";
    default:
      break;
  }
  return "EEG_READER_STATE UNKNOWN";
}


static const char *
compression_task_event_type_name(compression_task_event_type_t event_type)
{
  switch (event_type) {
    case COMPRESSION_EVENT_ENTER_STATE: return "COMPRESSION_EVENT_ENTER_STATE";
    case COMPRESSION_EVENT_TEST1:         return "COMPRESSION_EVENT_TEST1";
    case COMPRESSION_EVENT_TEST2:         return "COMPRESSION_EVENT_TEST2";
    case COMPRESSION_EVENT_TEST3:         return "COMPRESSION_EVENT_TEST3";
    default:
      break;
  }
  return "COMPRESSION_EVENT UNKNOWN";
}

void compression_task_test1(void){
  compression_task_event_t event = {.type = COMPRESSION_EVENT_TEST1};
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

void compression_task_test2(void){
  compression_task_event_t event = {.type = COMPRESSION_EVENT_TEST2};
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

void compression_task_test3(void){
  compression_task_event_t event = {.type = COMPRESSION_EVENT_TEST3};
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}


static void
log_event(compression_task_event_t *event)
{
  switch (event->type) {
    case COMPRESSION_EVENT_TEST1:
    case COMPRESSION_EVENT_TEST2:
    case COMPRESSION_EVENT_TEST3:
      // do nothing
      break;
    default:
      LOGV(TAG, "[%s] Event: %s", compression_task_state_name(g_compression_task_context.state), compression_task_event_type_name(event->type));
      break;
  }
}

static void
log_event_ignored(compression_task_event_t *event)
{
  switch(event->type){
    case COMPRESSION_EVENT_TEST1:
    case COMPRESSION_EVENT_TEST2:
    case COMPRESSION_EVENT_TEST3:
      // do nothing
      break;
    default:
      LOGD(TAG, "[%s] Ignored Event: %s", compression_task_state_name(g_compression_task_context.state), compression_task_event_type_name(event->type));
      break;
  }
}


CMPR_FLOAT raw_eeg_data[1024] = {18625,18625,18599,18546,18493,18499,18652,19074,19903,21254,23176,25586,28252,30808,32829,33951,33985,32990,31271,29302,27588,26515,26252,26712,27606,28554,29213,29384,29053,28376,27602,26980,26674,26715,27013,27396,27686,27756,27573,27192,26730,26314,26038,25936,25979,26097,26209,26252,26194,26044,25834,25607,25404,25250,25153,25104,25087,25079,25062,25023,24957,24869,24774,24685,24617,24574,24546,24515,24454,24343,24174,23958,23726,23514,23357,23272,23250,23263,23269,23226,23113,22933,22719,22522,22394,22365,22428,22539,22624,22611,22449,22134,21714,21269,20893,20652,20571,20622,20736,20831,20840,20733,20523,20255,19986,19760,19589,19454,19310,19116,18846,18508,18140,17796,17526,17355,17274,17241,17193,17066,16817,16432,15938,15386,14843,14371,14015,13788,13674,13633,13610,13547,13401,13150,12797,12374,11927,11505,11146,10863,10646,10462,10269,10030,9725,9358,8953,8546,8176,7873,7651,7502,7405,7326,7233,7101,6919,6689,6426,6149,5876,5621,5388,5175,4973,4779,4593,4422,4280,4180,4127,4119,4140,4169,4186,4176,4136,4079,4023,3991,3999,4051,4141,4249,4353,4436,4496,4542,4598,4695,4855,5088,5382,5705,6011,6257,6416,6486,6494,6490,6530,6657,6889,7211,7578,7936,8231,8433,8537,8565,8556,8555,8602,8719,8915,9177,9476,9771,10015,10165,10193,10094,9888,9622,9360,9164,9082,9129,9285,9498,9695,9809,9794,9645,9402,9134,8921,8825,8870,9032,9251,9446,9546,9509,9335,9065,8762,8493,8313,8247,8290,8408,8557,8688,8765,8768,8701,8583,8447,8329,8257,8248,8298,8385,8478,8544,8564,8538,8481,8419,8376,8359,8359,8351,8313,8238,8143,8065,8049,8121,8282,8498,8716,8882,8968,8977,8941,8910,8924,9003,9138,9297,9444,9549,9603,9619,9623,9644,9700,9796,9917,10042,10146,10212,10229,10199,10128,10028,9911,9794,9692,9627,9616,9671,9792,9959,10138,10285,10355,10321,10181,9963,9725,9536,9458,9527,9738,10039,10346,10567,10626,10491,10181,9766,9343,9011,8841,8855,9023,9270,9505,9642,9627,9455,9164,8825,8518,8308,8225,8261,8376,8516,8629,8683,8668,8593,8482,8359,8251,8180,8167,8228,8369,8579,8830,9075,9258,9333,9281,9119,8897,8691,8573,8588,8742,8994,9271,9495,9602,9566,9403,9167,8929,8757,8694,8744,8877,9035,9156,9196,9140,9009,8855,8739,8714,8806,9008,9278,9555,9775,9889,9874,9737,9509,9236,8964,8729,8550,8427,8342,8273,8199,8111,8014,7925,7863,7841,7858,7902,7952,7988,7998,7980,7941,7888,7824,7745,7635,7480,7271,7016,6738,6477,6273,6158,6143,6213,6329,6443,6511,6510,6441,6330,6215,6135,6112,6145,6212,6280,6315,6297,6226,6121,6013,5932,5896,5904,5936,5960,5942,5862,5720,5536,5347,5196,5119,5135,5240,5409,5601,5772,5886,5925,5893,5813,5720,5644,5606,5604,5621,5628,5601,5533,5432,5326,5245,5210,5226,5274,5323,5341,5307,5221,5102,4983,4894,4855,4866,4909,4955,4977,4958,4900,4815,4727,4653,4603,4574,4555,4531,4494,4443,4386,4331,4288,4253,4219,4171,4095,3986,3853,3715,3603,3547,3568,3668,3833,4027,4205,4323,4342,4248,4048,3778,3491,3247,3092,3049,3109,3229,3355,3431,3423,3331,3186,3037,2938,2920,2984,3099,3213,3273,3245,3126,2941,2739,2571,2473,2458,2512,2605,2695,2748,2743,2676,2557,2405,2245,2099,1984,1912,1889,1915,1980,2062,2136,2171,2147,2059,1922,1767,1634,1559,1558,1624,1725,1813,1842,1777,1611,1366,1090,843,679,635,713,882,1085,1256,1332,1280,1099,823,513,239,61,9,79,226,386,486,473,328,75,-231,-520,-728,-818,-791,-682,-547,-444,-418,-487,-645,-866,-1113,-1351,-1551,-1694,-1771,-1775,-1707,-1574,-1395,-1199,-1030,-931,-937,-1060,-1278,-1543,-1788,-1949,-1984,-1888,-1694,-1464,-1267,-1158,-1166,-1282,-1469,-1674,-1845,-1950,-1984,-1968,-1936,-1926,-1962,-2049,-2172,-2302,-2409,-2468,-2468,-2415,-2331,-2243,-2179,-2156,-2172,-2208,-2230,-2205,-2112,-1953,-1753,-1556,-1407,-1340,-1363,-1459,-1593,-1729,-1839,-1917,-1973,-2032,-2114,-2226,-2355,-2476,-2559,-2584,-2554,-2491,-2431,-2407,-2435,-2505,-2584,-2629,-2605,-2502,-2338,-2157,-2007,-1928,-1932,-1998,-2087,-2150,-2154,-2088,-1972,-1845,-1753,-1726,-1770,-1861,-1957,-2009,-1987,-1890,-1753,-1635,-1600,-1693,-1921,-2252,-2613,-2921,-3099,-3108,-2950,-2677,-2367,-2108,-1968,-1973,-2103,-2298,-2479,-2575,-2550,-2411,-2210,-2022,-1920,-1953,-2126,-2399,-2703,-2958,-3102,-3105,-2983,-2787,-2590,-2460,-2442,-2538,-2718,-2923,-3094,-3190,-3202,-3157,-3103,-3091,-3152,-3286,-3459,-3617,-3706,-3690,-3568,-3372,-3159,-2986,-2897,-2900,-2976,-3081,-3172,-3220,-3222,-3197,-3175,-3181,-3223,-3286,-3344,-3371,-3354,-3299,-3228,-3167,-3135,-3137,-3164,-3199,-3228,-3247,-3262,-3285,-3324,-3377,-3431,-3466,-3462,-3407,-3304,-3171,-3034,-2918,-2841,-2809,-2816,-2852,-2903,-2958,-3008,-3047,-3067,-3061,-3025,-2957,-2868,-2776,-2706,-2684,-2724,-2825,-2968,-3120,-3241,-3300,-3278,-3179,-3021,-2835,-2651,-2492,-2374,-2298,-2262,-2258,-2279,-2320,-2377,-2446,-2524,-2602,-2667,-2703,-2695,-2633,-2519,-2367,-2197,-2038,-1910,-1831,-1805,-1826,-1881,-1948,-2003,-2024,-1994,-1907,-1769,-1597,-1418,-1259,-1142,-1074,-1046,-1039,-1022,-970,-874,-741,-596,-477,-415,-425,-497,-600,-691,-728,-686,-564,-381,-173,23,181,286,338,349,335,316,311,338,414,545,727,942,1159,1341,1452,1471,1399,1260,1099,966,903,932,1046,1212,1388,1529,1608,1624,1594,1551,1527,1546,1616,1731,1876,2031,2177,2298,2382,2425,2431,2414,2397,2402,2443,2520,2613,2685,2692,2598,2389,2086,1743,1441,1264,1275,1493,1884,2369,2839,3193,3362,3329,3136,2865,2612,2461,2450,2571,2766,2957,3066,3048,2902,2674,2442,2289,2276,2426,2711,3067,3410,3666,3786,3766,3641,3469,3310,3209,3183,3221,3295,3376,3446,3504,3562,3637,3738,3858,3978,4068,4098,4052,3934,3771,3606,3492,3473,3570,3774,4039,4298,4479,4526,4419,4183,3879,3590,3390,3327,3408,3600,3842,4063,4204};

static void
handle_event(compression_task_event_t *event)
{
  // handle stateless events
  switch (event->type) {
  case COMPRESSION_EVENT_TEST1:
    CMPR_LOG_PUTS((char *)"log: COMPRESSION_EVENT_START");
    main_algo(raw_eeg_data, (sizeof(raw_eeg_data)/sizeof(raw_eeg_data[0])));
    return;
  case COMPRESSION_EVENT_TEST2:
    compare_wavelet_impl(raw_eeg_data, (sizeof(raw_eeg_data)/sizeof(raw_eeg_data[0])));
    break;
  case COMPRESSION_EVENT_TEST3:
    main_algo2(raw_eeg_data, (sizeof(raw_eeg_data)/sizeof(raw_eeg_data[0])));
    break;
  default:
    break;
  }

  switch (g_compression_task_context.state) {
    case COMPRESSION_STATE_STANDBY:
      log_event_ignored(event);
      break;

    default:
      // (We should never get here.)
      LOGE(TAG, "Unknown compression_task state: %d", (int) g_compression_task_context.state);
      break;
  }

}

void
compression_pretask_init(void)
{
  // Any pre-scheduler init goes here.

  // Create the event queue before the scheduler starts. Avoids race conditions.
  g_event_queue = xQueueCreateStatic(COMPRESSION_EVENT_QUEUE_SIZE,sizeof(compression_task_event_t),g_event_queue_array,&g_event_queue_struct);
  vQueueAddToRegistry(g_event_queue, "compression_task_event_queue");

}


static void
task_init()
{
  // Any post-scheduler init goes here.
  g_compression_task_task_handle = xTaskGetCurrentTaskHandle();

  LOGV(TAG, "Task launched. Entering event loop.");
}


void
compression_task(void *ignored)
{
  task_init();

  compression_task_event_t event;

  while (1) {

    // Get the next event.
    xQueueReceive(g_event_queue, &event, portMAX_DELAY);

    log_event(&event);

    handle_event(&event);
  }
}


#endif /*  (defined(ENABLE_COMPRESSION_TEST_TASK) && (ENABLE_COMPRESSION_TEST_TASK > 0U)) */




