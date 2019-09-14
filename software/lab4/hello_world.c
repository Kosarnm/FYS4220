#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <stdint.h>


#include "altera_avalon_pio_regs.h"
#include "io.h"
#include "alt_types.h"
#include "system.h"
#include "sys/alt_irq.h"
#include "i2c_avalon_wrapper.h"


#define DEBUG_ENABLED 1

//set the temperature (in degree celcius) at which you expect an alert interrupt
#define HIGH_ALERT_TEMPERATURE_CELC 27.3





#define TMP175_CONFIG_SHUTDOWN_MODE_bp		0
#define TMP175_CONFIG_SHUTDOWN_MODE_bm		(1<<TMP175_CONFIG_SHUTDOWN_MODE_bp)

#define TMP175_CONFIG_THERMOSTAT_MODE_bp	1
#define TMP175_CONFIG_THERMOSTAT_MODE_bm	(1<<TMP175_CONFIG_THERMOSTAT_MODE_bp)

#define TMP175_CONFIG_POLARITY_bp			2
#define TMP175_CONFIG_POLARITY_bm			(1<<TMP175_CONFIG_POLARITY_bp)

#define TMP175_CONFIG_FAULT_QUEUE_gp		3
#define TMP175_CONFIG_FAULT_QUEUE_gm		(0b11<<TMP175_CONFIG_FAULT_QUEUE_gp)

#define TMP175_CONFIG_CONV_RES_gp			5
#define TMP175_CONFIG_CONV_RES_gm			(0b11<<TMP175_CONFIG_CONV_RES_gp)

#define TMP175_CONFIG_ONE_SHOT_bp			7
#define TMP175_CONFIG_ONE_SHOT_bm			(1<<TMP175_CONFIG_ONE_SHOT_bp)


typedef enum {
	TMP175_shutdownMode_disable = (0<<TMP175_CONFIG_SHUTDOWN_MODE_bp),
	TMP175_shutdownMode_enable = (1<<TMP175_CONFIG_SHUTDOWN_MODE_bp),
}tmp175_shutdownMode_t;


typedef enum {
	TMP175_THERMOSTAT_MODE_COMP = (0<<TMP175_CONFIG_THERMOSTAT_MODE_bp),
	TMP175_THERMOSTAT_MODE_INTP = (1<<TMP175_CONFIG_THERMOSTAT_MODE_bp),
}tmp175_thermostatMode_t;

typedef enum {
	TMP175_POLARITY_ACTIVE_LOW = (0<<TMP175_CONFIG_POLARITY_bp),
	TMP175_POLARITY_ACTIVE_HIGH = (1<<TMP175_CONFIG_POLARITY_bp),
}tmp175_polarity_t;

typedef enum {
	TMP175_FAULT_QUEUE_1= (0<<TMP175_CONFIG_FAULT_QUEUE_gp),
	TMP175_FAULT_QUEUE_2= (1<<TMP175_CONFIG_FAULT_QUEUE_gp),
	TMP175_FAULT_QUEUE_4= (2<<TMP175_CONFIG_FAULT_QUEUE_gp),
	TMP175_FAULT_QUEUE_6= (3<<TMP175_CONFIG_FAULT_QUEUE_gp),
}tmp175_falutQueue_t;

typedef enum {
	TMP175_CONV_RES_9bit  = (0<<TMP175_CONFIG_CONV_RES_gp),
	TMP175_CONV_RES_10bit = (1<<TMP175_CONFIG_CONV_RES_gp),
	TMP175_CONV_RES_11bit = (2<<TMP175_CONFIG_CONV_RES_gp),
	TMP175_CONV_RES_12bit = (3<<TMP175_CONFIG_CONV_RES_gp),
}tmp175_convRes_t;

typedef enum {
	TMP175_ONE_SHOT_DISABLE= (0<<TMP175_CONFIG_ONE_SHOT_bp),
	TMP175_ONE_SHOT_ENABLE= (1<<TMP175_CONFIG_ONE_SHOT_bp),
}tmp175_oneShot_t;



typedef struct{
	tmp175_shutdownMode_t shutdownMode;
	tmp175_thermostatMode_t thermostatMode;
	tmp175_polarity_t polarity;
	tmp175_falutQueue_t faultQueue;
	tmp175_convRes_t convRes;
	tmp175_oneShot_t oneShot;
	alt_16 T_HIGH;
	alt_16 T_LOW;
}TMP175_config_t;


alt_8 TMP175_Read_ConfigReg(){
	return read_from_i2c_device(TMP175_ADDR,TMP175_CONFIG_REG,I2C_ONE_BYTE);
}

void TMP175_Write_ConfigReg(alt_8 data){
	write_to_i2c_device(TMP175_ADDR, TMP175_CONFIG_REG ,I2C_ONE_BYTE, data);
}


void TMP175_shutdownMode(tmp175_thermostatMode_t mode){
	alt_8 configReg;
	configReg = TMP175_Read_ConfigReg();
	configReg &= ~(TMP175_CONFIG_SHUTDOWN_MODE_bm);
	configReg |= mode;
	TMP175_Write_ConfigReg(configReg);
}

void TMP175_ThermostatMode(tmp175_shutdownMode_t mode){
	alt_8 configReg;
	configReg = TMP175_Read_ConfigReg();
	configReg &= ~(TMP175_CONFIG_THERMOSTAT_MODE_bm);
	configReg |= mode;
	TMP175_Write_ConfigReg(configReg);
}

void TMP175_Polarity(tmp175_polarity_t pol){
	alt_8 configReg;
	configReg = TMP175_Read_ConfigReg();
	configReg &= ~TMP175_CONFIG_POLARITY_bm;
	configReg |= pol;
	TMP175_Write_ConfigReg(configReg);
}

void TMP175_FaultQueue(tmp175_falutQueue_t faultQueue){
	alt_8 configReg;
	configReg = TMP175_Read_ConfigReg();
	configReg &= ~TMP175_CONFIG_FAULT_QUEUE_gm;
	configReg |= faultQueue;
	TMP175_Write_ConfigReg(configReg);
}

void TMP175_convRes(tmp175_convRes_t convRes){
	alt_8 configReg;
	configReg = TMP175_Read_ConfigReg();
	configReg &= ~TMP175_CONFIG_CONV_RES_gm;
	configReg |= convRes;
	TMP175_Write_ConfigReg(configReg);
}

void TMP175_oneShot(tmp175_oneShot_t sel){
	alt_8 configReg;
	configReg = TMP175_Read_ConfigReg();
	configReg &= ~TMP175_CONFIG_ONE_SHOT_bm;
	configReg |= sel;
	TMP175_Write_ConfigReg(configReg);
}


void TMP175_write_T_HIGH(alt_16 data){
	write_to_i2c_device(TMP175_ADDR, TMP175_THIGH_REG ,I2C_TWO_BYTE, data);
}

void TMP175_write_T_LOW(alt_16 data){
	write_to_i2c_device(TMP175_ADDR, TMP175_TLOW_REG ,I2C_TWO_BYTE, data);
}


alt_16 TMP175_ReadTemperature_DigitalValue(){
	return (((read_from_i2c_device(TMP175_ADDR, TMP175_TEMP_REG, I2C_TWO_BYTE)) >> 4)&0xFFF) ;
}






void TMP175_Configure(TMP175_config_t *config){

	TMP175_shutdownMode(config->shutdownMode);
	TMP175_ThermostatMode(config->thermostatMode);
	TMP175_Polarity(config->polarity );
	TMP175_FaultQueue( config->faultQueue);
	TMP175_convRes(config->convRes );
	TMP175_oneShot(config->oneShot );
	TMP175_write_T_HIGH(config->T_HIGH);
	TMP175_write_T_LOW(config->T_LOW);
}




void writeLed(alt_16 val){
	IOWR(LED_PIO_BASE, 0,val );
}

alt_16 ReadSwitches(){
	return IORD(SW_PIO_BASE, 0);
}



volatile alt_isr_func isr_func(void * context){

	switch(IORD_ALTERA_AVALON_PIO_EDGE_CAP(PIO_EXT_BASE)){

		case (1<<0): printf("\t\t\tkey 1 pressed\r\n"); break;
		case (1<<1): printf("\t\t\tkey 2 pressed\r\n"); break;
		case (1<<2): printf("\t\t\tkey 3 pressed\r\n"); break;
		case (1<<3): printf("\t\t\tTemperature High Alert \r\n"); break;
		default: printf("unknown interrupt\r\n"); break;
	}

	//clearing the interrupt flags
	IOWR_ALTERA_AVALON_PIO_EDGE_CAP(PIO_EXT_BASE , 0);

	 return 0;

}



void delay_s(unsigned int sec){
	usleep(1000000*sec);
}

void delay_ms(unsigned int milliSec){
	usleep(1000*milliSec);
}

int main()
{
	printf("Hello from Adam and Habib ;-) ;-) ;-) \n");
	volatile int edge_capture = 0;

	float high_alert_temp_raw = HIGH_ALERT_TEMPERATURE_CELC*258;



	//Doing some default configuration

	TMP175_config_t tmp175configuration;

	tmp175configuration.shutdownMode = TMP175_shutdownMode_disable;
	tmp175configuration.thermostatMode = TMP175_THERMOSTAT_MODE_COMP;
	tmp175configuration.polarity = TMP175_POLARITY_ACTIVE_LOW;
	tmp175configuration.faultQueue = TMP175_FAULT_QUEUE_1;
	tmp175configuration.convRes = TMP175_CONV_RES_12bit;
	tmp175configuration.oneShot = TMP175_ONE_SHOT_DISABLE;
	tmp175configuration.T_HIGH = high_alert_temp_raw;
	tmp175configuration.T_LOW = high_alert_temp_raw;

	TMP175_Configure(&tmp175configuration);


	alt_16 digitalVal;

 	//enabling the interrupt keys and alert signal
 	IOWR_ALTERA_AVALON_PIO_IRQ_MASK(PIO_EXT_BASE, 0x0F);

 	//clearing the interrupt flags
 	IOWR_ALTERA_AVALON_PIO_EDGE_CAP(PIO_EXT_BASE, 0);


 	//creating the interrupt and assigning function and context pointer
 	int err = alt_ic_isr_register(
 			PIO_EXT_IRQ_INTERRUPT_CONTROLLER_ID,
			PIO_EXT_IRQ,
			(void*) isr_func,
			(void*) edge_capture ,
			0x00
 			);

 	if(err == 0){
 		printf("interrupt created successfully\n");
 	}
 	else{
 		printf("interrupt creation failed\n");
 	}

 	//enabling the interrupt
 	alt_ic_irq_enabled(PIO_EXT_IRQ_INTERRUPT_CONTROLLER_ID,PIO_EXT_IRQ);


 	while(1){

 		writeLed(ReadSwitches());

 		digitalVal = TMP175_ReadTemperature_DigitalValue();

 		int tempInt = digitalVal*0.0625;
 		int tempDec = (int) (digitalVal*0.0625*1000) - tempInt*1000;

 		printf("Temperature : %d.%d degree celcius\r\n", tempInt, tempDec);

 		delay_ms(100);

 	}


  return 0;
}





