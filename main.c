/*
 * ProjectCode.cpp
 *
 * Created: 4/1/2024 3:42:09 PM
 * Author : WillHoover
 */ 

#include <avr/io.h>

//Function Prototypes
int send_to_MAX7221(unsigned char command, unsigned char data); //displays data on 7 segment displays
void wait(volatile int multiple, volatile char time_choice); //classic wait function :3
//void delay_T_msec_timer0(volatile char choice); //Do not use this, this will disrupt all pwm clock cycle shenanigans
void delay_T_msec_timer1(volatile char choice); //will be using this 
int ADC_Conversion(char pin_choice); //completes and ADC at the selected pin
void motor_run(char motor, char direction, int pwm); //runs selected motor at selected speed in selected direction
void motor_off(char motor); //turns off selected motor

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
	// Calibration start switch on PC4
	// Start test cycle on PC5
	
	
	//Begin Assignment Statements
    DDRB = 0b00101111; // 7 Segment is PB2,3,5 PB0,1 are motor pins
	PORTB = PORTB | 0<<PB0 | 0<<PB1; //setting motors pins off to start
	//PORTB
	
	DDRC = 0b0111000//PC0 to PC2 0's for the three ADC conversions PC# 1 for motor 2 reverse, PC4 and PC5 1's because LEDS are outputs
	PORTC = PORTC | 0<<PC4 | 0<<PC5 | 0<<PC1; //turning off the LED's and motor to start
	
	DDRD = 0b00010011// PD5 and PD6 PWM 0, D7 and D3 nothing on them so 0, D4 1 cause direction, D2 0 cause interrupt is input, switches are set as outputs
	PORTD = PORTD | 0<<PD4;
	//setting up main SPI
	SPCR = 0b01010001; // SPCR = 1<<SPE | 1<<MSTR | 1<<SPR0; // (SPIE = 0, SPE = 1, DORD = 0, MSTR = 1, CPOL = 0, CPHA = 0, SPR1 = 0, SPR0 = 1)
	// enable the SPI, set to Main mode 0, SCK = Fosc/16, lead with MSB

	send_to_MAX7221(0b00001001,0b00000011); //enable decoding mode
	send_to_MAX7221(0b00001011,0b00000010); //enable scan limit

	send_to_MAX7221(0b00001100,0b00000001); //turn on the display DIG 0
	send_to_MAX7221(0b00001100,0b00000010); //turn on the display DIG 1

    while (1) 
    {
		send_to_MAX7221(0b00001100,0b00000001); //turn on the display DIG 0
		send_to_MAX7221(0b00001100,0b00000010); //turn on the display DIG 1
	    //not everything just a baseline cause i know that were going to have to loop it to constantly check
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
void wait(volatile int multiple, volatile char time_choice) {
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


/* void delay_T_msec_timer0(volatile char choice) {
    //*** delay T ms **
    /* This subroutine creates a delay of T msec using TIMER0 with prescaler on clock, where, for a 16MHz clock:
    		for Choice = 1: T = 0.125 msec for prescaler set to 8 and count of 250 (preload counter with 5)
    		for Choice = 2: T = 1 msec for prescaler set to 64 and count of 250 (preload counter with 5)
    		for Choice = 3: T = 4 msec for prescaler set to 256 and count of 250 (preload counter with 5)
    		for Choice = 4: T = 16 msec for prescaler set to 1,024 and count of 250 (preload counter with 5)
			for Choice = Default: T = .0156 msec for no prescaler and count of 250 (preload counter with 5)
	
			Inputs: None
			Outputs: None
	*/
	
/*	TCCR0A = 0x00; // clears WGM00 and WGM01 (bits 0 and 1) to ensure Timer/Counter is in normal mode.
	TCNT0 = 0;  // preload value for testing on count = 250
	// preload value for alternate test on while loop: TCNT0 = 5;  // preload load TIMER0  (count must reach 255-5 = 250)
	
	switch ( choice ) { // choose prescaler
		case 1:
			TCCR0B = 0b00000010; //1<<CS01;	TCCR0B = 0x02; // Start TIMER0, Normal mode, crystal clock, prescaler = 8
		break;
		case 2:
			TCCR0B =  0b00000011; //1<<CS01 | 1<<CS00;	TCCR0B = 0x03;  // Start TIMER0, Normal mode, crystal clock, prescaler = 64
		break;
		case 3:
			TCCR0B = 0b00000100; //1<<CS02;	TCCR0B = 0x04; // Start TIMER0, Normal mode, crystal clock, prescaler = 256
		break; 
		case 4:
			TCCR0B = 0b00000101; //1<<CS02 | 1<<CS00; TCCR0B = 0x05; // Start TIMER0, Normal mode, crystal clock, prescaler = 1024
		break;
		default:
			TCCR0B = 0b00000001; //1<<CS00; TCCR0B = 0x01; Start TIMER0, Normal mode, crystal clock, no prescaler
		break;
	}
	
	while (TCNT0 < 0xFA); // exits when count = 250 (requires preload of 0 to make count = 250)
	// alternate test on while loop: while ((TIFR0 & (0x1<<TOV0)) == 0); // wait for TOV0 to roll over:
	// How does this while loop work?? See notes
	
	TCCR0B = 0x00; // Stop TIMER0
	//TIFR0 = 0x1<<TOV0;  // Alternate while loop: Clear TOV0 (note that this is a nonintuitive bit in that it is cleared by writing a 1 to it)
	
} */ // end delay_T_msec_timer0() 

int ADC_Conversion(char pin_choice){
	//need to adjust ADMUX register to read the correct pin value
	if(pin_choice == 1){
		//set adc to happen at moisture sensor
	}
	if(pin_choice == 2){
		//set adc to happen at water level
		ADMUX = ADMUX | 0<<MUX0 | 1<<MUX1 | 0<<MUX2 | 0<<MUX3;
	}
	if(pin_choice == 3){
		//set adc to happen at rotation potentiometer ie mux bits 0010
		ADMUX = ADMUX | 1<<MUX0 | 0<<MUX1 | 0<<MUX2 | 0<<MUX3;
	}
	ADCSRA = ADCSRA | 0b01000000; // start conversion
	while((ADCSRA & 0b00010000) == 0); //While ADIF is set to 1  keep looping
	return ADCH;
}

void motor_run(char motor, char direction, int pwm){
	
	//need logic to choose motor
	// 1 is rotation motor, 2 is pump motor
	if(motor == 1) { //start rotaion motor control
		OCR0A = pwm; //setting how fast the motor is running
		//direction logic
		if(direction == 0) { //0 is reverse
			PORTB = PORTB | 1<<PB0;
			PORTD = PORTD | 0<<PD4;
		} else if (direction == 1){ //1 is forward
			PORTB = PORTB | 0<<PB0;
			PORTD = PORTD | 1<<PD4;
		}
	} else if (motor == 2){ //start pump control
		//no direction needed as the pump will always run in the forward direction
		OCR0B = pwm;
		PORTB = PORTB | 1<<PB1;
	}
}

void motor_off(char motor){
	if(motor == 1){ //rotaion motor off
		PORTB = PORTB | 0<<PB0;
		PORTD = PORTD | 0<<PD4;
	} else if (motor == 2){ //pump off
		PORTB = PORTB | 0<<PB1;
	} else if (motor == 3) { //both off
		motor_off(1);
		motor_off(2);
	}
	
}

