
#include <stdint.h>
#include <stdbool.h>
#include "tm4c123gh6pm.h"
/*
int i,j;
void delay()
{
    for(i=0;i<100;i++)
    {
        for(j=0;j<100;j++);
    }
}
*/
int main(void)
{
        //Deciding to Toggle on port PB0
        SYSCTL_RCGC2_R |= 0x00000002;;      /* enable clock to GPIOB */
        GPIO_PORTB_LOCK_R = 0x4C4F434B;     /* unlock commit register */
        GPIO_PORTB_CR_R = 0x01;             /* make PORTB0 configurable */
        GPIO_PORTB_DIR_R = 0x01;            /* set PORTB0 pin as output (LED) pin */
        GPIO_PORTB_DEN_R = 0x01;            /* set PORTB pins 0 as digital pins */
        while(1)
        {
            GPIO_PORTB_DATA_R = 0x00;
            //delay();
            GPIO_PORTB_DATA_R = 0x01;
            //delay();
        }
}
