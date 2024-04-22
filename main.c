/*
 * ProjectCode.cpp
 *
 * Created: 4/1/2024 3:42:09 PM
 * Author : WillHoover
 */ 

#include <avr/io.h>
#include <math.h>

// Global Variables and Constants
const float d_per_lvl = 1.0588;
volatile int pwm_base; //stores the value of the pwm obtained in the calibration function.
const float moisture_max = 270; //Adjust this value once max level read by the sensor is found
const int moisture_threshold = 64;


//Function Prototypes
int send_to_MAX7221(unsigned char command, unsigned char data); //displays data on 7 segment displays
void wait(volatile int multiple); //classic wait function :3
//void delay_T_msec_timer0(volatile char choice); //Do not use this, this will disrupt all pwm clock cycle shenanigans
void delay_T_msec_timer1(volatile char choice); //will be using this 
int ADC_Conversion(char pin_choice); //completes and ADC at the selected pin
void motor_run(char motor, char direction, int pwm); //runs selected motor at selected speed in selected direction
void motor_off(char motor); //turns off selected motor
void calibration(); //function to determine the pwm value needed to operate motor at wanted angular velocity.
void water_cycle(); //function to complete 1 full watering cycle


int main(void)
{
	//Element Pins 
	// Motor one i.e. the rotation motor
	//PWM with PD6, forward with PB0, backward PD4
	// Motor 2 i.e. pump
	// pwm with PD5, forward with PB1, reverse with PC3
	// LED 1 i.e. Low water level on PC4
	// LED 2 i.e In full use on PC5
	// Water level ADC on PC1
	// Moisture level on PC0
	// Rotation ADC on PC2
	// Calibration start switch on PD0
	// Start test cycle on PD1
	
	//will begin to start referencing rotation motor as just motor and pump motor as pump
	
	//Begin Assignment Statements
	DDRC = 0b0111000; //PC0 to PC2 0's for the three ADC conversions PC# 3 for motor 2 reverse, PC4 and PC5 1's because LEDS are outputs
	PORTC = PORTC | (1<<PC4 | 1<<PC5);
	PORTC = PORTC ^ 0<<PC3; //turning off the LED's and motor to start
	
	DDRD = 0b11110000; // PD5 and PD6 PWM 0, D7 and D3 nothing on them so 0, D4 1 cause direction, D2 0 cause interrupt is input, switches are set as outputs
	PORTD = PORTD & 0<<PD4; //turninng off motor and LED
	PORTD = PORTD | 1<<PD7;
	
	DDRB = 0b00101111; // 7 Segment is PB2,3,5 PB0,1 are motor pins
	PORTB = PORTB & 0<<PB0 & 0<<PB1; //setting motors pins off to start
	//PORTB
	
	//setting up main SPI
	SPCR = 0b01010001; // SPCR = 1<<SPE | 1<<MSTR | 1<<SPR0; // (SPIE = 0, SPE = 1, DORD = 0, MSTR = 1, CPOL = 0, CPHA = 0, SPR1 = 0, SPR0 = 1)
	// enable the SPI, set to Main mode 0, SCK = Fosc/16, lead with MSB
	
	// ADC set up
	PRR = 0x00;
	ADCSRA = 0b10000111;
	ADMUX = 0b00100101;

	send_to_MAX7221(0b00001001,0b00000011); //enable decoding mode
	send_to_MAX7221(0b00001011,0b00000010); //enable scan limit

	send_to_MAX7221(0b00001100,0b00000001); //turn on the display DIG 0
	//send_to_MAX7221(0b00001100,0b00000010); //turn on the display DIG 1
	
	//Set up PWM timers
	TCCR0A = 0b10100011;
	TCCR0B = 0b00000011;
	
	// Variables
	int result;
	char calibration_flag = 1; //test flag for check in purposes
	volatile float water_warning_lvl = 235;
	volatile float moisture_content;
	int t = 0; //time keeping variable 
	
    while (1) 
    {
		
		//test line
		const float moisture_max = 220;
		
		send_to_MAX7221(0b00001100,0b00000001); //turn on the display DIG 0
		//send_to_MAX7221(0b00001100,0b00000010); //turn on the display DIG 1

	   	send_to_MAX7221(0b00000001,0b00000000); //binary 0 on DIG0
		send_to_MAX7221(0b00000010,0b00000000); //binary 0 on DIG1
		//wait(1000);
		
		//key notes: potentiometer has range of 270 degrees with 255 levels to give 1.0588 degrees/level
		//midpoint is roughly level 128 (actual value 127.5)
		// 45 degree rotation would be 42.5 steps, will round to 43 steps
		
		//Start final product code
		
		//Poll for button press to calibrate
		while(calibration_flag){
			if(!(PIND & 0b0000001)){
				wait(50);
				calibration_flag = 0;
			} else {
			wait(50);
			}
		}
		calibration();
		calibration_flag = 1; //resetting flag for use in next button polling
		
		//Poll for button press for first test cycle
		while(calibration_flag){
			if(!(PIND & (1<<PD1))){
				wait(50);
				calibration_flag = 0;
			} else {
				wait(50);
			}
		}
		water_cycle();
		PORTC = (!(PORTC | 1<<PC5)); //Turning on full use light
		PORTC = PORTC | 1<<PC4; //turning off red LED (in case above line turns on
			
		
		//while(1) loop for steady state operation, 
			//read moisture level for soil
				//display moisture level to 7 segment display
				//run water_cycle() if moisture level fails threshold
			//adc check water level
				//light led if water level too low
		
		while(1){
			
			//variables
			volatile int i = 0; //keeps track of 10's place digit of percentage
			volatile int j = 0; //keeps track of 1's place digit of percentage
			
			//read moisture level, need sensors to see exactly how this will be coded
			result = ADC_Conversion(1); // read moisture soil content
			moisture_content = ((moisture_max - result) / moisture_max) * 100.0; //moisture level as 2 digit percentage
			//test line
			wait(10);
			moisture_content = 100 - moisture_content;
			volatile float moisture_value = moisture_content;
			wait(10);
			//need to separate moisture_content into 2 integers.
			while(moisture_content > 10) {
				moisture_content = moisture_content - 10;
				i++;
				send_to_MAX7221(0b00000001,i); //display 10's place on dig 0
			}
			while (moisture_content > 0) {
				moisture_content--;
				j++;
				send_to_MAX7221(0b00000010,j); // display 1's place on dig 1
			}
			send_to_MAX7221(0b00000001,i); //display 10's place on dig 0
			send_to_MAX7221(0b00000010,j); // display 1's place on dig 1
			t = 60; // 60 second delay so water can permeate through soil
			if(moisture_value < moisture_threshold) {
				while(t > 0) {
					//water reservoir level
					result = ADC_Conversion(2);
					if(result > water_warning_lvl) {
						PORTC = ((PORTC | 1<<PC4));
						PORTD = (!(PORTD | 1<<PD7));
						} else {
						PORTC = (!(PORTC | 1<<PC4));
						PORTD = PORTD | 1<<PD7;
					}
					wait(1000);
					t--;
				}
				water_cycle();
			}
			
			//water reservoir level
			result = ADC_Conversion(2);
			if(result > water_warning_lvl) {
				PORTC = ((PORTC | 1<<PC4));
				PORTD = (!(PORTD | 1<<PD7));
			} else {
				PORTC = (!(PORTC | 1<<PC4));
				PORTD = PORTD | 1<<PD7;
			}
			
			wait(1000);
		}
    }
}

//Function to send data to MAX 7221
int send_to_MAX7221(unsigned char command, unsigned char data){
		PORTB = PORTB & 0b11111011; // Clear PB2, which is the SS bit, so that
		// transmission can begin
		SPDR = command; // Send command
		while(!(SPSR & (1<<SPIF))); // Wait for transmission to finish
		SPDR = data; // Send data
		while(!(SPSR & (1<<SPIF))); // Wait for transmission to finish
		PORTB = PORTB | 0b00000100; // Return PB2 to 1, which is the SS bit, to end
		// transmission
		return 0;
}

//WAIT FUNCTION
void wait(volatile int multiple) {
	/* This subroutine calls others to create a delay.
		 Total delay = multiple*T, where T is in msec and is the delay created by the called function.
	
		Inputs: multiple = number of multiples to delay, where multiple is the number of times an actual delay loop is called.
		Outputs: None
	*/
	
	while (multiple > 0) {
		delay_T_msec_timer1(2); 
		multiple--;
	}
} // end wait()

void delay_T_msec_timer1(volatile char choice) {
	//
	// ***Note that since the Timer1 register is 16 bits, the delays can be much higher than shown here.
	// This subroutine creates a delay of T msec using TIMER1 with prescaler on clock, where, for a 16MHz clock:
	//T = 0.125 msec for prescaler set to 8 and count of 250 (preload counter with 65,535-5)
	//T = 1 msec for prescaler set to 64 and count of 250 (preload counter with 65,535-5)
	//T = 4 msec for prescaler set to 256 and count of 250 (preload counter with 65,535-5)
	//T = 16 msec for prescaler set to 1,024 and count of 250 (preload counter with 65,535-5)
	//Default: T = .0156 msec for no prescaler and count of 250 (preload counter with 65,535-5)

	//Inputs: None
	//Outputs: None

	TCCR1A = 0x00; // clears WGM00 and WGM01 (bits 0 and 1) to ensure Timer/Counter is in normal mode.
	TCNT1 = 5;  // preload load TIMER1 with 5 if counting to 255 (count must reach 65,535-5 = 250)
	// or preload with 0 and count to 250

	switch ( choice ) { // choose prescaler
		case 1:
		TCCR1B = 1<<CS11;//TCCR1B = 0x02; // Start TIMER1, Normal mode, crystal clock, prescaler = 8
		break;
		case 2:
		TCCR1B =  1<<CS11 | 1<<CS10;//TCCR1B = 0x03;  // Start TIMER1, Normal mode, crystal clock, prescaler = 64
		break;
		case 3:
		TCCR1B = 1<<CS12;//TCCR1B = 0x04; // Start TIMER1, Normal mode, crystal clock, prescaler = 256
		break;
		case 4:
		TCCR1B = 1<<CS12 | 1<<CS10;//TCCR1B = 0x05; // Start TIMER1, Normal mode, crystal clock, prescaler = 1024
		break;
		default:
		TCCR1A = 1<<CS10;//TCCR1B = 0x01; Start TIMER1, Normal mode, crystal clock, no prescaler
		break;
	}

	//while ((TIFR1 & (0x1<<TOV1)) == 0); // wait for TOV1 to roll over at 255 (requires preload of 65,535-5 to make count = 250)
	// How does this while loop work?? See notes
	while (TCNT1 < 0xfa); // exits when count = 250 (requires preload of 0 to make count = 250)

	TCCR1B = 0x00; // Stop TIMER1
	//TIFR1 = 0x1<<TOV1;  // Clear TOV1 (note that this is an odd bit in that it
	//is cleared by writing a 1 to it)

} // end delay_T_msec_timer1()

int ADC_Conversion(char pin_choice){
	//need to adjust ADMUX register to read the correct pin value
	ADMUX = ADMUX & (1<<MUX0 & 1<<MUX1 & 1<<MUX2 & 1<<MUX3);
	ADMUX = ADMUX |  0b00100000;
	wait(10); //small delay to allow admux bits to fully change
	//ADMUX = ADMUX ^ (1<<MUX0 | 1<<MUX1 | 1<<MUX2 | 1<<MUX3);
	if(pin_choice == 1){
		//set adc to happen at moisture sensor
		//empty if statement because this condition is the default based on above bit assignments and clearing
	}
	if(pin_choice == 2){
		//set adc to happen at water level
		ADMUX = ADMUX | 1<<MUX0 | 0<<MUX1 | 0<<MUX2 | 0<<MUX3;
	}
	if(pin_choice == 3){
		//set adc to happen at rotation potentiometer ie mux bits 0010
		ADMUX = ADMUX | 0<<MUX0 | 1<<MUX1 | 0<<MUX2 | 0<<MUX3;
	}
	ADCSRA = ADCSRA | 0b01000000; // start conversion
	while((ADCSRA & 0b00010000) == 0); //While ADIF is set to 1  keep looping
	return ADCH;
}

void motor_run(char motor, char direction, int pwm){
	
	//need logic to choose motor
	// 1 is rotation motor, 2 is pump motor
	if(motor == 1) { //start rotation motor control
		OCR0A = pwm; //setting how fast the motor is running
		//direction logic
		if(direction == 1) { //1 is forward
			PORTD = (!(PORTD | 1<<PD4));
			PORTB = PORTB | 1<<PB0;
			PORTD = PORTD | 1<<PD6;
		} else if (direction == 0){  //0 is reverse 
			PORTB = PORTB | 0<<PB0;
			PORTD = PORTD | 1<<PD4 | 1<< PD6;
		}
	} else if (motor == 2){ //start pump control
		//no direction needed as the pump will always run in the forward direction
		OCR0B = pwm;
		PORTB = PORTB | 1<<PB1;
		PORTD = PORTD | 1<<PD5;
	}
	PORTD = PORTD | 1<<PD7;
}

void motor_off(char motor){
	if(motor == 1){ //rotaion motor off
		PORTB = PORTB & 0<<PB0;
		PORTD = (!(PORTD | 1<<PD4));
	} else if (motor == 2){ //pump off
		PORTB = PORTB & 0<<PB1;
	} else if (motor == 3) { //both off
		motor_off(1);
		motor_off(2);
	}
	PORTD = PORTD | 1<<PD7;
}

void calibration(){
	//start by getting pwm_base value.
	motor_off(3);
	volatile float position = ADC_Conversion(3); //getting initial position in steps.
	volatile float angular_V = ((ADC_Conversion(3)-position)*d_per_lvl)/2.0; //getting angular velocity for 2 second pulse of motor
	char pwm = 100;
	while (((angular_V) < 3.0) && (angular_V > -3)) { //3.0 degrees/s is the wanted velocity based on motor analysis document
		position = ADC_Conversion(3);
		motor_run(1,1,pwm);
		wait(2000);
		motor_off(1);
		float new_position = ADC_Conversion(3);
		angular_V = ((new_position-position)*d_per_lvl)/2.0;
		motor_run(1,0,pwm);
		wait(2000);
		motor_off(1);
		pwm = pwm + 10;
	}
	pwm_base = pwm; //setting pwm speed for the rest of the program
	
	//getting to correct initial position (halfway through wiper arc) for test cycle and then normal operation
	position = ADC_Conversion(3);
	while(position > 128){ //moving down to center
		if( (position - 128)  < 8){
			motor_run(1,0,(pwm_base-15));
			wait(500);
			position = ADC_Conversion(3);
		}
		else { 
			motor_run(1,0,pwm_base);
			wait(500);
			position = ADC_Conversion(3);
		}
	} motor_off(1);
	while(position < 128){ //moving up to center
		if(128 - position < 8){
			motor_run(1,1,(pwm_base-15));
			wait(500);
			position = ADC_Conversion(3);
		} else {
			motor_run(1,1,pwm_base);
			wait(500);
			position = ADC_Conversion(3);
		}
	} motor_off(1);
}

//for water cycle switched all motor calls from 1 to 0 and visa versa, that may fix it
void water_cycle(){
	//goals
	//rotate motor 45 degrees with pump on, slow motor and pump around the corner, reverse the direction, return to origin, then do on opposite side.
	volatile char position = ADC_Conversion(3);
	//moving (+) 45 deg first
	while (position < 171){
		if((171 - position) < 8) {
			motor_run(1,1,(pwm_base-15));
			motor_run(2,1,80);
			wait(500);
			motor_off(3);
			position = ADC_Conversion(3);
		} else {
			motor_run(1,1,pwm_base);
			motor_run(2,1,95);
			wait(500);
			motor_off(3);
			position = ADC_Conversion(3);
		}
		position = ADC_Conversion(3);
	} //end of first movement 
	//moving (-) 90 deg
	while(position > 85) {
		if((position - 85 < 8)){
			motor_run(1,0,(pwm_base-15));
			motor_run(2,1,80);
			wait(500);
			motor_off(3);
			position = ADC_Conversion(3);
		} else {
			motor_run(1,0,pwm_base);
			motor_run(2,1,95);
			wait(500);
			motor_off(3);
			position = ADC_Conversion(3);
		}
		position = ADC_Conversion(3);
	} //end of second movement
	while(position < 128) {
		if((128 - position) < 8) {
			motor_run(1,1,(pwm_base-15));
			motor_run(2,1,80);
			wait(500);
			motor_off(3);
			position = ADC_Conversion(3);
		} else {
			motor_run(1,1,pwm_base);
			motor_run(2,1,95);
			wait(500);
			motor_off(3);
			position = ADC_Conversion(3);
		}
		position = ADC_Conversion(3);
	}
	motor_off(3);
} //End of function
