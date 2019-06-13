/*
 * "Hello World" example.
 *
 * This example prints 'Hello from Nios II' to the STDOUT stream. It runs on
 * the Nios II 'standard', 'full_featured', 'fast', and 'low_cost' example
 * designs. It runs with or without the MicroC/OS-II RTOS and requires a STDOUT
 * device in your system's hardware.
 * The memory footprint of this hosted application is ~69 kbytes by default
 * using the standard reference design.
 *
 * For a reduced footprint version of this template, and an explanation of how
 * to reduce the memory footprint for a given application, see the
 * "small_hello_world" template.
 *
 */

#include "alt_types.h"
#include <stdio.h>
#include <unistd.h>
#include "system.h"
#include "sys/alt_irq.h"
#include "altera_avalon_pio_regs.h"

#ifdef EGM_NAME
	void configureEGM(int);
	static void stimulus_in_ISR ();
#endif
void doInterrupt();
void doTightPolling();
int background();
int main()
{
	int switchVal;

	switchVal = IORD(SWITCH_PIO_BASE, 0) & 0x1;
	if (switchVal) {
		printf("Interrupt mode:\n");
	} else {
		printf("Tight polling mode:\n");
	}

	printf("Press B0 to start.\nPeriod,\tDutyC,\tBT,\tLat,\tMissed,\tMult\n");
	while((IORD(BUTTON_PIO_BASE, 0) & 0x01) != 0x00);
	IOWR(LED_PIO_BASE, 0, 2);
	if (switchVal) {
		doInterrupt();
	} else {
		doTightPolling();
	}
	IOWR(EGM_BASE, 0, 0); // disabling EGM
	IOWR(LED_PIO_BASE, 0, 0);
	return 0;
}

void doInterrupt() {
	alt_irq_register(STIMULUS_IN_IRQ, (void *)0, stimulus_in_ISR);
	IOWR(STIMULUS_IN_BASE,3,0x00); //clearing interrupt flag
	IOWR(STIMULUS_IN_BASE,2,0xFF); //unmasking
	int AverageLatency, missed, multi;
	int i;
	for(i = 1; i <= 750; i+= 1){
#ifdef EGM_NAME
		configureEGM(i);
		IOWR(EGM_BASE, 0, 1); // enabling EGM
#endif
		int totback = 0;
		while (IORD(EGM_BASE, 1) != 0){
			if(!background()){
				totback++;
			}
		}
		AverageLatency = IORD(EGM_BASE, 4);
		missed = IORD(EGM_BASE, 5);
		multi = IORD(EGM_BASE, 6);
		printf("%d,\t%d,\t%d,\t%d,\t%d,\t%d\n",i,2*i,totback,AverageLatency,missed,multi);
	}
}

void doTightPolling() {
	int i, total, AverageLatency, missed, multi;
	for (i = 1; i <= 750; i+= 1) {

		#ifdef EGM_NAME
			configureEGM(i);
			IOWR(EGM_BASE, 0, 1); // enabling EGM
			IOWR(RESPONSE_OUT_BASE,0,0x01);
			IOWR(RESPONSE_OUT_BASE,0,0x00);
		#endif
		int bck;
			total = 0;

			bck = 0;
			int previous = 1;
			while (!(IORD(STIMULUS_IN_BASE,0) & !previous)) {
				if (!IORD(STIMULUS_IN_BASE, 0)) {
					previous = 0;
				}
				bck++;
				background();
				total++;
			}
			bck-= 1;

		//IOWR(EGM_BASE, 0, 0);
		IOWR(RESPONSE_OUT_BASE,0,0x01);
		IOWR(RESPONSE_OUT_BASE,0,0x00);
		int responseSent = 1;
		//int current = 0;
		//int prev = 0;
		while (IORD(EGM_BASE, 1) != 0) {
			//current = IORD(STIMULUS_IN_BASE, 0);
			if (IORD(STIMULUS_IN_BASE, 0)){ //|| current && !prev) {
				if (responseSent == 0) {
					IOWR(RESPONSE_OUT_BASE,0,0x01);
					IOWR(RESPONSE_OUT_BASE,0,0x00);
					int j;
					for (j = 0; j < bck; j++) {
						background();
					}
					total += bck;
					responseSent = 1;
				}
			} else {
				responseSent = 0;
			}
			//prev =
		}

		AverageLatency = IORD(EGM_BASE, 4);
		missed = IORD(EGM_BASE, 5);
		multi = IORD(EGM_BASE, 6);
		//
		//printf("characterization period is %d\n", bck);
		printf("%d,\t%d,\t%d,\t%d,\t%d,\t%d\n",i,2*i,total,AverageLatency,missed,multi);
	}
}

int background()
{

	int j;
	int x = 0;
	int grainsize = 4;
	int g_taskProcessed = 0;
	int valLed = IORD(LED_PIO_BASE, 0);
	valLed++;
	IOWR(LED_PIO_BASE, 0, valLed);
	for(j = 0; j < grainsize; j++) {
		g_taskProcessed++;
	}
	valLed--;
	IOWR(LED_PIO_BASE, 0, valLed);
	return x;
}

void configureEGM(int pulseWidth) {
	IOWR(EGM_BASE, 0, 0);
	int period = pulseWidth * 2;
	IOWR(EGM_BASE, 2, period); //setting up period
	IOWR(EGM_BASE, 3, pulseWidth); //setting pulse_width

}

static void stimulus_in_ISR (){
	int valLed = IORD(LED_PIO_BASE, 0);
	valLed += 4;
	IOWR(LED_PIO_BASE, 0, valLed);
	IOWR(RESPONSE_OUT_BASE,0,0x01);//Register ISR
	IOWR(RESPONSE_OUT_BASE,0,0x00);//unmask
	IOWR(STIMULUS_IN_BASE,3,0x00); //clearing interrupt flag
	valLed -= 4;
	IOWR(LED_PIO_BASE, 0, valLed);
}
