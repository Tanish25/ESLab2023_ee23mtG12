#include <stdint.h>
#include "tm4c123gh6pm.h"

//global variables
volatile uint8_t inc_count = 0;
volatile uint8_t dec_count = 0;

int compare_value = 80;//50 percent duty cycle

#define MASK_BITS 0x11 // pins for which interrupts should be enabled

//systick registers
#define STCTRL *((volatile long *) 0xE000E010)    // control and status register
#define STRELOAD *((volatile long *) 0xE000E014)    // reload value register
#define STCURRENT *((volatile long *) 0xE000E018)    // current value register

//systick macros
#define ENABLE      (1 << 0)    // bit 0 of CSR enables timer
#define TICKINT      (1 << 1)    // bit 1 of CSR for interrupt generation
#define CLKINT      (1 << 2)    // bit 2 of CSR to specify clock frequency
#define COUNT_FLAG  (1 << 16)   // bit 16 of CSR automatically set to 1 when timer expires


void PWM_Control(volatile uint16_t compare)
{
    SYSCTL_RCGCPWM_R |= (1<<1);    /* Enable clock to PWM1 module */
    SYSCTL_RCC_R &= (~(1<<20));    /* Directly feed clock to PWMI module without pre-divider */

    /* PWMl channel 6 setting */
    PWM1_3_CTL_R &= (~(1<<0));    /* Disable Generator 3 counter */
    PWM1_3_CTL_R &= (~(1<<1));  /* select down count mode of counter 3*/
    //PWM1_3_CTL_R |= (1<<1);  /* select up count mode of counter 3*/
    PWM1_3_LOAD_R = 160;   /* set load value for 1kHz: 16Mhz divided by 160 gives 1kHz */
    PWM1_3_CMPA_R = compare;  /* set duty cycle value according to functional parameter */
    PWM1_3_GENA_R |= (1<<2) | (1<<3) | (1<<7);   /* Set PWM output when counter reloaded and clear when matches PWMCMPA */
    PWM1_3_CTL_R |= (1<<0);   /* Enable Generator 3 counter */
    PWM1_ENABLE_R |= (1<<6);   /* Enable PWMI channel 6 output */
}
void GPIO_PortF_Handler()
{
    GPIO_PORTF_ICR_R = 0x11; // Clear the interrupt flag
    STCTRL = 0x07; // Enable SysTick Timer
}

void SysTickIntHandler()
{
    if(GPIO_PORTF_DATA_R & 0x10)
    {
        inc_count += 1; //increase PWM duty cycle by 5 percent
    }

    else
    {
        dec_count += 1; //decrease PWM duty cycle by 5 percent
    }
    STCTRL = 0x00; // Disable SysTick Timer
}

int main()
{
//gpio Pins initialisation
//we choose PORTF because its oins are connected to LED as well, so we can have an average estimate of duty cycle via LED Brightness
    SYSCTL_RCGCGPIO_R |= (1<<5);    /* Enable system clock to PORTF */
    GPIO_PORTF_LOCK_R = 0x4C4F434B;     /* unlock commit register */
    GPIO_PORTF_CR_R = 0xFF;             /* make PORTF0 configurable */
    GPIO_PORTF_DIR_R &= ~0x11; // Set Pin 0 and 4 as input
    GPIO_PORTF_PUR_R |= 0x11;  // Enable the internal pull-up resistor for Pin 0 and 4
    GPIO_PORTF_DEN_R |= 0x11;  // Digital enable Pin 0 and 4
   GPIO_PORTF_DEN_R |= (1<<2);    /* set PF2 as a digital pin */

    GPIO_PORTF_AFSEL_R |= (1<<2); /* PF2 sets a alternate function */
    GPIO_PORTF_PCTL_R |= 0x00000500;    /* make PF2 PWM output pin */
 

    //GPIO interrupts
    GPIO_PORTF_IM_R &= ~MASK_BITS; // mask interrupt by clearing bits
    GPIO_PORTF_IS_R &= ~MASK_BITS; // edge sensitive interrupts
    GPIO_PORTF_IBE_R &= ~MASK_BITS; // interrupt NOT on both edges
    GPIO_PORTF_IEV_R &= ~MASK_BITS; // falling edge trigger


    //Set protity using NVIC
    NVIC_PRI7_R = (NVIC_PRI7_R & 0xF1FFFFFF) | (3 << 21); // bits 21-23 for interrupt 30 (port F)
    NVIC_EN0_R |= 1 << 30; // enable interrupt 30 (PORTF)

    GPIO_PORTF_ICR_R = MASK_BITS; // clear any prior interrupt
    GPIO_PORTF_IM_R |= MASK_BITS; // unmask interrupt by setting bits

//Systick initialisation
    STCURRENT = 0; //to make sure the new reload value gets reflected when the 
    STRELOAD = 16000*1000;                       // reload value for 1 second/1000 ms
    STCTRL |= (CLKINT | TICKINT | ENABLE);        // set internal clock, enable the timer

    PWM_Control(compare_value);//We start PWM generation with 50 percent duty cycle first

    while(1)
    {
        if (dec_count > 0)
        {
            if(compare_value > 0)
            {
                compare_value = compare_value - (160*0.05);//reducing duty cycle by 5 percent
                PWM_Control(compare_value);//generate new PWM with changed duty cycle
                dec_count -= dec_count;//lessen number of decremnets
            }

            else
            {
                dec_count -= dec_count;
            }
        }
        else if (inc_count > 0)
        {
            if(compare_value < 160)
            {   compare_value = compare_value + (160*0.05);//increasing duty cycle by 5 percent
                PWM_Control(compare_value);//generate new PWM with changed duty cycle
                inc_count -= inc_count;//lessen number of increments
            }

            else
            {
                inc_count -= inc_count;
            }
        }

    }
}
