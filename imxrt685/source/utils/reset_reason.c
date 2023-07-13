
#include "board_ff4.h"
#include "reset_reason.h"

#include "fsl_device_registers.h"
#include "fsl_debug_console.h"

static uint32_t g_startup_reset_reason_value = 0;

void
save_reset_reason(void)
{
  //g_startup_reset_reason_value = PMC->AOREG1; //ToDo port
}
 
uint32_t
get_reset_reason(void)
{
  return g_startup_reset_reason_value;
}
