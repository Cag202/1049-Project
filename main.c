/*
 * ProjectCode.cpp
 *
 * Created: 4/1/2024 3:42:09 PM
 * Author : WillHoover
 */ 

#include <avr/io.h>

int send_to_MAX7221(unsigned char command, unsigned char data);
void wait(volatile int multiple, volatile char time_choice);
void delay_T_msec_timer0(volatile char choice);

int main(void)
{
    DDRB = 0b00101111; //set correctly i think
	//PORTB
	DDRC = 0b0111000//PC0 to PC2 0's for the three ADC conversions PC# 1 for motor 2 reverse, PC4 and PC5 1's because LEDS are outputs
	//PORTC PC4 and PC5 1 LED's off in beginning 
	DDRD = 0b00010011// PD5 and PD6 PWM 0, D7 and D3 nothing on them so 0, D4 1 cause direction, D2 0 cause interrupt is input, switches are set as outputs
	//PORTD
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
		delay_T_msec_timer0(time_choice); 
		multiple--;
	}
} // end wait()

void delay_T_msec_timer0(volatile char choice) {
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
	
	TCCR0A = 0x00; // clears WGM00 and WGM01 (bits 0 and 1) to ensure Timer/Counter is in normal mode.
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
	
} // end delay_T_msec_timer0()

