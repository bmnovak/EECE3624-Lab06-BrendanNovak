/*
 * Lab06.c
 *
 * Created: 10/4/2022 9:16:57 AM
 * Initial Coder: jfhutton
 * Current Coder: Brendan Novak
 * Modified:      10/10/2026
 *
 * This lab uses hardware LEDs wired to PORTA, an interrupt from the joystick center
 * button, and a timer interrupt to build a game loop program that can display 
 * different patterns on the LEDs.
 *
 * While there are many "down and dirty" ways to get this coding done, try to 
 * remember your coding and data structures classes.  Things like ENUM, Arrays, Functions
 * could help make for more elegant coding.
 *
 */ 

#include <avr/io.h>             // Needed for AVR IO defines
#include <avr/interrupt.h>      // Needed for AVR interrupt defines

#define LEDS                PORTA    // alias PORTA

// Enumerate the possible LED flashing patterns
enum patterns {
    LowToHigh = 0,
    HighToLow,
    BackAndForth,
    BonusPattern
};

// Global variables for communication between ISRs and main
const unsigned char TCNT0_COUNT_SET = 0x8E; // Count for 1ms loop (Provided by Prof Hutton)
volatile enum patterns Mode = LowToHigh;    // Current active pattern state
volatile unsigned char Tick = 0;            // Millisecond tick counter incremented by Timer0 ISR

int main(void){
	// Variables for main tracking
	unsigned char current_led = 0;
	signed char step_dir = 1;
	enum patterns last_mode = LowToHigh;
	
	// State machine initialization.
	
	// Initialization for LEDs
	// Set direction for A ports.
	DDRA = 0xFF;  // Set the Direction for all PORTA pins to be outputs
	LEDS = 0xFF;  // Set the PORTA for all pins to be high (i.e. OFF for active-low LEDs)
	
	// Initialization for Timer Interrupt
	// With a 7.3MHz crystal we have a 0.137us period
	// To build a 1ms timer "tick" we set prescaler to 1/64, TCNT0 = 0x8E, and enable overflow interrupt
	TCCR0 = (1<<CS02);
	TCNT0 = TCNT0_COUNT_SET;
	TIMSK = (1<<TOIE0);
	
	// Port Initialization
	// Configure the joystick button pin (PD0 / INT0) as an input
	DDRD &= ~(1 << DDD0); 
	
	// Interrupt Enable Block
	// Configure INT0 to trigger on the rising edge (releasing the center joystick pushbutton)
	EICRA = (1<<ISC01) | (1<<ISC00);  
	EIMSK = (1<<INT0);                 // Enable External Interrupt 0[cite: 6]
	
	// Enable Global Interrupts
	sei();
	
	// Main Loop (Game Loop)
	while (1) {
		// Wait until Tick reaches 50ms interval
		if (Tick >= 50) {
			Tick = 0; // Reset tick counter
			
			// Reset pattern position and direction if Mode changed
			if (last_mode != Mode) {
				last_mode = Mode;
				current_led = (Mode == HighToLow) ? 7 : 0;
				step_dir = 1;
			}
			
			// Execute pattern logic based on current Mode
			switch (Mode) {
				case LowToHigh:
					LEDS = ~(1 << current_led); // Turn on single LED active-low
					current_led++;
					if (current_led > 7) {
						current_led = 0;
					}
					break;
					
				case HighToLow:
					LEDS = ~(1 << current_led);
					if (current_led == 0) {
						current_led = 7;
					} else {
						current_led--;
					}
					break;
					
				case BackAndForth:
					LEDS = ~(1 << current_led);
					if (current_led == 7) {
						step_dir = -1;
					} else if (current_led == 0) {
						step_dir = 1;
					}
					current_led += step_dir;
					break;
					
				case BonusPattern:
					// Bonus pattern: Nibble / pair scanning back and forth
					{
						static unsigned char bonus_state = 0;
						if (bonus_state == 0) {
							LEDS = ~(0x03); // Pins A0, A1 ON
							bonus_state = 1;
						} else if (bonus_state == 1) {
							LEDS = ~(0x0C); // Pins A2, A3 ON
							bonus_state = 2;
						} else if (bonus_state == 2) {
							LEDS = ~(0x30); // Pins A4, A5 ON
							bonus_state = 3;
						} else {
							LEDS = ~(0xC0); // Pins A6, A7 ON
							bonus_state = 0;
						}
					}
					break;
			}
		}
	}
}

// Timer0 overflow ISR triggered every 1ms
ISR(TIMER0_OVF_vect){
	TCNT0 = TCNT0_COUNT_SET; // Reload initial count value for 1ms interval[cite: 6]
	Tick++;                  // Increment millisecond tick counter[cite: 5, 7]
}

// Hardware Pin Interrupt ISR for INT0 (triggered when center joystick button is released)
ISR(INT0_vect){
	// Cycle through the four pattern modes (0 -> 1 -> 2 -> 3 -> 0)[cite: 6]
	if (Mode == LowToHigh) {
		Mode = HighToLow;
	} else if (Mode == HighToLow) {
		Mode = BackAndForth;
	} else if (Mode == BackAndForth) {
		Mode = BonusPattern;
	} else {
		Mode = LowToHigh;
	}
}