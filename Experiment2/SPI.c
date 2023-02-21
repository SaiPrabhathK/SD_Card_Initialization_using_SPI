/*
* SPI.c
*
* Created: 10/16/2022 4:10:59 PM
*  Author: brrxmc
*/
#include "board_struct.h"
#include "GPIO_Outputs.h"
#include "SPI.h"

uint8_t receive_response(uint8_t num_bytes,uint8_t * rec_array);

uint8_t rcvd_val;
uint8_t return_value;
uint8_t SD_card_type = ' ';

uint8_t error_status;

//rcvd_val = SPI_Transfer(&SPI0, send_val,&error_flag)
void set_error_status(){
	error_status= receive_response(5,rec_values);
}

//Initializes SPI
uint8_t SPI_Master_Init(volatile SPI_t *SPI_base, uint32_t clock_rate)
{
	uint16_t divider = (F_CPU/F_DIV)/(clock_rate);
	if (divider<2){
		SPI_base->SPCR = ((1<<SPE)|(1<<MSTR)|(CPOL_BIT<<CPOL)| (CPHA_BIT<<CPHA)|0<<0);
		SPI_base->SPSR = 1;
	}
	else if (divider<4){
		SPI_base->SPCR = ((1<<SPE)|(1<<MSTR)|(CPOL_BIT<<CPOL)| (CPHA_BIT<<CPHA)|0<<0);
		SPI_base->SPSR = 0;
	}
	else if (divider<8){
		SPI_base->SPCR = ((1<<SPE)|(1<<MSTR)|(CPOL_BIT<<CPOL)| (CPHA_BIT<<CPHA)|1<<0);
		SPI_base->SPSR = 1;
	}
	else if (divider<16){
		SPI_base->SPCR = ((1<<SPE)|(1<<MSTR)|(CPOL_BIT<<CPOL)| (CPHA_BIT<<CPHA)|1<<0);
		SPI_base->SPSR = 0;
	}
	else if (divider<32){
		SPI_base->SPCR = ((1<<SPE)|(1<<MSTR)|(CPOL_BIT<<CPOL)| (CPHA_BIT<<CPHA)|2<<0);
		SPI_base->SPSR = 1;
	}
	else if (divider<64){
		SPI_base->SPCR = ((1<<SPE)|(1<<MSTR)|(CPOL_BIT<<CPOL)| (CPHA_BIT<<CPHA)|2<<0);
		SPI_base->SPSR = 0;
	}
	else if (divider<128){
		SPI_base->SPCR = ((1<<SPE)|(1<<MSTR)|(CPOL_BIT<<CPOL)| (CPHA_BIT<<CPHA)|3<<0);
		SPI_base->SPSR = 0;
	}
	else{
		return_value = clock_rate_error;
	}
	if (SPI_base == SPI0){
		GPIO_Output_Set(PB, (1<<5));
		GPIO_Output_Init(PB, (1<<5));
	}
	else{
		GPIO_Output_Set(PB, (1<<3));
		GPIO_Output_Init(PB, (1<<3));
	}
	if(CPOL_BIT == 0){
		GPIO_Output_Clear(PB, (1<<7));
		GPIO_Output_Init(PB, (1<<7));
	}
	else{
		GPIO_Output_Clear(PD, (1<<7));
		GPIO_Output_Init(PD, (1<<7));
	}
	return return_value;
	
}

//Writes to SPI data register
uint8_t SPI_Transfer( volatile SPI_t *SPI_base, uint8_t send_value)
{
	uint8_t status;
	// First start a transfer by writing send_value to SPDR
	(SPI_base->SPDR) = send_value;
	// Next wait in a loop until SPIF is set
	do
	{
		status= (SPI_base->SPSR);
	}while((status&0x80)==0);
	// Then return the value from SPDR
	return SPI_base->SPDR;
}

//Sends command to SD Card using SPI transfer
uint8_t Send_Command ( uint8_t command, uint32_t argument){
	uint8_t send_value;
	if(command<64)
	{
		return_value = no_errors;
		send_value = STARTT|command;
		rcvd_val=SPI_Transfer(SD_SPI_port, send_value);
	}
	else{
		return_value = illegal_command;
		return return_value;
	}
	for(uint8_t index=0;index<4;index++)
	{
		send_value=(uint8_t)(argument>>(24-(index*8)));
		rcvd_val=SPI_Transfer(SD_SPI_port,send_value);
	}
	
	// The final byte to send is determined by the CMD_value.
	if(command==CMD0)
	{
		send_value=CRC7_CMD0;
	}
	else if(command==CMD8)
	{
		send_value=CRC7_CMD8;
	}
	else
	{
		send_value=0x01; // end bit only, CRC7=0
	}
	rcvd_val=SPI_Transfer(SD_SPI_port,send_value);
	// Return the error flag.
	return return_value;
	
}

//Receives response from SD Card using SPI Transfer
uint8_t receive_response(uint8_t num_bytes,uint8_t * rec_array){
	return_value = no_errors;
	uint8_t timeout = 0;
	do
	{
		rcvd_val=SPI_Transfer(SD_SPI_port,0xFF); // SPI_Receive
		timeout++;
	}while((rcvd_val==0xFF)&&(timeout!=0));
	// Check for SPI error, timeout error or communication error
	if(timeout==0)
	{
		return_value=SD_timeout_error;
	}
	else if(!((rcvd_val&0xFE)== 0x00 || (rcvd_val&0xFE)== 0x01)) // 0x00 and 0x01 are good values
	{
		*rec_array=rcvd_val; // return the value to see the error
		return_value=SD_comm_error;
	}
	else// Receive the rest of the bytes (if there are more to receive).
	{
		*rec_array=rcvd_val; // first received value (R1 resp.)
		if(num_bytes>1)
		{
			for(uint8_t index=1;index<num_bytes;index++)
			{
				rcvd_val=SPI_Transfer(SD_SPI_port,0xFF);
				*(rec_array+index)=rcvd_val;
			}
		}
	}
	rcvd_val=SPI_Transfer(SD_SPI_port,0xFF);
	return return_value;
}

//Initializes SD Card
uint8_t SD_Init(void)
{
	uint32_t ACMD41_arg = 0x00000000;
	uint8_t timeout = 0;
	GPIO_Output_Set(SD_CS_port, SD_CS_pin);
	for(uint8_t i = 1; i<=10; i++)
	{
		SPI_Transfer(SPI0, 0xFF);
	}
	
	GPIO_Output_Clear(SD_CS_port, SD_CS_pin);
	Send_Command(CMD0, 0x00);
	error_status = receive_response(5, rec_values);
	
	if(error_status==no_errors)
	{		GPIO_Output_Clear(SD_CS_port,SD_CS_pin);		error_flag = Send_Command(CMD8, 0x000001AA);		if(error_flag==no_errors)
		{
			error_flag=receive_response(5,rec_values);
		}
		
		GPIO_Output_Set(SD_CS_port, SD_CS_pin);		if((rec_values[0]==0x01)&&(error_flag==no_errors))
		{
			//Check voltage compatibility:
			if((rec_values[0]==0x01)&&(rec_values[4]==0xAA))
			{
				ACMD41_arg=0x40000000; // High-Capacity Support
				GPIO_Output_Clear(SD_CS_port,SD_CS_pin);
				Send_Command(CMD58, 0x00);
				error_flag=receive_response(5,rec_values);
				GPIO_Output_Set(SD_CS_port, SD_CS_pin);
				//check R1 response is 0x01
				if((rec_values[0]==0x01))
				{
					//check bit 20 and 21 in the r3 response
					if(rec_values[2] & 0x4 || rec_values[2] & 0x5)
					{
						GPIO_Output_Clear(SD_CS_port,SD_CS_pin);

						do
						{
							Send_Command(CMD55, 0x00);
							error_flag= receive_response(5, rec_values);
							Send_Command(CMD41, ACMD41_arg);
							error_flag= receive_response(5, rec_values);
							timeout++;
						}while(rec_values[0]!=0x00 && timeout !=0);
						
						if(return_value==no_errors)
						{
							Send_Command(CMD58, 0x00);
							error_flag = receive_response(5, rec_values);
							if (rec_values[1] & 0x80)
							{
								if(rec_values[1] & 0x40 )
								{
									SD_card_type = 'h';
								}
								else
								{
									SD_card_type = 's';
								}
							}
						}
					}
				}
			}
			else
			{
				error_status= incompatible_voltage;
			}
		}
		else
		{
			error_status=illegal_command;
		}
	}	else if(rec_values[0]==0x05)
	{
		error_status=no_errors; // if supporting older cards
		ACMD41_arg=0x00000000; // No High-Capacity Support
	}	else
	{
		//Return a value that would help determine the problem
		error_status=error_flag;
	}	return error_status;}//Reads block from SD Carduint8_t read_block( volatile SPI_t *SPI_base, uint16_t num_bytes, uint8_t *array){	do
	{
		rcvd_val=SPI_Transfer(SPI_base,0xFF); //SPI_Receive
	}while(rcvd_val==0xFF);		if(rcvd_val == 0x00)	{		do
		{
			rcvd_val=SPI_Transfer(SPI_base,0xFF);
		}while(rcvd_val==0xFF);				if(rcvd_val == 0xFE)		{			for(uint16_t index=0; index<num_bytes; index++)  //index must 16-bits
			{
				rcvd_val=SPI_Transfer(SPI_base,0xFF);
				*(array+index)=rcvd_val;
			}			//Receives the CRC16 and discards, then sends one final SPI transfer			rcvd_val=SPI_Transfer(SPI_base,0xFF);
			rcvd_val=SPI_Transfer(SPI_base,0xFF);
			rcvd_val=SPI_Transfer(SPI_base,0xFF);		}		else		{			return start_token_error;		}	}	}
