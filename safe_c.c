//      Donald Wight 
//      EECE 344 S14
//****************************************************************************************************     
//This program is used to interface with a cpld that is linked to a "Safe". It checks for the correct
//input of the switches, if the "safe" gets to hot, cotnrols the locking mechanism on the safe and 
// controls an lcd for user interface
//****************************************************************************************************
//includes for the project
#include "stm32F30x.h"
#include "STM32f3_discovery.h"
#include "stm32f30x_gpio.h"
#include "stdlib.h"

RCC_ClocksTypeDef RCC_Clocks;
__IO uint32_t ticks = 0;

GPIO_InitTypeDef        GPIO_InitStructure;

void    GPIO_Inits_Input(void);

void GPIO_Inits_Output(void);

static __IO uint32_t ticks;
//Handler for the systick
void systick_handler(void);

//Function for initialized GPIO ports that never change
void Init_default_constants(void);

//Function for the arm/cpld communication for switches
void arm_cpld(int);

// function to turn lcd on
void lcd_On(void);

// Function to write to the lcd
void lcd_write(char writer[], int, int);

//Function to reset the lcd
void lcd_reset(void);

//remove either delay or delay2 once busyflag is working
//which one am i using  (delay2) i beliee
void delay (int);


void pwm_pulse(int);

void temp_sensor(void);

void adc_inits(void);

int systick_1=0;
// int for the address
int address;

int read_write;
int tens = 0;

//int to keep track of button pushe
int push_button=0;
//********
//remove once busyflag is working
int systick = 0;

int repeat = 0;

int switchs_xor = 0x0000;
int switchs_one = 0x0000;
int switchs_two = 0x0000;
int wrong = 1;
int temp_init =0;
int temp = 0;
int button=0;
__IO uint16_t  ADC1ConvertedValue = 0, ADC1ConvertedVoltage = 0;
__IO uint32_t calibration_value = 0;
ADC_InitTypeDef       ADC_InitStructure;
ADC_CommonInitTypeDef ADC_CommonInitStructure;
GPIO_InitTypeDef    GPIO_InitStructure;
int main(void)
{
      SystemCoreClockUpdate();
      RCC_ClocksTypeDef RCC_Clocks;
      /* SysTick end of count event each 1/8s */
      RCC_GetClocksFreq(&RCC_Clocks);
      SysTick_Config(RCC_Clocks.HCLK_Frequency / 72000);
      pwm_pulse(10);
      
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~  
        // superloop
      char array[17]={NULL};
      Init_default_constants();
      lcd_On();
      delay(25);
      adc_inits();
     // infinite loop
      while (1)
      {
       temp_sensor();
       if (button == 0)
       {
         lcd_write("asl", 1, 1);
         button=1;
       }
      }
     while (1)
      {        
        // for the first time to set all the values
        if (push_button == 0)
        {
          Init_default_constants();
          pwm_pulse(10);
          wrong =1;
        }
        // function to take in the value from switches and output to the cpld led
       // if the button has not been pushed
        if (push_button==1)
        {
          // Only runs this once
          if( repeat <=1)
          {
            // resets the lcd
            lcd_reset();
            // puts the sentence into the array
            sprintf(array,"Enter Combination:");
            // write the array to the lcd screen
            lcd_write(array, 18, 0);
            // makes sure this wont loop again
            repeat=2;
          }
          // functions to take an input from the switches
          arm_cpld(1);
        }
        
       //if the button has been pushed once
       if (push_button==2)
        {
          // only will rrun once
          if (repeat <=1)
          {
            // resets the lcd
            lcd_On();
            // sprintf the sentence to the array
            sprintf(array,"Password Attempt:");
            // writes whats in the array to the screen of the lcd
            lcd_write(array, 17, 1); 
            // makes sure it wont enter this loop again
            repeat=2;
          }
          // reads in a valye for the password from the switches
          arm_cpld(2); 
        }
        // if the button has been pushed twice
        if (push_button==3)
        {
          // function to xor the combination with the password
          arm_cpld(3);
          // if the switches are equal
          if ((switchs_xor == 0) && repeat <=1 && push_button==3)
          {
            // reset lcd
            lcd_reset();
            // puts correct into the array
            sprintf(array, "Correct:");
            // writes to the lcd
            lcd_write(array, 8, 0);
            pwm_pulse(5);
            repeat=2;
          }
          //if the switches are not equal
          if ((switchs_xor !=0) && repeat <=1 && push_button==3)
          {
            // reset the lcd
            lcd_reset();
            // write to the lcd screen
            sprintf(array, "number:");
            // write to the lcd scree
            lcd_write(array, 6, 1);
            // if you enter incorrect password 3 times you get locked out
            if (wrong == 3)
            {
              lcd_reset();
              // changes the pulse length to 20 to move the servo
              pwm_pulse(20);
              sprintf(array, "Reset");
              // write to the lcd scree
              lcd_write(array, 5, 0);
              while(1);
            }
            wrong++;
            repeat=2;
          }
          // makes sure you dont go through the lcd write again
        }
      }
}
void adc_inits(void)
{
    /* ADC Channel configuration */
	/* GPIOC Periph clock enable */
	RCC_ADCCLKConfig(RCC_ADC12PLLCLK_Div2);

	/* Enable ADC1 clock */
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_ADC12, ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);

	/* Configure ADC Channel7 as analog input */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 ;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL ;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	ADC_StructInit(&ADC_InitStructure);

	/* Calibration procedure */  
	ADC_VoltageRegulatorCmd(ADC1, ENABLE);

	/* Insert delay equal to 10 µs */
	delay(10);

	ADC_SelectCalibrationMode(ADC1, ADC_CalibrationMode_Single);
	ADC_StartCalibration(ADC1);

	while(ADC_GetCalibrationStatus(ADC1) != RESET );
	calibration_value = ADC_GetCalibrationValue(ADC1);
	 
	ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent;                                                                    
	ADC_CommonInitStructure.ADC_Clock = ADC_Clock_AsynClkMode;                    
	ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled;             
	ADC_CommonInitStructure.ADC_DMAMode = ADC_DMAMode_OneShot;                  
	ADC_CommonInitStructure.ADC_TwoSamplingDelay = 0;          
	ADC_CommonInit(ADC1, &ADC_CommonInitStructure);

	ADC_InitStructure.ADC_ContinuousConvMode = ADC_ContinuousConvMode_Enable;
	ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b; 
	ADC_InitStructure.ADC_ExternalTrigConvEvent = ADC_ExternalTrigConvEvent_0;         
	ADC_InitStructure.ADC_ExternalTrigEventEdge = ADC_ExternalTrigEventEdge_None;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_OverrunMode = ADC_OverrunMode_Disable;   
	ADC_InitStructure.ADC_AutoInjMode = ADC_AutoInjec_Disable;  
	ADC_InitStructure.ADC_NbrOfRegChannel = 1;
	ADC_Init(ADC1, &ADC_InitStructure);

	/* ADC1 regular channel7 configuration */ 
	ADC_RegularChannelConfig(ADC1, ADC_Channel_2, 1, ADC_SampleTime_7Cycles5);

	/* Enable ADC1 */
	ADC_Cmd(ADC1, ENABLE);

	/* wait for ADRDY */
	while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_RDY));

	/* Start ADC1 Software Conversion */ 
	ADC_StartConversion(ADC1);   

	/* Infinite loop */
	/* Test EOC flag */
	while(ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);

	/* Get ADC1 converted data */
	ADC1ConvertedValue =ADC_GetConversionValue(ADC1);

	/* Compute the voltage */
	ADC1ConvertedVoltage = (ADC1ConvertedValue *3300)/0xFFF;
	temp_init = ADC1ConvertedVoltage;
	temp_init=temp_init/10;
	temp_init=temp_init-273;    
}
void temp_sensor(void)
{
   /* Start ADC1 Software Conversion */ 
  ADC_StartConversion(ADC1);   
  
  /* Infinite loop */
    /* Test EOC flag */
    while(ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);
    
    /* Get ADC1 converted data */
    ADC1ConvertedValue =ADC_GetConversionValue(ADC1);
    
    /* Compute the voltage */
    // divide converted value by 0xfff(#of bits in reg) and mult by 3300(input voltage)
    ADC1ConvertedVoltage = (ADC1ConvertedValue *3300)/0xFFF;
    temp = ADC1ConvertedVoltage;
    // divide by ten to get kelvin(10 mv = 1 kelvin)
    temp=temp/10;
    // subract to get celcius
    temp=temp-273;
    tens= ((ADC1ConvertedVoltage%1000)%100)/10;
    // if out of bounds then we infinite loop
    // did +- 3 to make it not as finicky
    if (temp < temp_init-3 ||temp>temp_init+3)
    {
      while(1);
    }
    
}
void lcd_On(void)
{ 
  // sends out commands to turn on the lcd
  // commands given in data sheet
  GPIO_Write(GPIOB, 0x0000);
    
  GPIO_Inits_Output();
  
  GPIO_Write(GPIOC, 0xFF00);
  GPIO_Write(GPIOD, 0x0000);  
  
  GPIO_Write(GPIOC, 0x0000);
  GPIO_Write(GPIOB, 0x8000); 
  GPIO_Write(GPIOD, 0x3F00);
  delay(100);
  GPIO_Write(GPIOB, 0x0000);
  GPIO_Write(GPIOC, 0x0000);
  GPIO_Write(GPIOB, 0x8000);
  GPIO_Write(GPIOD, 0x0F00);
  delay(100);
  GPIO_Write(GPIOB, 0x0000);
  GPIO_Write(GPIOC, 0x0000);
  GPIO_Write(GPIOB, 0x8000);
  GPIO_Write(GPIOD, 0x0100);
  
  delay(100);
  GPIO_Write(GPIOB, 0x0000);
  GPIO_Write(GPIOC, 0x0000);
  GPIO_Write(GPIOB, 0x8000);
  GPIO_Write(GPIOD, 0x0600);
  delay(100);
  GPIO_Inits_Input();
}  
void lcd_write(char writer[], int length, int number)
{
  int var_x = 0x0000;
  char c[12];
  lcd_reset();
  sprintf(c,"voltage= %3d",ADC1ConvertedValue);
  for(int i = 0; i<14;i++)
  {
    var_x=(int)c[i]*16*16;
    GPIO_Write(GPIOB, 0x0000);
    GPIO_Inits_Output();
    GPIO_Write(GPIOD, 0x0000);
    GPIO_Write(GPIOC, 0xFFF0);
    GPIO_Write(GPIOB, 0x1000);
    GPIO_Write(GPIOB, 0x9000);
    GPIO_Write(GPIOC, 0x0000);
    GPIO_Write(GPIOD, var_x);
    delay(20);
  }
    GPIO_Write(GPIOB,0x0000);
  GPIO_Inits_Output();
  GPIO_Write(GPIOD,0xC000);
  GPIO_Write(GPIOC, 0x0000);
  GPIO_Write(GPIOB,0x8000);
  GPIO_Write(GPIOB,0x0000);
  delay(20);
  sprintf(c,"celcius=%1u",temp);
  for(int i = 0; i<10;i++)
  {
    var_x=(int)c[i]*16*16;
    GPIO_Write(GPIOB, 0x0000);
    GPIO_Inits_Output();
    GPIO_Write(GPIOD, 0x0000);
    GPIO_Write(GPIOC, 0xFFF0);
    GPIO_Write(GPIOB, 0x1000);
    GPIO_Write(GPIOB, 0x9000);
    GPIO_Write(GPIOC, 0x0000);
    GPIO_Write(GPIOD, var_x);
    delay(20);

  }
    sprintf (c,".");
      var_x=(int) c[0]*16*16;
    GPIO_Write(GPIOB, 0x0000);
    GPIO_Inits_Output();
    GPIO_Write(GPIOD, 0x0000);
    GPIO_Write(GPIOC, 0xFFF0);
    GPIO_Write(GPIOB, 0x1000);
    GPIO_Write(GPIOB, 0x9000);
    GPIO_Write(GPIOC, 0x0000);
    GPIO_Write(GPIOD, var_x);
    delay(20);      
	sprintf (c,"%1u", tens);
      var_x=(int) c[0]*16*16;
    GPIO_Write(GPIOB, 0x0000);
    GPIO_Inits_Output();
    GPIO_Write(GPIOD, 0x0000);
    GPIO_Write(GPIOC, 0xFFF0);
    GPIO_Write(GPIOB, 0x1000);
    GPIO_Write(GPIOB, 0x9000);
    GPIO_Write(GPIOC, 0x0000);
    GPIO_Write(GPIOD, var_x);
    delay(20);
	GPIO_Inits_Input();
	GPIO_Write(GPIOB, address);
	GPIO_Write(GPIOC, read_write);
  
}
void lcd_reset(void)
{
  // sends out data to reset and return the lcd cursor to home
  GPIO_Write(GPIOB, 0x0000);
  GPIO_Inits_Output();
  GPIO_Write(GPIOC, 0x0000);
  GPIO_Write(GPIOB, 0x8000);
  GPIO_Write(GPIOD, 0x0200);
  delay(100);
  GPIO_Write(GPIOB, 0x0000);
  GPIO_Write(GPIOC, 0x00F0);
  GPIO_Write(GPIOC, 0x0000);
  GPIO_Write(GPIOB, 0x8000);
  GPIO_Write(GPIOD, 0x0100);
  GPIO_Write(GPIOB, 0x00000);
  delay(100);
}
void arm_cpld(int x)
{
    // sets cpld_led to 0
    // used for debugging purposes
    //GPIO_Write(GPIOE, 0x0000);
    GPIO_Write(GPIOB, address);
    // makes sure the wr bit is set
    GPIO_Write(GPIOC, read_write);
    // if the button has not been pushed we will read in and save as
    // sets switchse to the bits high on gpiod
  if (x ==1 || x==2)
  {
    if (x ==1)
    {
     switchs_one=(GPIO_ReadInputData (GPIOD));
        //debugging purposes only
    }
    if (x==2)
    {
      switchs_two=(GPIO_ReadInputData (GPIOD));
        //debugging purposes only
    }
    //*************
    //will this work
    GPIO_Write(GPIOE,GPIO_ReadInputData (GPIOD));
  }
  if (x==3)
  {
    switchs_xor = switchs_one ^ switchs_two;
    // changes the gpio ports to output
    GPIO_Inits_Output();
    // xor the two vlues from the switdes
    // send out the xor valyue
    GPIO_Write(GPIOD, switchs_xor);
    //debugging purposes only
    GPIO_Write(GPIOE, switchs_xor);
  }  
}
// function to initialize ports at the beginning and ports that do not change
void Init_default_constants(void)
{
      //initializes the led's for debugging purposes
      STM_EVAL_LEDInit(LED3);
      STM_EVAL_LEDInit(LED4);
      STM_EVAL_LEDInit(LED5);
      STM_EVAL_LEDInit(LED6);
      STM_EVAL_LEDInit(LED7);      
      STM_EVAL_LEDInit(LED8);
      STM_EVAL_LEDInit(LED9);
      STM_EVAL_LEDInit(LED10);
      
      // initializes button press     
      STM_EVAL_PBInit(BUTTON_USER, BUTTON_MODE_EXTI);
     // initializes gpioc
      RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOC, ENABLE);  
      GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
      GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
      GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
      GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
      GPIO_Init(GPIOC, &GPIO_InitStructure);
      //initializes gpiob
       RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB, ENABLE);  
      GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15 | GPIO_Pin_14 | GPIO_Pin_13 | GPIO_Pin_12;
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
      GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
      GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
      GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
      GPIO_Init(GPIOB, &GPIO_InitStructure);
      GPIO_Write(GPIOD, 0x0000);
      GPIO_Inits_Input();
      // sets the address line to connect to the switches
      address = 0xD000;
      read_write = 0x00F0;
      // sets the read write bit to read from the cpld
      GPIO_Write(GPIOC, read_write);
      // initializes switchs_one and two
      // writes the address to gpiob
      GPIO_Write(GPIOB, address);
      // sets the gpio data ports to inputs
      //GPIO_Inits_Input();
      // exits this loop by adding one to push_button which will later be used as a reference to 
      // how many inputs the switches have given
      push_button++;
      
}
// function to initialize GPIO's as an output
void GPIO_Inits_Output(void)
{
  // GPIOD initialization for GPIOD as an output
      RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOD, ENABLE);  
      GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15 | GPIO_Pin_14 | GPIO_Pin_13 | GPIO_Pin_12
        | GPIO_Pin_11| GPIO_Pin_10| GPIO_Pin_9| GPIO_Pin_8;
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
      GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
      GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
      GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
      GPIO_Init(GPIOD, &GPIO_InitStructure);   
  
}
void GPIO_Inits_Input(void)
{
  // GPIOD initialization to be an input
      RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOD, ENABLE);  
      GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15 | GPIO_Pin_14 | GPIO_Pin_13 | GPIO_Pin_12
        | GPIO_Pin_11| GPIO_Pin_10| GPIO_Pin_9| GPIO_Pin_8;
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
      GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
      GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
      GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
      GPIO_Init(GPIOD, &GPIO_InitStructure);
      
}
// function that deals with a button press interrupt
void delay(int count)
{
  //***********8
  // see if having the waitinbetween the GPIOC and busy read works
  int busy_flag = 0x8000;
  // makes sure the lcd is not reading any data and no data is being put out
  GPIO_Write(GPIOB, 0x0000);
  GPIO_Write(GPIOC, 0x0000);
  GPIO_Write(GPIOD, 0x0000);
  GPIO_Write(GPIOC, 0x0FF0);
  //while busy flag returns a 1
  while (busy_flag == 0x8000)
  {
    GPIO_Write(GPIOB, 0x8000);
    GPIO_Write(GPIOB, 0x0000);
    busy_flag = GPIO_ReadInputData(GPIOD);
  }
  //quick wait after the code exits thebusy flag loop
  systick=0;
  while (systick != (count+10));
  GPIO_Write(GPIOB, 0x1000);
  GPIO_Write(GPIOC, 0x0FF0);
  GPIO_Inits_Output();
  GPIO_Write(GPIOD, 0x0000);     
}
void pwm_pulse(int pulse)
{
  // structure for the gpio port with the pulse
        GPIO_InitTypeDef        GPIO_InitStructure;								
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;						
	TIM_OCInitTypeDef				TIM_OCInitStructure;							
	// enables periph clock for gpioa
        RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);				
	// inits the pin 1 on gpioa
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
        // sets it to an alternate function
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;									
        GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;								
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
        //If this stops it from working then switch to PDOWN
        GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;						
        GPIO_Init(GPIOA, &GPIO_InitStructure);											
              // initializes the clock for the pulse
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
        // sets the gpioa pin 1 to the desired alternative function for pwm signal
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource1, GPIO_AF_1);			
	
        // sets up the clock  by dividing by the prescaler
	TIM_TimeBaseStructure.TIM_Prescaler = 7200;
        // period to have it be at 20 ms. actual amount is 20.02ms
	TIM_TimeBaseStructure.TIM_Period = 178;									
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
        TIM_TimeBaseStructure.TIM_ClockDivision = 0;
        TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;

        TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
	
        // initializes the clock to be a pwm signal
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM2;					
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = pulse;
        TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;
        TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Set;
        
        TIM_OC2Init(TIM2, &TIM_OCInitStructure);
															
	TIM_Cmd(TIM2, ENABLE);	
        TIM_CtrlPWMOutputs(TIM2, ENABLE);

  

	
}
void EXTI0_IRQHandler(void)
{
  // sets the button push variable plus one so the code will continue to run
  push_button++;
  // sets repeat = 0 for the next lcd write
  repeat = 0;
  button=0;
  // if the button is pushed for the second time it will run the code to switch the address and read write bit
  if (push_button ==3)
  {
    read_write=0x0000;
    address = 0xC000;
  }
  // if you push the button again it will reset the code
  if (push_button >3)
  {
    // if switches are the same
    if(switchs_xor ==0)
    {
      push_button = 0;
    }
    // iv switches are not the same
    if (switchs_xor !=0)
    {
       // moves back to changing your password 
      GPIO_Write(GPIOD, 0x0000);
      GPIO_Inits_Input();
      Init_default_constants();
      push_button=2;
    }
  }
  // clears the pending bit for the interrupt
  EXTI_ClearITPendingBit(USER_BUTTON_EXTI_LINE);
}
void SysTick_Handler(void)
{
  systick++;
  systick_1++;
  GPIOF->ODR=0x0000;
  if (systick_1 ==1)
  {
    GPIOF->ODR=0xFFFF;
  }
  if (systick_1 ==20)
  {
    systick_1 =0;
  }   
}
#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* User can add his own implementation to report the file name and line number,
     ex: printf("number parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif
