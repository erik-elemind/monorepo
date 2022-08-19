
#include "hrm_vcnl4020c.h"

#include <stdio.h>

#include "fsl_debug_console.h"
#include "loglevels.h"

static const char *TAG __attribute__ ((unused)) = "hrm_vcnl"; // Logging prefix for this module

void hrm_init(heart_rate_monitor* hrm, vcnl4020c* vcnl, float hysteresis_percent, float spo2_point, float spo2_slope)
{
	hrm->sensor = vcnl;

	// "size" is the MAX num elements in the red and ir arrays.
	hrm->size = HRM_BUF_SIZE;

	hrm_reinit(hrm);

	hrm->spo2_point = spo2_point;
	hrm->spo2_slope = spo2_slope;

	hrm->red_curr_regval = 0;
	hrm->ir_curr_regval = 0;

	hrm->hys_pct = hysteresis_percent;
}

void hrm_reinit(heart_rate_monitor* hrm){
  // "fill" is the num of elements in the red and ir arrays.
  hrm->fill = 0;

  memset(hrm->red, 0, (hrm->size)*sizeof(hrm->red[0]));
  memset(hrm->ir, 0, (hrm->size)*sizeof(hrm->ir[0]));

  // "new_index" keeps track of where new values will be stored into the red and ir arrays.
  // Since the red and ir arrays will be used as circular buffers,
  // initialize new_index to a value close to the end of the buffer (in this case, 3),
  // to make it easier to test the wrapping logic.
  hrm->new_index = (hrm->size > 3) ? (hrm->size-3) : (hrm->size);

  hrm->red_total = 0;
  hrm->ir_total = 0;
}

void hrm_sample_red(heart_rate_monitor* hrm)
{
  // turn on red led
  vcnl4020c_set_led_current(hrm->sensor, hrm->red_curr_regval);
  vcnl4020c_set_led(hrm->sensor, true, false);
  // set red interrupt
  vcnl4020c_set_interrupt(hrm->sensor, true, false, false, false, 0);
  // ensure interrupt is cleared before we start
  vcnl4020c_clr_interrupt_status(hrm->sensor);
  // read the red
  vcnl4020c_set_measure_once(hrm->sensor, false, true);
}

void hrm_sample_ir(heart_rate_monitor* hrm)
{
  // turn on ir led
  vcnl4020c_set_led_current(hrm->sensor, hrm->ir_curr_regval);
  vcnl4020c_set_led(hrm->sensor, false, true);
  // set ir interrupt
  vcnl4020c_set_interrupt(hrm->sensor, true, false, false, false, 0);
  // ensure interrupt is cleared before we start
  vcnl4020c_clr_interrupt_status(hrm->sensor);
  // read the ir
  vcnl4020c_set_measure_once(hrm->sensor, false, true);
}

void hrm_sample_ambient(heart_rate_monitor* hrm)
{
  // turn off red and ir leds
  vcnl4020c_set_led_current(hrm->sensor, 0);
  vcnl4020c_set_led(hrm->sensor, false, false);
  // set ambient interrupt
  vcnl4020c_set_interrupt(hrm->sensor, false, true, false, false, 0);
  // ensure interrupt is cleared before we start
  vcnl4020c_clr_interrupt_status(hrm->sensor);
  // read the ambient
  vcnl4020c_set_measure_once(hrm->sensor, true, false);
}

/*
 * This sampling function should be called periodically.
 */
void hrm_update(heart_rate_monitor* hrm, uint16_t red, uint16_t ir)
{
	uint16_t newest_index = hrm->new_index;

	uint16_t red_new = red;
	uint16_t red_old = hrm->red[newest_index];
	hrm->red[newest_index] = red_new;
	hrm->red_total = hrm->red_total + red_new - red_old;

	uint16_t ir_new = ir;
	uint16_t ir_old = hrm->ir[newest_index];
	hrm->ir[newest_index] = ir_new;
	hrm->ir_total = hrm->ir_total + ir_new - ir_old;

	// increment the fill counter
	if(hrm->fill < hrm->size){
		hrm->fill = hrm->fill+1;
	}

	// increment the index
	uint16_t next_index = (hrm->new_index+1)%(hrm->size);
	hrm->new_index = next_index;

	// compute dc and ac components
	hrm->red_dc = hrm->red_total / hrm->fill;
	hrm->red_ac = red_new - hrm->red_dc;
	hrm->ir_dc = hrm->ir_total / hrm->fill;
	hrm->ir_ac = ir_new - hrm->ir_dc;

	// dynamically adjust the led currents, once every sample buffer size
	if(newest_index == 0){
		uint16_t lo_thres = 65535*(1 - hrm->hys_pct)/2;
	    uint16_t hi_thres = 65535*(1 + hrm->hys_pct)/2;
//	    LOGV(TAG, "%d <-> %d", lo_thres, hi_thres);
		if(hrm->red_dc < lo_thres){
			hrm->red_curr_regval += 1;
		}else if(hrm->red_dc > hi_thres){
			hrm->red_curr_regval -= 1;
		}
		if(hrm->ir_dc < lo_thres){
			hrm->ir_curr_regval += 1;
		}else if(hrm->ir_dc > hi_thres){
			hrm->ir_curr_regval -= 1;
		}
	}

	// Limit HRM current to within 10-200 mA.
	// This corresponds to current registry values 1 -20.
	if(hrm->red_curr_regval < 1){
	  hrm->red_curr_regval = 1;
	}else if(hrm->red_curr_regval>20){
	  hrm->red_curr_regval = 20;
	}
    if(hrm->ir_curr_regval < 1){
      hrm->ir_curr_regval = 1;
    }else if(hrm->ir_curr_regval>20){
      hrm->ir_curr_regval = 20;
    }

//    LOGV(TAG, "");
//    LOGV(TAG, "newest_index: %d", newest_index);
//    LOGV(TAG, "red_dc %d, ir_dc %d", hrm->red_dc, hrm->ir_dc);
//    LOGV(TAG, "red_ac %d, ir_ac %d", hrm->red_ac, hrm->ir_ac);
//	  LOGV(TAG, "red_CR %d, ir_CR %d", hrm->red_curr_regval,  hrm->ir_curr_regval);
}

/*
 * Returns the BPM
 */
float hrm_get_heart_rate(heart_rate_monitor* hrm)
{
	return 0;
}

/*
 * Returns the blood oxygenation percentage as a floating point number from 0-1.
 */
float hrm_get_spo2(heart_rate_monitor* hrm)
{
	float red_ac_f = (float)(hrm->red_ac);
	float red_dc_f = (float)(hrm->red_dc);
	float ir_ac_f = (float)hrm->ir_ac;
	float ir_dc_f = (float)hrm->ir_dc;

	float a_ratio = (red_ac_f/red_dc_f) / (ir_ac_f/ir_dc_f);
	return hrm->spo2_slope * a_ratio + hrm->spo2_point;
}
