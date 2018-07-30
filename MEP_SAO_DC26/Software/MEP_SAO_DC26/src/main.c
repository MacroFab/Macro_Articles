#include <si_toolchain.h>
#include <SI_EFM8SB1_Register_Enums.h>
#include "InitDevice.h"

void delay_ms(int);
void test_pattern(void);
void reset_leds(void);
void all_on(void);

void pattern_chase();
void alternate();
void count();
void random();

volatile int mS_counter = 0;
volatile int LED_OUTPUT = 0;

int count_delay = 0;

void main(void)
{
	enter_DefaultMode_from_RESET();

	reset_leds();

	IE_EA = 1;

	while(1)
	{
		test_pattern();
		reset_leds();

		random();
		reset_leds();

		pattern_chase();
		reset_leds();

		alternate();
		reset_leds();
		alternate();
		reset_leds();
		alternate();
		reset_leds();

		count();
		reset_leds();

		all_on();
		reset_leds();

		all_on();
		reset_leds();
	}
	return;
}

void random()
{
	int i;
	for(i = 0; i < 50; i++)
	{
		LED_OUTPUT = rand();
		delay_ms(100);
	}
}

void count()
{
	count_delay = 25;
	while(LED_OUTPUT < 32767)
	{
		LED_OUTPUT++;
		delay_ms(count_delay);
		if((count_delay > 2) && (LED_OUTPUT % 128 == 0))
		{
			count_delay--;
		}
	}
	return;
}

void alternate()
{
	LED_OUTPUT = 0x1FFF;
	delay_ms(200);

	LED_OUTPUT = 0x6000;
	delay_ms(200);

	return;
}

void pattern_chase()
{
	LED_OUTPUT = 0x0001;
	delay_ms(200);

	LED_OUTPUT = 0x0003;
	delay_ms(200);

	LED_OUTPUT = LED_OUTPUT << 1;
	delay_ms(200);

	LED_OUTPUT = LED_OUTPUT << 1;
	delay_ms(200);

	LED_OUTPUT = LED_OUTPUT << 1;
	delay_ms(200);

	LED_OUTPUT = LED_OUTPUT << 1;
	delay_ms(200);

	LED_OUTPUT = LED_OUTPUT << 1;
	delay_ms(200);

	LED_OUTPUT = LED_OUTPUT << 1;
	delay_ms(200);

	LED_OUTPUT = LED_OUTPUT << 1;
	delay_ms(200);

	LED_OUTPUT = LED_OUTPUT << 2;
	delay_ms(200);

	LED_OUTPUT = LED_OUTPUT << 2;
	delay_ms(200);

	LED_OUTPUT = 0x2000;
	delay_ms(200);

	LED_OUTPUT = LED_OUTPUT << 1;
	delay_ms(200);

	return;
}

void reset_leds()
{
	LED_OUTPUT = 0x0000;
	delay_ms(200);

	return;
}

void all_on()
{
	LED_OUTPUT = 0xFFFF;
	delay_ms(200);

	return;
}

void test_pattern()
{
	LED_OUTPUT = 0x0001;
	delay_ms(100);

	LED_OUTPUT = LED_OUTPUT << 1;
	delay_ms(100);

	LED_OUTPUT = LED_OUTPUT << 1;
	delay_ms(100);

	LED_OUTPUT = LED_OUTPUT << 1;
	delay_ms(100);

	LED_OUTPUT = LED_OUTPUT << 1;
	delay_ms(100);

	LED_OUTPUT = LED_OUTPUT << 1;
	delay_ms(100);

	LED_OUTPUT = LED_OUTPUT << 1;
	delay_ms(100);

	LED_OUTPUT = LED_OUTPUT << 1;
	delay_ms(100);

	LED_OUTPUT = LED_OUTPUT << 1;
	delay_ms(100);

	LED_OUTPUT = LED_OUTPUT << 1;
	delay_ms(100);

	LED_OUTPUT = LED_OUTPUT << 1;
	delay_ms(100);

	LED_OUTPUT = LED_OUTPUT << 1;
	delay_ms(100);

	LED_OUTPUT = LED_OUTPUT << 1;
	delay_ms(100);

	LED_OUTPUT = LED_OUTPUT << 1;
	delay_ms(100);

	LED_OUTPUT = LED_OUTPUT << 1;
	delay_ms(100);

	return;
}

void delay_ms(int milliseconds)
{
	milliseconds = milliseconds + mS_counter;
	while((milliseconds != mS_counter) || (milliseconds > mS_counter));
	return;
}
