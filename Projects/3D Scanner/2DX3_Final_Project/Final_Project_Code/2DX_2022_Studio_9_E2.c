/*  

Modified March 28, 2025
by Ricky Huang

*/
#include <stdint.h>
#include "PLL.h"
#include "SysTick.h"
#include "uart.h"
#include "onboardLEDs.h"
#include "tm4c1294ncpdt.h"
#include "VL53L1X_api.h"


#define I2C_MCS_ACK             0x00000008  // Data Acknowledge Enable
#define I2C_MCS_DATACK          0x00000008  // Acknowledge Data
#define I2C_MCS_ADRACK          0x00000004  // Acknowledge Address
#define I2C_MCS_STOP            0x00000004  // Generate STOP
#define I2C_MCS_START           0x00000002  // Generate START
#define I2C_MCS_ERROR           0x00000002  // Error
#define I2C_MCS_RUN             0x00000001  // I2C Master Enable
#define I2C_MCS_BUSY            0x00000001  // I2C Busy
#define I2C_MCR_MFE             0x00000010  // I2C Master Function Enable

#define MAXRETRIES              5           // number of receive attempts before giving up

uint32_t motorStatus = 1;
uint32_t dataAcquisitionStatus = 1;

void I2C_Init(void){
  SYSCTL_RCGCI2C_R |= SYSCTL_RCGCI2C_R0;           // activate I2C0
  SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R1;           // activate port B
  while((SYSCTL_PRGPIO_R&0x0002) == 0){}; // ready?

    GPIO_PORTB_AFSEL_R |= 0x0C;           // 3) enable alt funct on PB2,3       0b00001100
    GPIO_PORTB_ODR_R |= 0x08;             // 4) enable open drain on PB3 only

    GPIO_PORTB_DEN_R |= 0x0C;             // 5) enable digital I/O on PB2,3


                                                                            // 6) configure PB2,3 as I2C
//  GPIO_PORTB_PCTL_R = (GPIO_PORTB_PCTL_R&0xFFFF00FF)+0x00003300;
  GPIO_PORTB_PCTL_R = (GPIO_PORTB_PCTL_R&0xFFFF00FF)+0x00002200;    //TED
    I2C0_MCR_R = I2C_MCR_MFE;                       // 9) master function enable
    I2C0_MTPR_R = 0b0000000000000101000000000111011;                       // 8) configure for 100 kbps clock (added 8 clocks of glitch suppression ~50ns)

       
}

void PortH_Init(void){
	
	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R7;				
	while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R7) == 0){};	
	GPIO_PORTH_DIR_R |= 0x0F;        								
  GPIO_PORTH_AFSEL_R &= ~0x0F;     								
  GPIO_PORTH_DEN_R |= 0x0F;        																									
  GPIO_PORTH_AMSEL_R &= ~0x0F;     								
	
}



void PortJ_Init(void) {
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R8;  // Activate clock for Port J
    while ((SYSCTL_PRGPIO_R & SYSCTL_PRGPIO_R8) == 0) {};  // Wait for clock to stabilize

    GPIO_PORTJ_DIR_R &= ~0x03;  // Set PJ0 and PJ1 as inputs
    GPIO_PORTJ_DEN_R |= 0x03;   // Enable digital functionality on PJ0 and PJ1
    GPIO_PORTJ_AFSEL_R &= ~0x03; // Disable alternate function on PJ0 and PJ1
    GPIO_PORTJ_AMSEL_R &= ~0x03; // Disable analog functionality on PJ0 and PJ1
    GPIO_PORTJ_PUR_R |= 0x03;   // Enable pull-up resistors on PJ0 and PJ1
}


//The VL53L1X needs to be reset using XSHUT.  We will use PG0
void PortG_Init(void){
    //Use PortG0
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R6;                // activate clock for Port N
    while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R6) == 0){};    // allow time for clock to stabilize
    GPIO_PORTG_DIR_R &= 0x00;                                        // make PG0 in (HiZ)
  GPIO_PORTG_AFSEL_R &= ~0x01;                                     // disable alt funct on PG0
  GPIO_PORTG_DEN_R |= 0x01;                                        // enable digital I/O on PG0
                                                                                                    // configure PG0 as GPIO
  //GPIO_PORTN_PCTL_R = (GPIO_PORTN_PCTL_R&0xFFFFFF00)+0x00000000;
  GPIO_PORTG_AMSEL_R &= ~0x01;                                     // disable analog functionality on PN0

    return;
}

//XSHUT     This pin is an active-low shutdown input;
// the board pulls it up to VDD to enable the sensor by default.
// Driving this pin low puts the sensor into hardware standby. This input is not level-shifted.
void VL53L1X_XSHUT(void){
    GPIO_PORTG_DIR_R |= 0x01;                                        // make PG0 out
    GPIO_PORTG_DATA_R &= 0b11111110;                                 //PG0 = 0
    SysTick_Wait10ms(10);
    GPIO_PORTG_DIR_R &= ~0x01;                                            // make PG0 input (HiZ)
   
}


void SpinMotor(int direction) {
    uint32_t delay = 200;     
    
    if (direction == 1) {
        GPIO_PORTH_DATA_R = 0b00000011;
        SysTick_Wait10ms(delay);
        GPIO_PORTH_DATA_R = 0b00000110;
        SysTick_Wait10ms(delay);
        GPIO_PORTH_DATA_R = 0b00001100;
        SysTick_Wait10ms(delay);
        GPIO_PORTH_DATA_R = 0b00001001;
        SysTick_Wait10ms(delay);
    } else if (direction == -1) {
        GPIO_PORTH_DATA_R = 0b00001001;
        SysTick_Wait10ms(delay);
        GPIO_PORTH_DATA_R = 0b00001100;
        SysTick_Wait10ms(delay);
        GPIO_PORTH_DATA_R = 0b00000110;
        SysTick_Wait10ms(delay);
        GPIO_PORTH_DATA_R = 0b00000011;
        SysTick_Wait10ms(delay);
    }


	}

//*********************************************************************************************************
//*********************************************************************************************************
//*********** MAIN Function *****************************************************************
//*********************************************************************************************************
//*********************************************************************************************************
uint16_t dev = 0x29; //address of the ToF sensor as an I2C slave peripheral
int status=0;

uint32_t position = 0;
uint32_t motorDirection = 1;
	
int main(void) {
  uint8_t byteData, sensorState=0, myByteArray[10] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF} , i=0;
  uint16_t wordData;
  uint16_t Distance;
  uint16_t SignalRate;
  uint16_t AmbientRate;
  uint16_t SpadNum;
  uint8_t RangeStatus;
  uint8_t dataReady;

		//initialize
		PLL_Init();
		SysTick_Init();
		onboardLEDs_Init();
		I2C_Init();
		UART_Init();
		PortH_Init();
		PortG_Init();
		PortJ_Init();

		// hello world!
		UART_printf("Program Begins\r\n");
		int mynumber = 1;
		sprintf(printf_buffer,"2DX ToF Program Studio Code %d\r\n",mynumber);
		UART_printf(printf_buffer);


		/* Those basic I2C read functions can be used to check your own I2C functions */
		status = VL53L1X_GetSensorId(dev, &wordData);

		sprintf(printf_buffer,"(Model_ID, Module_Type)=0x%x\r\n",wordData);
		UART_printf(printf_buffer);

		// 1 Wait for device ToF booted
		while(sensorState==0){
		status = VL53L1X_BootState(dev, &sensorState);
		SysTick_Wait10ms(10);
			}
		UART_printf("ToF Chip Booted!\r\n Please Wait...\r\n");

		status = VL53L1X_ClearInterrupt(dev); /* clear interrupt has to be called to enable next interrupt*/

			/* 2 Initialize the sensor with the default setting  */
		status = VL53L1X_SensorInit(dev);
		Status_Check("SensorInit", status);


  status = VL53L1X_StartRanging(dev);   // 4 This function has to be called to enable the ranging

	SysTick_Wait10ms(40000);
	uint32_t scan_count = 0;

while(1) {
    
    if ((GPIO_PORTJ_DATA_R & 0x01) == 0) {      
        while ((GPIO_PORTJ_DATA_R & 0x01) == 0) { // Button to Toggle Rotation
            SysTick_Wait10ms(20);
        }
        motorStatus = !motorStatus;
    }
    if ((GPIO_PORTJ_DATA_R & 0x02) == 0) {      
        while ((GPIO_PORTJ_DATA_R & 0x02) == 0) { //Button to Toggle Data Acquisition
            SysTick_Wait10ms(20);
        }
        dataAcquisitionStatus = !dataAcquisitionStatus;
    }
    
    if (motorStatus == 1) { //Spins motor if Motor Status is ON
        
        SpinMotor(motorDirection);
        position++;
        if ((position % 16) == 0) {
            if (dataAcquisitionStatus == 1 && motorDirection == 1) {
								FlashLED1(1);  //Flash LED to indicate measurement has been taken
                while (dataReady == 0) {
                    status = VL53L1X_CheckForDataReady(dev, &dataReady);
                    VL53L1_WaitMs(dev, 5);
                }
                dataReady = 0;
                status = VL53L1X_GetRangeStatus(dev, &RangeStatus);
                status = VL53L1X_GetDistance(dev, &Distance);
                status = VL53L1X_GetSignalRate(dev, &SignalRate);
                status = VL53L1X_GetAmbientRate(dev, &AmbientRate);  //Information recieved from sensor
                status = VL53L1X_GetSpadNb(dev, &SpadNum);
                status = VL53L1X_ClearInterrupt(dev);
								
								FlashLED3(1); //Flash LED to indicate tranmission is complete
                sprintf(printf_buffer, "%u\n", Distance);
                UART_printf(printf_buffer);
								
            }
						if (motorDirection == -1){
							SysTick_Wait10ms(100); //Rotate counter clockwise to inital position
							scan_count++;
						}
						else{
							SysTick_Wait10ms(2000); //Buffer time to allow stationary scan
							scan_count++;
						}
            
        }
				if (scan_count >= 31) {
                motorDirection = -motorDirection;  //One rotation complete, sets direcction to CCW so it returns to inital position
                scan_count = 0;                     

                SysTick_Wait10ms(20000); 
            }
    }
    else if (motorStatus == 0 && dataAcquisitionStatus == 1) { //Only scans and motor doesnt rotate
        
        while (dataReady == 0) {
            status = VL53L1X_CheckForDataReady(dev, &dataReady);
            VL53L1_WaitMs(dev, 5);
        }
				
        dataReady = 0;
        status = VL53L1X_GetRangeStatus(dev, &RangeStatus);
        status = VL53L1X_GetDistance(dev, &Distance);
        status = VL53L1X_GetSignalRate(dev, &SignalRate);
        status = VL53L1X_GetAmbientRate(dev, &AmbientRate);
        status = VL53L1X_GetSpadNb(dev, &SpadNum);
        status = VL53L1X_ClearInterrupt(dev);
				FlashLED2(1); //Flash LED to tell that we are scanning stationary 
        sprintf(printf_buffer, "%u\n", Distance);
        UART_printf(printf_buffer);
        SysTick_Wait10ms(2000);
    }
		
    else {
        
				FlashAllLEDs(); //Indicates that no funtionaility is happening, no data is being taken and motor is stationary
				SysTick_Wait10ms(20);
        
    }
}

 
VL53L1X_StopRanging(dev);
  while(1) {}
}	



