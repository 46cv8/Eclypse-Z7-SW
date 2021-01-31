/**
 * @file main.cpp
 * @author Cosmin Tanislav
 * @author Cristian Fatu
 * @date 15 Nov 2019
 * @brief File containing the ZMOD DAC1411 baremetal demo.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <xil_printf.h>
#include "xparameters.h"

#include "zmoddac1411.h"
#include "zmod.h"


#define TRANSFER_LEN	0x400
#define ZMOD_DAC_BASE_ADDR 	XPAR_ZMODDAC_0_AXI_ZMODDAC1411_V1_0_0_BASEADDR
#define DMA_DAC_CH1_BASE_ADDR 	XPAR_ZMODDAC_0_AXI_DMA_CH1_BASEADDR
#define DMA_DAC_CH2_BASE_ADDR 	XPAR_ZMODDAC_0_AXI_DMA_CH2_BASEADDR
#define FLASH_ADDR_DAC   	0x31
#define DMA_DAC_CH1_IRQ 		XPAR_FABRIC_ZMODDAC_0_AXI_DMA_CH1_MM2S_INTROUT_INTR
#define DMA_DAC_CH2_IRQ 		XPAR_FABRIC_ZMODDAC_0_AXI_DMA_CH2_MM2S_INTROUT_INTR
#define IIC_BASE_ADDR   XPAR_PS7_I2C_1_BASEADDR

/*
 * Simple DAC test, using simple ramp values populated in the buffer.
* @param offset - the voltage offset for the generated ramp
* @param amplitude - the amplitude for the generated ramp
* @param step - the step between two generated samples
* @param channel - the channel where samples will be generated
* @param frequencyDivider - the output frequency divider
* @param gain - the gain for the channel
*/
void dacRampDemo(float offset, float amplitude, float step, uint8_t channel, uint8_t frequencyDivider, uint8_t gain)
{
	ZMODDAC1411 dacZmod(ZMOD_DAC_BASE_ADDR, DMA_DAC_CH1_BASE_ADDR, DMA_DAC_CH2_BASE_ADDR, IIC_BASE_ADDR, FLASH_ADDR_DAC, DMA_DAC_CH1_IRQ, DMA_DAC_CH2_IRQ);
	uint16_t *buf;
	float val;
	uint16_t valRaw;
	size_t length;
	if (amplitude == 0)
	{
		length = 1;
		buf = dacZmod.allocBuffer(channel, length);
		dacZmod.setOutputSampleFrequencyDivider(channel, frequencyDivider);
		dacZmod.setGain(channel, gain);
		valRaw = dacZmod.getSignedRawFromVolt(offset, gain);
		buf[0] = valRaw;
	}
	else if (amplitude != 0)
	{
		length = (size_t)(amplitude/step) << 2;
		int i;
		if (length > ((1<<14) - 1))
		{
			// limit the length to maximum buffer size (1<<14 - 1)
			length = ((1<<14) - 1);
			// adjust step
			step = amplitude/(length>>2);
		}

		buf = dacZmod.allocBuffer(channel, length);

		dacZmod.setOutputSampleFrequencyDivider(channel, frequencyDivider);
		dacZmod.setGain(channel, gain);

		i = 0;
		// ramp up
		for(val = -amplitude; val < amplitude; val += step)
		{
			valRaw = dacZmod.getSignedRawFromVolt(val + offset, gain);
			buf[i++] = valRaw;
		}
		// ramp down
		for(val = amplitude; val > -amplitude; val -= step)
		{
			valRaw = dacZmod.getSignedRawFromVolt(val + offset, gain);
			buf[i++] = valRaw;
		}
	}
	// send data to DAC and start the instrument
	dacZmod.setData(channel, buf, length);
	dacZmod.start();
	for (uint16_t i = 0; i < 50; i++)
	{
		while (!dacZmod.isDMATransferComplete(channel)) {;}
		dacZmod.setData(channel, buf, length);
	}
	while (!dacZmod.isDMATransferComplete(channel)) {;}

	dacZmod.freeBuffer(channel, buf, length);

}


int main() {
	// offset 					2 V
	// amplitude 				3 V
	// step 					10 mV
	// channel 					CH1
	// Output Frequency Divider	2
	// gain						HIGH - corresponds to HIGH input Range
	dacRampDemo(2, 3, 0.01, 0, 2, 1);
    return 0;
}
