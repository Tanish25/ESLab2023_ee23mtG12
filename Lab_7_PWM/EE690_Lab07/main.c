#include <stdint.h>
#include "tm4c123gh6pm.h"

//macros:
#define MAX_DUTY_CYCLE 95     // Maximum duty cycle percentage
#define MIN_DUTY_CYCLE 5      // Minimum duty cycle percentage
#define DUTY_CYCLE_STEP 5    // Duty cycle change step in percentage

volatile uint8_t inc_count = 0;//stores number of increments
volatile uint8_t dec_count = 0;//stores number of decrements
int compare_value = 80;//value is 80 which corresponds to 50 percent duty cycle for 1kHz

 
#define MASK_BITS 0x11 // pins for which interrupts should be enabled


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

    if(GPIO_PORTF_DATA_R & 0x01)
    {
        inc_count += 1;
    }

    else if(GPIO_PORTF_DATA_R & 0x10)
    {
        dec_count += 1;
    }
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
                inc_count -= inc_count;//lessen number of decremnets
            }

            else
            {
                inc_count -= inc_count;
            }
        }

    }
}