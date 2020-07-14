/*                                  LED Control                                          */
/*                          TARGET DEVICE: PIC18F27K40                                   */
/*****************************************************************************************/

//Included PIC18F27K40 header file to gain access to PIC specific functions & keywords
#include <18F27K40.h> 

//Setting fuses using preprocessor directive (#fuses)
//RSTOSC_HFINTRC_1MHZ - enable 1MHz internal oscillator
//CLKOUT - Clock output enable
//NOMCLR - Master clear disabled
//NOPUT - Power-up timer disabled
//NOWDT - Windowed watchdog timer disabled
//NOLVP - Low-voltage programming disabled
//NOPROTECT - User NVM code protection disabled
//NOCPD - Data NVM code protection disabled
#fuses RSTOSC_HFINTRC_1MHZ CLKOUT NOMCLR NOPUT NOWDT NOLVP NOPROTECT NOCPD

//Change clock speed to 1MHz & inform compiler of clock speed
#use delay(internal=1MHZ,clock_out)

//Structure definitions - mapping of IO pins to structure, two mapping used
//Mapping No.1 - for specifications 1,2,3,4,5,6,7,8,9,12,14,15
struct IO_def1
{
   int ExperimentSelection:4; //RA0..3 used as experiment selection (4 bits)
   int unused_A1:2;           //RA4..5 unused (2 bits)
   int1 Clock_Output;         //RA6 clock output (1 bit)
   int1 unused_A2;            //RA7 unused (1 bit)
   int B_inputs:4;            //RB0..3 grouped to be used as inputs later on (4 bits)
   int unused_B:4;            //RB4..7 unused (4 bits)
   int Multi_LED:6;           //RC0..5 used to display output
   int unused_C:2;            //RC6..7 unused (2 bits)
};
//Mapping No.2 - for specifications 10,11,13
struct IO_def2
{
   int ExperimentSelection:4; //RA0..3 used as experiment selection (4 bits)
   int unused_A1:2;           //RA4..5 unused (2 bits)
   int1 Clock_Output;         //RA6 clock output (1 bit)
   int1 unused_A2;            //RA7 unused (1 bit)
   int B_inputs_1:2;          //RB0..1 grouped to be used as inputs later on (2 bits)
   int B_inputs_2:2;          //RB2..3 grouped to be used as inputs later on (2 bits)
   int unused_B:4;            //RB4..7 unused (4 bits)
   int Multi_LED:6;           //RC0..5 used to display output
   int unused_C:2;            //RC6..7 unused
};

//Structure delcarations
struct IO_def1 IO_Port_1;       //Declare structure 1 for mapping to PORTA,PORTB,PORTC registers
struct IO_def2 IO_Port_2;       //Declare structure 2 for mapping to PORTA,PORTB,PORTC registers
struct IO_def1 IO_Port_1_Latch; //Declare structure for mapping to LATA,LATB,LATC registers
struct IO_def1 IO_Port_1_Direction; //Declare structure for mapping to TRISA,TRSIB,TRISC registers

//Load structure to memory addresses corresponding to PORTA,PORTB,PORTC registers
#byte IO_Port_1 = 0xF8D
#byte IO_Port_2 = 0xF8D

//Load structure to memory addresses corresponding to LATA,LATB,LATC registers
#byte IO_Port_1_Latch = 0xF83

//Load structure to memory addresses corresponding to TRISA,TRISB,TRISC registers
#byte IO_Port_1_Direction = 0xF88

//Grey code lookup table for specifications 8,9
int LUT_grey[]={
   0b0000,0b0001,0b0011,0b0010,0b0110,0b0111,0b0101,0b0100,
   0b1100,0b1101,0b1111,0b1110,0b1010,0b1011,0b1001,0b1000};
   
//Delay lookup table for running light in specfication 15               
unsigned long long int LUT_delay[16]={
   50,75,150,200,300,400,600,800,1000,
   1200,1600,2400,3200,4000,4300,4600};
   
int counter;     //Counter variable for looping through LUT in specification 8 and 9
int clockstate;  //Stores clock state for hopped microprocessor in specification 4
int PreviousExperimentSelection; //Stores previous experiment selection

void main()
{
   IO_Port_1_Direction.ExperimentSelection=0b1111; //RA0..3 as inputs
   IO_Port_1_Direction.unused_A1=0b00;             //RA4..5 as outputs
   IO_Port_1_Direction.Clock_Output=0b0;           //RA6 as output
   IO_Port_1_Direction.unused_A2=0b0;              //RA7 as output
   IO_Port_1_Direction.B_inputs=0b1111;            //RB0..3 as inputs
   IO_Port_1_Direction.unused_B=0b0000;            //RB4..7 as outputs
   IO_Port_1_Direction.Multi_LED=0b000000;         //RC0..5 as outputs
   IO_Port_1_Direction.unused_C=0b00;              //RC6..7 as outputs
   
   port_a_pullups(0x0F);   //Enable pull-up resistors for RA0..3
   port_b_pullups(0x0F);   //Enable pull-up resistors for RB0..3
   
   PreviousExperimentSelection=0b10000; //Used to enter clock speed selection during startup
   
  while(TRUE) //Prevents microprocessor from sleeping
   {
      if(IO_Port_1.ExperimentSelection!=PreviousExperimentSelection) //Detects switch change
      {
         //Set previous experiment to current exp current experiment
         PreviousExperimentSelection=IO_Port_1.ExperimentSelection;
         
         //Reseting outputs,counter and clock indicator (clock state)
         IO_Port_1.Multi_LED=0b000000; //Switches off all LEDs when switch change detected
         counter=0;                    //Reset counter when switch change detected 
         clockstate=0;                 //Reset clock state when switch change detected
         
         //Clock speed selection
         switch(IO_Port_1.ExperimentSelection){ //Detects current switch code
            case 0b0000: //Switch code 0000 detected corresponding to spec 0
            {
               setup_oscillator(OSC_HFINTRC_4MHZ);    //Set clock speed to 4MHz
               break;
            }
            case 0b0001: //Switch code 0001 detected corresponding to spec 1
            {
               setup_oscillator(OSC_HFINTRC_8MHZ);    //Set clock speed to 8MHz
               break;
            }
            default: //Default case for all other specfications & satisfies spec 2
            {
               setup_oscillator(OSC_HFINTRC_16MHZ);   //Set clock speed to 16MHz
            }
         }
      }
      
      //Output and clock manipulation to meet the remaining specifications
      switch(IO_Port_1.ExperimentSelection){ //Detects current switch code
        case 0b0100: //Switch code 0100 detected corresponding to spec 4
         {
         if(clockstate==0) //Checks clock state
            {
               setup_oscillator(OSC_HFINTRC_4MHZ); //Set clock speed to 4MHz
               clockstate=1;                       //Set clockstate to 1
               IO_Port_1.Multi_LED=IO_Port_1.Multi_LED^0b000001; //Toggle RC0 output
               #use delay(clock=4MHZ)              //Inform compiler of current clock speed
               delay_ms(500);                      //Delay 0.5s
               break;                              //breaks out of switch case
            }
         if(clockstate==1) //Checks clock state
            {
               setup_oscillator(OSC_HFINTRC_8MHZ); //Set clock speed to 8MHz
               clockstate=2;                       //Set clockstate to 2
               IO_Port_1.Multi_LED=IO_Port_1.Multi_LED^0b000001; //Toggle RC0 output
               #use delay(clock=8MHZ)              //Inform compiler of current clock speed
               delay_ms(500);                      //Delay 0.5s
               break;                              //breaks out of switch case
            }
         if(clockstate==2) //Checks clock state
            {
               setup_oscillator(OSC_HFINTRC_16MHZ);//Set clock speed to 16MHz
               clockstate=0;                       //Set clockstate to 0
               IO_Port_1.Multi_LED=IO_Port_1.Multi_LED^0b000001; //Toggle RC0 output
               #use delay(clock=16MHZ)             //Inform compiler of current clock speed
               delay_ms(500);                      //Delay 0.5s
               break;                              //breaks out of switch case
            }
         }
         
      case 0b0011: //Switch code 0011 detected corresponding to spec 3
         {
         IO_Port_1.Multi_LED=IO_Port_1.Multi_LED^0b000001;  //Toggle RC0 output
         delay_ms(500);                                     //Delay 0.5s
         break;
         }
         
      case 0b0110: //Switch code 0110 detected corresponding to spec 6
         {
         IO_Port_1.Multi_LED=(IO_Port_1.Multi_LED+0b1)&(0b001111); //Binary up counter
         delay_ms(1000);                                           //Delay 1s 
         break;
         }
         
      case 0b0111: //Switch code 0111 detected corresponding to spec 7
         {
         IO_Port_1.Multi_LED=(IO_Port_1.Multi_LED-0b1)&(0b001111); //Binary down counter
         delay_ms(1000);                                           //Delay 1s
         break;
         }
         
      case 0b1000: //Switch code 1000 detected corresponding to spec 8
         {
         IO_Port_1.Multi_LED=LUT_grey[counter=((counter+1)%16)]; //Grey code up counter
         delay_ms(1000);                                         //Delay 1s
         break;
         }
         
      case 0b1001: //Switch code 1001 detected corresponding to spec 9
         {
         counter=(counter+1)%8; //Counter to loop through the LUT and pure binary up counter
         IO_Port_1.Multi_LED=(LUT_grey[counter]<<3)+counter; //Grey code and binary up counter
         delay_ms(1000);                                     //Delay 1s
         break;
         }
         
      case 0b1010: //Switch code 1010 detected corresponding to spec 10
         {
         IO_Port_2.Multi_LED=IO_Port_2.B_inputs_2&IO_Port_2.B_inputs_1; //RB0..1 AND RB2..3
         break;
         }
         
      case 0b1011: //Switch code 1011 detected corresponding to spec 11
         {
         IO_Port_2.Multi_LED=IO_Port_2.B_inputs_2|IO_Port_2.B_inputs_1; //RB0..1 OR RB2..3
         break;
         }
         
      case 0b1100: //Switch code 1100 detected corresponding to spec 12
         {
         IO_Port_1.Multi_LED=(~IO_Port_1.B_inputs)&(0b001111); //NOT RB0..3
         break;
         }
         
      case 0b1101: //Switch code 1101 detected corresponding to spec 13
         {
         IO_Port_2.Multi_LED=IO_Port_2.B_inputs_2^IO_Port_2.B_inputs_1; //RB0..1 EXOR RB2..3
         break;
         }
         
      case 0b1110: //Switch code 1110 detected corresponding to spec 14
         {
         IO_Port_1.Multi_LED= //SR Latch
         ((IO_Port_1.B_inputs&0b1)+((((~IO_Port_1.B_inputs)>>1)&0b1)&IO_Port_1.Multi_LED))>0;
         break;
         }
         
      case 0b1111: //Switch code 1111 detected corresponding to spec 15
         {
         IO_Port_1.Multi_LED= //Running light
         (IO_Port_1.Multi_LED<<1)+(((IO_Port_1.Multi_LED<<1)&0b111111)==0);
         delay_ms(LUT_delay[IO_Port_1.B_inputs]); //Delay using look up table
         break;
         }     
       }
   }
}
