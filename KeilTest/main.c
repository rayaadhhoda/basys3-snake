//------------------------------------------------------------------------------------------------------
// Program SoC using C
// 1)Input from 8-bit switch and output to LEDs
// 2)Input characters from keyboard (UART) and output to the terminal
// 3)A counter is incremented from 1 to 10, and displayed on the VGA monitor
//------------------------------------------------------------------------------------------------------

#define AHB_VGA_BASE				0x50000000
#define AHB_UART_BASE				0x51000000
#define AHB_TIMER_BASE			0x52000000
#define AHB_GPIO_BASE				0x53000000
#define AHB_7SEG_BASE				0x54000000
#define NVIC_INT_ENABLE			0xE000E100
#define NVIC_INT_PRIORITY0	0xE000E400

volatile static int counter=0x31;

void UART_ISR()
{
	__asm volatile (
			"MOVS r0, #0x51     \n"
			"LSLS r0, r0, #24   \n"   /* r0 = 0x51000000 (UART_BASE) */
			"LDR  r1, [r0, #0]  \n"   /* read char from UART */
			"STR  r1, [r0, #0]  \n"   /* write char back to UART */
			::: "r0", "r1"
	);
	
	//char c = *(volatile unsigned int*) AHB_UART_BASE;        // read RX
  //*(volatile unsigned int*) AHB_UART_BASE = c;             // write back TX
}

void Timer_ISR()
{
	*(unsigned int*) AHB_VGA_BASE = counter;                     // print digit (ASCII '1'�'9')
	*(unsigned int*) AHB_7SEG_BASE = counter - 0x30;            // 7-seg uses 0�9, not ASCII

	counter++;
	if (counter==0x3A)
		*(unsigned int*) (AHB_TIMER_BASE+8) = 0;	//Stop timer if counter reaches 9
	*(unsigned int*) (AHB_TIMER_BASE+0x0C) = 1;	//Clear timer interrupt request
}

//////////////////////////////////////////////////////////////////
// Main Function
//////////////////////////////////////////////////////////////////

int main(void) {
	
	*(unsigned int*) AHB_TIMER_BASE = 50000000;			  //Timer load register: =<clock frequency>
	*(unsigned int*) (AHB_TIMER_BASE+8) = 0x03;				//Timer 4-bits control register: [0]: timer enable, [1] mode (free-run or reload) [2]: prescaler

	*(unsigned int*) NVIC_INT_PRIORITY0 = 0x00004000;	//Priority: IRQ0(Timer): 0x00, IRQ1(UART): 0x40
  *(unsigned int*) NVIC_INT_ENABLE = 0x00000003;		//Enable interrupts for UART and timer
  *(unsigned int*) AHB_7SEG_BASE = 0;						 	 	//reset 7-segment display
	*(unsigned int*) (AHB_7SEG_BASE +0x04) = 0;		
	*(unsigned int*) (AHB_7SEG_BASE +0x08)= 0;		
	*(unsigned int*) (AHB_7SEG_BASE +0x0C)= 0;		
	while(1){
		*(unsigned int*) (AHB_GPIO_BASE + 0x04) = 0x0000;          // set read mode
		int sw = *(unsigned int*) AHB_GPIO_BASE;                    // read switches
		*(unsigned int*) (AHB_GPIO_BASE + 0x04) = 0x0001;  // was 0x00FF; set write mode
		*(unsigned int*) AHB_GPIO_BASE = sw;                        // write to LEDs
	}
}


