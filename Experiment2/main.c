/*
 * UART_solution_struct.c
 *
 * Created: 10/5/2021 3:06:09 PM
 * Author : brrxmc, mem8c3, spk6f2
 */ 

#include <avr/io.h>
#include "board_struct.h"
#include "GPIO_Outputs.h"
#include "LEDS.h"
#include "UART.h"
#include <util/delay.h>
#include <avr/pgmspace.h>
#include "UART_Print.h"
#include "print_memory.h"
#include "Long_Serial_In.h"
#include "SPI.h"
#include <stdio.h>

uint32_t serial_input;

const char prompt_string[]  = "/n Enter block #: ";
char * print_buffer;


int main(void)
{
	//Initialize GPIO, UART, SPI, and SD Card
	GPIO_Output_Init(SD_CS_port, SD_CS_pin);
	UART_Init(UART1, 9600);
	SPI_Master_Init(SPI0, 400000);
	SD_Init();
	print_buffer = Export_print_buffer();

	//Prompts user for input, uses input to read block #
	for(;;)
	{
			sprintf(print_buffer, "Enter block #: ", prompt_string);
			UART_Transmit_String(UART1, 0, print_buffer);
			GPIO_Output_Clear(SD_CS_port, SD_CS_pin);
			serial_input = Long_Serial_Input(UART1);
			error_flag=Send_Command(CMD17,serial_input);
			error_flag=read_block(SPI0, 512, data_array);
			print_memory(UART1, 512, data_array);
	}	
}
