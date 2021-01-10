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
#include <math.h>
#include <sys/time.h>

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

#define PI 3.14159265358979323846

/*
 * Modulated Signal Test Using 3 Frequencies
 * https://ccrma.stanford.edu/~jos/mdft/Sinusoidal_Amplitude_Modulation_AM.html
 * https://www.youtube.com/watch?v=NgT5u1R2xKo
 *
 * @param freq1 - the first modulation frequency
 * @param freq2 - the first modulation frequency
 * @param freq3 - the first modulation frequency
 * @param offset - the voltage offset for the generated ramp
 * @param amplitude - the amplitude for the generated ramp
 * @param length - the samples to use for this waveform
 */
void dacModulated(ZMODDAC1411 *dacZmod, uint16_t *buf, double freq1, double freq2, double freq3, float amplitude, size_t length, uint8_t gain)
{
	double freq1_mf = (double)2*(double)PI*freq1/(double)length;
	double freq2_mf = (double)2*(double)PI*freq2/(double)length;
	double freq3_mf = (double)2*(double)PI*freq3/(double)length;
	for (uint32_t i = 0; i < length; i++)
	{
		buf[i] = dacZmod->getSignedRawFromVolt(
			amplitude*sin(i*freq1_mf)*sin(i*freq2_mf)*sin(i*freq3_mf),
			gain
		);
	}
}

/*
 * Simple DAC test, using simple ramp values populated in the buffer.
 * @param offset - the voltage offset for the generated ramp
 * @param amplitude - the amplitude for the generated ramp
 * @param length - the samples to use for this waveform
 */
void dacRampDemo(ZMODDAC1411 *dacZmod, uint16_t *buf, float offset, float amplitude, size_t length, uint8_t gain)
{
	uint32_t i;
	int64_t valInt;
	int64_t offsetInt = ((double)offset/(double)(gain ? 5.0:1.25))*(double)((int64_t)1<<(13+48));
	int64_t amplitudeInt = ((double)amplitude/(double)(gain ? 5.0:1.25))*(double)((int64_t)1<<(13+48));
	int64_t stepInt = amplitudeInt/(length>>2);

	// ramp up
	i = 0;
	valInt = -amplitudeInt+offsetInt;
	if (valInt < (int64_t)-0x1FFF << 48)
	{
		valInt = (int64_t)-0x1FFF << 48;
	}
	for(; i < length>>1; i++)
	{
		if (i < 5 || i > (length>>1)-5) {
			std::cout << "\nramp up: " <<  (int16_t)(valInt>>48) << " " << i;
		}
		buf[i] = ((uint16_t)((uint16_t)(valInt>>48) & 0x3FFF)) << 2;
		valInt += stepInt;
	}
	// ramp down
	valInt = amplitudeInt+offsetInt;
	if (valInt > (int64_t)0x1FFF << 48)
	{
		valInt = (int64_t)0x1FFF << 48;
	}
	for(; i < length; i++)
	{
		if (i < (length>>1)+5 || i > length-5) {
			std::cout << "\nramp down: " << (int16_t)(valInt>>48) << " " << i;
		}
		buf[i] = ((uint16_t)((uint16_t)(valInt>>48) & 0x3FFF)) << 2;;
		valInt -= stepInt;
	}

	std::cout << "\nused length: ";
	std::cout << length;
	std::cout << "\nused amplitudeInt: ";
	std::cout << amplitudeInt;
	std::cout << "\nused offsetInt: ";
	std::cout << offsetInt;
	std::cout << "\nused stepInt: ";
	std::cout << stepInt;
	std::cout << "\nlast i: ";
	std::cout << i;
	std::cout << "\nlast valInt: ";
	std::cout << valInt;
}

int main() {
	std::cout << "\nZmodDAC1411 Demo Started";

	ZMODDAC1411 dacZmod(DAC_BASE_ADDR, DAC_DMA_CH1_BASE_ADDR, DAC_DMA_CH2_BASE_ADDR, IIC_BASE_ADDR, DAC_FLASH_ADDR, DAC_DMA_CH1_IRQ, DAC_DMA_CH2_IRQ);
	size_t lengthCh1 = 100000000; // 0x10000000
	size_t lengthCh2 = 100000000; // 45000000
	uint16_t *bufCh1 = dacZmod.allocBuffer(0, lengthCh1);
	uint16_t *bufCh2 = dacZmod.allocBuffer(1, lengthCh2);
	uint32_t currentCycleCh1 = 0;
	uint32_t currentCycleCh2 = 0;
	uint32_t maxCycles = 100000;
	struct timeval stop, start;

	dacZmod.setOutputSampleFrequencyDivider(0);
	// channel 					0 - CH1, 1 - CH2
	// gain						1 - corresponds to HIGH input Range
	dacZmod.setGain(0, 1);
	dacZmod.setGain(1, 1);

	// dacPtr
	// bufPtr
	// offset 					2 V
	// amplitude 				3 V
	// length 					20000000 Samples
	// gain						1 - corresponds to HIGH input Range
	gettimeofday(&start, NULL);
//	dacRampDemo(&dacZmod, bufCh1, 2, 3, lengthCh1, 1);
	// first two terms		sin2*sin3 = cos(3-2)+cos(3+2) = cos(1)+cos(5)
	// all three terms		sin2*sin3*sin(5) = cos(3-2)+cos(3+2) = cos(4)+cos(6)+cos(10)  (2 -2)*(3 -3)*(5 -5) = ((2+3) (-2+-3) + (-2+3) (-3+2))(5 -5) = (5 -5 1 -1)(5 -5) = ((5+5) (-5+-5) (5-5) (-5+5) (1+5) (-1+-5) (5-1) (1-5)) = (10, -10, 0, 0, 6, -6, 4, -4)
	dacModulated(&dacZmod, bufCh1, 2, 3, 5, 3, lengthCh1, 1); // 0, 4, 6, 10
	gettimeofday(&stop, NULL);
	std::cout << "\nCh1 Buffer Populated in ";
	std::cout << ((stop.tv_sec - start.tv_sec) * 1000000 + stop.tv_usec - start.tv_usec);
	gettimeofday(&start, NULL);
//	dacRampDemo(&dacZmod, bufCh2, 2, 3, lengthCh2, 1);
//	dacModulated(&dacZmod, bufCh2, 3, 5, 7, 3, lengthCh2, 1); // 15, 9, 5, 1
	gettimeofday(&stop, NULL);
	std::cout << "\nCh2 Buffer Populated in ";
	std::cout << ((stop.tv_sec - start.tv_sec) * 1000000 + stop.tv_usec - start.tv_usec);
	std::cout << std::flush;

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
//			std::cout << "\nch1 cycle completed: ";
//			std::cout << currentCycleCh1;
			if (currentCycleCh1 < maxCycles)
			{
				dacZmod.setData(0, bufCh1, lengthCh1);
			}
		}
		if (dacZmod.isDMATransferComplete(1))
		{
			currentCycleCh2++;
//			std::cout << "\nch2 cycle completed: ";
//			std::cout << currentCycleCh2;
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
