/*
 * data_queue.c
 *
 *  Created on: Aug 25, 2021
 *      Author: DavidWang
 */

#include <event_queue.h>
#include <string.h>


void data_queue_init(event_queue_t* eq,
    UBaseType_t queue_len, UBaseType_t queue_item_size, uint8_t* queue_storage,
    size_t data_len, uint8_t* data_storage,
    const char* name){

  // initialize event queue
  eq->event_handle = xQueueCreateStatic( queue_len, queue_item_size, queue_storage, &(eq->event_struct) );
  vQueueAddToRegistry(eq->event_handle, name);

  // initialize data storage
  eq->data_storage = data_storage;
  eq->data_len = data_len;
  eq->data_size = 0;
  eq->head = 0;
  eq->tail = 0;

  // initialize mutex
  eq->sem =  xSemaphoreCreateMutexStatic( &(eq->sem_buffer) );
  //   vQueueAddToRegistry(queue->sem, "ble_uart_tx_sem");
}

BaseType_t data_queue_send(event_queue_t* eq, event_t* event, TickType_t xTicksToWait){
  xSemaphoreTake(eq->sem, portMAX_DELAY);

  // Enough room on queue and the data buffer?
  // If not, release, and wait


//  BaseType_t send_success = xQueueSend( eq->event_handle, event->item, xTicksToWait );

  if ( event->data_len + eq->data_size < eq->data_len){
    memcpy(eq->data_storage, event->data, event->data_len);
  }else{

  }


  xSemaphoreGive(eq->sem);

  return pdPASS;
}

BaseType_t data_queue_receive(event_queue_t* queue, event_t* event, TickType_t xTicksToWait){

  // block on receiving
//  BaseType_t recv_success = xQueueRecieve( queue->event_handle, event->item, xTicksToWait );

  // will have no problem getting data from data queue


  return pdPASS;
}

BaseType_t data_queue_free(event_queue_t* queue, event_t* event){
  // TODO: release the memory used by queue.

  return pdPASS;
}


BaseType_t data_queue_space_available(event_queue_t* queue, size_t* queue_space, size_t* data_space){
  return pdPASS;
}

