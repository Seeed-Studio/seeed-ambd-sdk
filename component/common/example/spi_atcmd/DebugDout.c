#include "DebugDout.h"

#include <device.h>

#define PIN_DEBUG_DOUT0	(_PB_20)
#define PIN_DEBUG_DOUT1	(_PB_21)

void DebugDoutInit(void)
{
	GPIO_InitTypeDef gpioDef;
	memset(&gpioDef, 0, sizeof(gpioDef));
	gpioDef.GPIO_Pin = PIN_DEBUG_DOUT0;
	gpioDef.GPIO_Mode = GPIO_Mode_OUT;
	gpioDef.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(&gpioDef);

	memset(&gpioDef, 0, sizeof(gpioDef));
	gpioDef.GPIO_Pin = PIN_DEBUG_DOUT1;
	gpioDef.GPIO_Mode = GPIO_Mode_OUT;
	gpioDef.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(&gpioDef);
}

void DebugDout0(bool on)
{
	GPIO_WriteBit(PIN_DEBUG_DOUT0, on ? GPIO_PIN_HIGH : GPIO_PIN_LOW);
}

void DebugDout1(bool on)
{
	GPIO_WriteBit(PIN_DEBUG_DOUT1, on ? GPIO_PIN_HIGH : GPIO_PIN_LOW);
}
