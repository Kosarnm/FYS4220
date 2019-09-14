/*************************************************************************
* Copyright (c) 2004 Altera Corporation, San Jose, California, USA.      *
* All rights reserved. All use of this software and documentation is     *
* subject to the License Agreement located at the end of this file below.*
**************************************************************************
* Description:                                                           *
* The following is a simple hello world program running MicroC/OS-II.The * 
* purpose of the design is to be a very simple application that just     *
* demonstrates MicroC/OS-II running on NIOS II.The design doesn't account*
* for issues such as checking system call return codes. etc.             *
*                                                                        *
* Requirements:                                                          *
*   -Supported Example Hardware Platforms                                *
*     Standard                                                           *
*     Full Featured                                                      *
*     Low Cost                                                           *
*   -Supported Development Boards                                        *
*     Nios II Development Board, Stratix II Edition                      *
*     Nios Development Board, Stratix Professional Edition               *
*     Nios Development Board, Stratix Edition                            *
*     Nios Development Board, Cyclone Edition                            *
*   -System Library Settings                                             *
*     RTOS Type - MicroC/OS-II                                           *
*     Periodic System Timer                                              *
*   -Know Issues                                                         *
*     If this design is run on the ISS, terminal output will take several*
*     minutes per iteration.                                             *
**************************************************************************/


#include <stdio.h>
#include <string.h>
#include "includes.h"

#include "altera_avalon_pio_regs.h"
#include "io.h"
#include "alt_types.h"
#include "system.h"
#include "sys/alt_irq.h"
#include "i2c_avalon_wrapper.h"


volatile alt_isr_func isr_func(void * context);



/* Definition of Task Stacks */
#define   TASK_STACKSIZE       2048
OS_STK    task1_stk[TASK_STACKSIZE];
OS_STK    task2_stk[TASK_STACKSIZE];
OS_STK    task_temperature_stk[TASK_STACKSIZE];
OS_STK    task_interrupt_stk[TASK_STACKSIZE];
OS_STK    task_tmp175alert_stk[TASK_STACKSIZE];
/* Definition of Task Priorities */

#define TASK_INTERRUPT_PRIORITY     1
#define TASK_TEMPERATURE_PRIORITY   2
#define TASK_TMP17_5ALERT_PRIORITY	3
#define TASK1_PRIORITY      		4
#define TASK2_PRIORITY      		5

/* Semaphores */
OS_EVENT *shared_jtag_sem;
OS_EVENT *key1_sem;
OS_EVENT *tmp175alert_sem;
OS_EVENT *MSG_box;

//TMP175 function

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








/* Prints "Hello World" and sleeps for three seconds */
void task1(void* pdata)
{
	int t_start;
	int t_end;
	int t_prev = 0;
	INT8U error_code = OS_NO_ERR;

  while (1)
  { 
		t_start = OSTimeGet();
		char text1[] = "Hello from Task1\n";
		//ask wait for semaphore
		OSSemPend(shared_jtag_sem, 0 , &error_code);
		for (int i = 0; i<strlen(text1);i++){
			putchar(text1[i]);
		}
		t_end = OSTimeGet();
		printf("T1: (Start, End, Ex.T.,P: (%d , %d, %d, %d) \n", t_start, t_end,t_end-t_start, t_start-t_prev);
		OSSemPost(shared_jtag_sem);
		t_prev = t_start;
		OSTimeDlyHMSM(0, 0, 5, 0);
  }
}
/* Prints "Hello World" and sleeps for three seconds */
void task2(void* pdata)
{
	int t_start;
	int t_end;
	int t_prev = 0;
	INT8U error_code = OS_NO_ERR;

  while (1)
  {
		t_start = OSTimeGet();
		char text2[] = "Hello from Task2\n";
		//ask wait for semaphore
		OSSemPend(shared_jtag_sem, 0 , &error_code);
		for (int i = 0; i<strlen(text2);i++){
			putchar(text2[i]);
		}
		t_end = OSTimeGet();
		printf("T2: (Start, End, Ex.T.,P: (%d , %d, %d, %d) \n", t_start, t_end,t_end-t_start, t_start-t_prev);
		OSSemPost(shared_jtag_sem);
		t_prev = t_start;
		OSTimeDlyHMSM(0, 0, 5, 0);




  }
}


void task_temperature(void* pdata)
{

	int t_start;
	int t_end;
	int t_prev = 0;
	INT8U error_code = OS_NO_ERR;

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

	int * mailbox_data_in = 0;
	int timeout = 5000; //intial timeout value
	while(1){

		//read mailbox data
		mailbox_data_in = (alt_16*)OSMboxPend(MSG_box,timeout,&error_code);
		if(error_code != OS_ERR_TIMEOUT){
			timeout = *mailbox_data_in*100;
		}
		digitalVal = TMP175_ReadTemperature_DigitalValue();
		float tempCelc = digitalVal*0.0625f;

		OSSemPend(shared_jtag_sem, 0 , &error_code);
		printf("\n\nTemperature : %f degree celcius\r\n", tempCelc);
		printf("next read will be in : %d ms\r\n\n", timeout);
		OSSemPost(shared_jtag_sem);
	}
}



/* Prints "Hello World" and sleeps for three seconds */
void task_interrupt(void* pdata)
{

	INT8U error_code = OS_NO_ERR;


	int mailbox_data;

	while (1)
	{
	  //wait until the ke1_sem is posted from ISR
	  OSSemPend(key1_sem, 0 , &error_code);


	  //read value from switches
	  mailbox_data = ReadSwitches();
	  //store and send this value using mailbox to temperature task
	  error_code = OSMboxPost(MSG_box,&mailbox_data);




	  OSSemPend(shared_jtag_sem, 0 , &error_code);
	  printf("\nHello from Interrupt!\n");
	  OSSemPost(shared_jtag_sem);

	//

	}
}


/* Prints "Hello World" and sleeps for three seconds */
void task_alertFromTMP175(void* pdata)
{

INT8U error_code = OS_NO_ERR;

  while (1)
  {
	  //wait until the ke1_sem is posted from ISR
	  OSSemPend(tmp175alert_sem, 0 , &error_code);

	  OSSemPend(shared_jtag_sem, 0 , &error_code);
	  printf("\nHIGH TEMPERATURE ALERT!\n");
	  OSSemPost(shared_jtag_sem);



  }
}



/* The main function creates two task and starts multi-tasking */
int main(void)
{
	volatile int edge_capture = 0;

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









IOWR_ALTERA_AVALON_PIO_IRQ_MASK(PIO_EXT_BASE, 0x0F);

  printf("MicroC/OS-II Licensing Terms\n");
  printf("============================\n");
  printf("Micrium\'s uC/OS-II is a real-time operating system (RTOS) available in source code.\n");
  printf("This is not open-source software.\n");
  printf("This RTOS can be used free of charge only for non-commercial purposes and academic projects,\n");
  printf("any other use of the code is subject to the terms of an end-user license agreement\n");
  printf("for more information please see the license files included in the BSP project or contact Micrium.\n");
  printf("Anyone planning to use a Micrium RTOS in a commercial product must purchase a commercial license\n");
  printf("from the owner of the software, Silicon Laboratories Inc.\n");
  printf("Licensing information is available at:\n");
  printf("Phone: +1 954-217-2036\n");
  printf("Email: sales@micrium.com\n");
  printf("URL: www.micrium.com\n\n\n");  

  OSTaskCreateExt(task1,
                  NULL,
                  (void *)&task1_stk[TASK_STACKSIZE-1],
                  TASK1_PRIORITY,
                  TASK1_PRIORITY,
                  task1_stk,
                  TASK_STACKSIZE,
                  NULL,
                  0);
              
               
  OSTaskCreateExt(task2,
                  NULL,
                  (void *)&task2_stk[TASK_STACKSIZE-1],
                  TASK2_PRIORITY,
                  TASK2_PRIORITY,
                  task2_stk,
                  TASK_STACKSIZE,
                  NULL,
                  0);


  OSTaskCreateExt(task_temperature,
                  NULL,
                  (void *)&task_temperature_stk[TASK_STACKSIZE-1],
                  TASK_TEMPERATURE_PRIORITY,
				  TASK_TEMPERATURE_PRIORITY,
                  task_temperature_stk,
                  TASK_STACKSIZE,
                  NULL,
                  0);

  OSTaskCreateExt(task_interrupt,
                  NULL,
                  (void *)&task_interrupt_stk[TASK_STACKSIZE-1],
                  TASK_INTERRUPT_PRIORITY,
				  TASK_INTERRUPT_PRIORITY,
				  task_interrupt_stk,
                  TASK_STACKSIZE,
                  NULL,
                  0);

  OSTaskCreateExt(task_alertFromTMP175,
                  NULL,
                  (void *)&task_tmp175alert_stk[TASK_STACKSIZE-1],
                  TASK_TMP17_5ALERT_PRIORITY,
				  TASK_TMP17_5ALERT_PRIORITY,
				  task_tmp175alert_stk,
                  TASK_STACKSIZE,
                  NULL,
                  0);




  //initializing the semaphores
  shared_jtag_sem = OSSemCreate(1);
  key1_sem = OSSemCreate(0);
  tmp175alert_sem = OSSemCreate(0);

  //creating an empty mailbox
  MSG_box = OSMboxCreate((void*)NULL );


  OSStart();
  return 0;
}





volatile alt_isr_func isr_func(void * context){
	INT8U error_code = OS_NO_ERR;
	switch(IORD_ALTERA_AVALON_PIO_EDGE_CAP(PIO_EXT_BASE)){

		case (1<<0): OSSemPost(key1_sem);break;
		//case (1<<1): printf("\t\t\tkey 2 pressed\r\n"); break;
		//case (1<<2): printf("\t\t\tkey 3 pressed\r\n"); break;
		case (1<<3): OSSemPost(tmp175alert_sem);; break;
		default: printf("unknown interrupt\r\n"); break;
	}

	//clearing the interrupt flags
	IOWR_ALTERA_AVALON_PIO_EDGE_CAP(PIO_EXT_BASE , 0);

	 return 0;

}

/******************************************************************************
*                                                                             *
* License Agreement                                                           *
*                                                                             *
* Copyright (c) 2004 Altera Corporation, San Jose, California, USA.           *
* All rights reserved.                                                        *
*                                                                             *
* Permission is hereby granted, free of charge, to any person obtaining a     *
* copy of this software and associated documentation files (the "Software"),  *
* to deal in the Software without restriction, including without limitation   *
* the rights to use, copy, modify, merge, publish, distribute, sublicense,    *
* and/or sell copies of the Software, and to permit persons to whom the       *
* Software is furnished to do so, subject to the following conditions:        *
*                                                                             *
* The above copyright notice and this permission notice shall be included in  *
* all copies or substantial portions of the Software.                         *
*                                                                             *
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR  *
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,    *
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE *
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER      *
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING     *
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER         *
* DEALINGS IN THE SOFTWARE.                                                   *
*                                                                             *
* This agreement shall be governed in all respects by the laws of the State   *
* of California and by the laws of the United States of America.              *
* Altera does not recommend, suggest or require that this reference design    *
* file be used in conjunction or combination with any other product.          *
******************************************************************************/
