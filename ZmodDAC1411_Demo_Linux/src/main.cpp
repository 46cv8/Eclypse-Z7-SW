/**
 * @file main.cpp
 * @author Cosmin Tanislav
 * @author Cristian Fatu
 * @date 15 Nov 2019
 * @brief File containing the ZMOD DAC1411 linux demo.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fstream>
#include <iostream>

#include "zmod.h"
#include "zmoddac1411.h"

#define TRANSFER_LEN	0x400
#define IIC_BASE_ADDR   0xE0005000
#define ZMOD_IRQ 		61

#define DAC_BASE_ADDR 		0x43C00000
#define DAC_DMA_CH1_BASE_ADDR 	0x40400000
#define DAC_DMA_CH2_BASE_ADDR 	0x40410000
#define DAC_FLASH_ADDR   	0x31
#define DAC_DMA_CH1_IRQ 		61
#define DAC_DMA_CH2_IRQ 		62

/*
 * Simple DAC test, using simple ramp values populated in the buffer.
* @param offset - the voltage offset for the generated ramp
* @param amplitude - the amplitude for the generated ramp
* @param targetLength - the samples to use for this ramp waveform
* @param channel - the channel where samples will be generated
* @param frequencyDivider - the output frequency divider
* @param gain - the gain for the channel
* @param cycles - the number of cycles to run the waveform
*/
void dacRampDemo(ZMODDAC1411 *dacZmod, uint16_t *buf, float offset, float amplitude, size_t length, uint8_t channel, uint8_t frequencyDivider, uint8_t gain)
{
	float val;
	uint16_t valRaw;
	float step;
	uint32_t i;

	step = amplitude/(length>>2);
	dacZmod->setOutputSampleFrequencyDivider(frequencyDivider);
	dacZmod->setGain(channel, gain);

	// ramp up
	i = 0;
	for(; i < length>>1; i++)
	{
		val = -amplitude + (i*amplitude)/(length>>2);
		valRaw = dacZmod->getSignedRawFromVolt(val + offset, gain);
		buf[i] = valRaw;
	}
	// ramp down
	for(; i < length; i++)
	{
		val = amplitude - ((i-(length>>1))*amplitude)/(length>>2);
		valRaw = dacZmod->getSignedRawFromVolt(val + offset, gain);
		buf[i] = valRaw;
	}
	std::cout << "\nused length: ";
	std::cout << length;
	std::cout << "\nused step: ";
	std::cout << step;
	std::cout << "\nlast i: ";
	std::cout << i;
	std::cout << "\nlast value: ";
	std::cout << val;
	std::cout << "\nlast value raw: ";
	std::cout << valRaw;
}

int main() {
	std::cout << "\nZmodDAC1411 Demo Started";

	ZMODDAC1411 dacZmod(DAC_BASE_ADDR, DAC_DMA_CH1_BASE_ADDR, DAC_DMA_CH2_BASE_ADDR, IIC_BASE_ADDR, DAC_FLASH_ADDR, DAC_DMA_CH1_IRQ, DAC_DMA_CH2_IRQ);
	size_t lengthCh1 = 20000000;
	size_t lengthCh2 = 15000000;
	uint16_t *bufCh1 = dacZmod.allocBuffer(0, lengthCh1);
	uint16_t *bufCh2 = dacZmod.allocBuffer(1, lengthCh2);
	uint32_t currentCycleCh1 = 0;
	uint32_t currentCycleCh2 = 0;
	uint32_t maxCycles = 100;

	// dacPtr
	// bufPtr
	// offset 					2 V
	// amplitude 				3 V
	// length 					20000000 Samples
	// channel 					CH1
	// Output Frequency Divider	2
	// gain						HIGH - corresponds to HIGH input Range
	dacRampDemo(&dacZmod, bufCh1, 2, 3, lengthCh1, 0, 0, 1);
	dacRampDemo(&dacZmod, bufCh2, 2, 3, lengthCh2, 1, 0, 1);
	// send data to DAC and start the instrument
	dacZmod.setData(0, bufCh1, lengthCh1);
	dacZmod.setData(1, bufCh2, lengthCh2);
	// sleep to ensure DAC has some data before starting
	sleep(0.1);
	dacZmod.start();
	while (currentCycleCh1 < maxCycles && currentCycleCh2 < maxCycles)
	{
		if (dacZmod.isDMATransferComplete(0))
		{
			currentCycleCh1++;
			std::cout << "\nch1 cycle completed: ";
			std::cout << currentCycleCh1;
			if (currentCycleCh1 < maxCycles)
			{
				dacZmod.setData(0, bufCh1, lengthCh1);
			}
		}
		if (dacZmod.isDMATransferComplete(1))
		{
			currentCycleCh2++;
			std::cout << "\nch2 cycle completed: ";
			std::cout << currentCycleCh2;
			if (currentCycleCh2 < maxCycles)
			{
				dacZmod.setData(1, bufCh2, lengthCh2);
			}
		}
	}
	while (!dacZmod.isDMATransferComplete(0) || !dacZmod.isDMATransferComplete(1)) {;}
	dacZmod.freeBuffer(0, bufCh1, lengthCh1);
	dacZmod.freeBuffer(1, bufCh2, lengthCh2);
	std::cout << "\nZmodDAC1411 Demo Done";
	return 0;
}
