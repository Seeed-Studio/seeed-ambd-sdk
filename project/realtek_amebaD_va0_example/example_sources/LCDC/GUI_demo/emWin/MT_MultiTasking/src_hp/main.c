
#include "ameba_soc.h"
#include "main.h"
#include "GUI.h"
#include "WM.h"
#include "WIDGET.h"
#include "gpio_api.h"
#include "gpio_irq_api.h"

#define INT_PIN    _PA_17

void gpio_touch_irq_handler (uint32_t id, gpio_irq_event event)
{
	GUI_TOUCH_Exec();
}


int main(void)
{
	WM_SetCreateFlags(WM_CF_MEMDEV);
	GUI_Init();

	/*touch interrupt pin*/
	gpio_irq_t gpio_btn;

	gpio_irq_init(&gpio_btn, INT_PIN, gpio_touch_irq_handler, (uint32_t)(&gpio_btn));
	gpio_irq_set(&gpio_btn, IRQ_FALL, 1);   
	gpio_irq_enable(&gpio_btn);
  
	MainTask();
}

