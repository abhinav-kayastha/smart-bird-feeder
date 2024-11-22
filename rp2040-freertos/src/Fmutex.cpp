/*
 * Fmutex.cpp
 *
 *  Created on: 15.8.2017
 *      Author: krl
 */

#include "Fmutex.h"

Fmutex::Fmutex()
{
	mutex = xSemaphoreCreateMutex();
}

Fmutex::~Fmutex()
{
	vSemaphoreDelete(mutex);
}

void Fmutex::lock()
{
	xSemaphoreTake(mutex, portMAX_DELAY);
}

void Fmutex::unlock()
{
	xSemaphoreGive(mutex);
}
