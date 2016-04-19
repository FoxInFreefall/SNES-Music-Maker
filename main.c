#include <msp430.h>
#include <time.h>

#define CLOCK_PIN BIT1
#define LATCH_PIN BIT2
#define DATA_PIN BIT3

#define SPEAKER_PIN1 BIT6
#define SPEAKER_PIN2 BIT1

#define BTN_R BIT4
#define BTN_L BIT5
#define BTN_X BIT6
#define BTN_A BIT7

#define BTN_RIGHT BIT8
#define BTN_LEFT BIT9
#define BTN_DOWN BITA
#define BTN_UP BITB

#define BTN_ST BITC
#define BTN_SEL BITD
#define BTN_Y BITE
#define BTN_B BITF

/*
 * main.c
 */
//_delay_cycles(580000);	// For delaying for a number of cycles

void init_launchpad() {
	WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer

	P1DIR = 0;
	P2DIR = 0;

	P1OUT = 0;
	P2OUT = 0;

	P1REN = 0;
	P1SEL = 0;
	P1SEL2 = 0;
}

void setup_snes() {
	P1DIR |= CLOCK_PIN;		// Out
	P1DIR |= LATCH_PIN;		// Out
	P1DIR &= ~DATA_PIN;		// In

	P1REN |= DATA_PIN;		// Enable pullup resistor on data pin

	P1OUT |= CLOCK_PIN;		// Clock starts high
	P1OUT &= ~LATCH_PIN;	// Latch starts low
	P1OUT |= DATA_PIN;		// Data active high
}

unsigned int poll_snes() {
	unsigned int result = 0;
	volatile unsigned int i = 0;

	// Send latch (begin new poll)
	P1OUT |= LATCH_PIN;
	P1OUT &= ~LATCH_PIN;

	for (i = 0; i < 12; i++) {
		// Send clock cycle (begin new button)
		P1OUT |= CLOCK_PIN;
		P1OUT &= ~CLOCK_PIN;

		result = result << 1;
		//TODO: are () needed for ~P1IN?
		if ((~P1IN) & DATA_PIN) {
			result |= BIT0;
		}
	}

	for (i = 0; i < 3; i++) {
		// Send clock cycle (begin new button)
		P1OUT |= CLOCK_PIN;
		P1OUT &= ~CLOCK_PIN;

		result = result << 1;
	}

	// Finish poll
	P1OUT |= CLOCK_PIN;

	result = result << 1;

	return result;
}

void setup_speaker() {
	P1DIR |= SPEAKER_PIN1;
	P1SEL |= SPEAKER_PIN1;
	CCTL1 = OUTMOD_7;				// CCR1 reset/set
	TACTL = TASSEL_2 | MC_1;		// SMCLK, upmode

	P2DIR |= SPEAKER_PIN2;
	P2SEL |= SPEAKER_PIN2;
	TA1CCTL1 = OUTMOD_7;
	TA1CTL = TASSEL_2 | MC_1;
}

void set_freq1(float freq) {
	unsigned int period = 1000000 / freq;
	float dutyCycle = CCR1 / (float)CCR0;

	CCR0 = period;
	CCR1 = period * dutyCycle;
}

void set_vol1(float vol) {
	float dutyCycle = vol / 2;		// 0 ~ 0.5

	CCR1 = CCR0 * dutyCycle;
}

void set_freq2(float freq) {
	unsigned int period = 1000000 / freq;
	float dutyCycle = TA1CCR1 / (float)TA1CCR0;

	TA1CCR0 = period;
	TA1CCR1 = period * dutyCycle;
}

void set_vol2(float vol) {
	float dutyCycle = vol / 2;		// 0 ~ 0.5

	TA1CCR1 = TA1CCR0 * dutyCycle;
}

// Defining own pow function means we don't need to import math.h (big)
int pow(int x, int y) {
	int i = 0;
	int result = 1;
	for (i = 0; i < y; i++)
		result = result * x;
	return result;
}

 int main(void) {
	init_launchpad();

	setup_snes();
	setup_speaker();

	float f[] = {
			16.3515625,		//C		0
			17.32390625,	//C#	1
			18.3540625,		//D		2
			19.44546875,	//D#	3
			20.60171875,	//E		4
			21.82671875,	//F		5
			23.0778125,		//F#	6
			24.4996875,		//G		7
			25.9565625,		//G#	8
			27.5,			//A		9
			29.1353125,		//A#	10
			30.86765625,	//B		11
	};

	int key_e[] = {
				4,		//E		=> F
				6,		//F#	=> G
				8,		//G#	=> G
				9,		//A		=> A#
				11,		//B		=> C
				13,		//C#	=> D
				15,		//D#	=> D
				16		//E		=> F
	};

	int key_f[] = {
				5,		//F		=> F#
				7,		//G		=> G#
				9,		//A		=> G#
				10,		//A#	=> B
				12,		//C		=> C#
				14,		//D		=> D#
				16,		//E		=> F
				17		//F		=> F#
	};
	int key_fa[] = {
				6,		//F		=> F#
				8,		//G		=> G#
				8,		//A		=> G#
				11,		//A#	=> B
				13,		//C		=> C#
				15,		//D		=> D#
				17,		//E		=> F
				18		//F		=> F#
	};

	int button_order[] = {
			BTN_DOWN,
			BTN_B,
			BTN_LEFT,
			BTN_RIGHT,
			BTN_Y,
			BTN_A,
			BTN_UP,
			BTN_X
	};

	set_freq1(f[0]);
	set_vol1(1);

	set_freq2(f[0]);
	set_vol2(1);

	// Public
	volatile int master_octave = 4;

	// Private: buttons
	unsigned int buttons = 0;
	unsigned int last_buttons = 0;
	unsigned int new_buttons = 0;
	unsigned int released_buttons = 0;

	// Private
	int i = 0;

	// Private: octave
	int octave_shift = 0;

	// Private: recording
//	int memory[] = {
//			41, 0, 0, 0,
//			41, 0, 0, 0,
//			41, 0, 0, 0,
//			41, 0, 0, 0,
//			41, 0, 0, 0,
//			41, 0, 0, 0,
//			41, 0, 0, 0,
//			41, 0, 0, 0,
//	};
//	int tempo = 60;

	// Private: buttons to notes
	int buttons_playing1 = -1;
	int buttons_playing2 = -1;
	int n_buttons_playing = 0;

//	clock_t last_measure = clock();
    for (;;) {
		buttons = poll_snes();
    	new_buttons = (last_buttons ^ buttons) & buttons;
    	released_buttons = (last_buttons ^ buttons) & last_buttons;

    	// Octave
    	if (new_buttons & BTN_L) {
    		if (master_octave - 1 >= 0) {
    			master_octave--;
    			octave_shift = (~buttons & BTN_SEL) ? -1 : 0;
    		}
    	} else if (new_buttons & BTN_R) {
    		master_octave++;
    		octave_shift = (~buttons & BTN_SEL) ? 1 : 0;
    	}

    	if (released_buttons & BTN_L) {
    		if (octave_shift == -1) {
    			master_octave++;
				octave_shift = 0;
    		}
		} else if (released_buttons & BTN_R) {
			if (octave_shift == 1) {
				master_octave--;
				octave_shift = 0;
			}
		}

    	// Notes
    	buttons_playing1 = -1;
    	buttons_playing2 = -1;
    	n_buttons_playing = 0;
		for (i = 0; i < 8 && n_buttons_playing < 2; i++) {
			if (buttons & button_order[i]) {
				if (n_buttons_playing == 0)
					buttons_playing1 = i;
				else
					buttons_playing2 = i;
				n_buttons_playing++;
			}
    	}

    	// Logic
    	float vol1 = 0;
    	int button = buttons_playing1;
    	if (button != -1) {
    		int index = (buttons & BTN_SEL) ? key_fa[button] : key_f[button];
    		int octave = index / 12 + 1 + master_octave;
    		if (octave < 1)
    			octave = 1;
    		index = index % 12;
    		float freq = f[index] * pow(2, octave);

			set_freq1(freq);

			vol1 = 0.5;
    	}
    	set_vol1(vol1);

    	float vol2 = 0;
    	button = buttons_playing2;
		if (button != -1) {
			int index = (buttons & BTN_SEL) ? key_fa[button] : key_f[button];
			int octave = index / 12 + 1 + master_octave;
			if (octave < 1)
				octave = 1;
			index = index % 12;
			float freq = f[index] * pow(2, octave);

			set_freq2(freq);

			vol2 = 0.5;
		}
		set_vol2(vol2);

//    	else {
//    		clock_t now = clock();
//    		clock_t time_since_last_measure = now - last_measure;
//    		float freq = 0;
//			if (now - last_measure <= CLOCKS_PER_SEC / 4.0) {
//				freq = CLOCKS_PER_SEC/2000.0;
//			}
//			if (now - last_measure >= CLOCKS_PER_SEC) {
//				last_measure = now;
//			}
//			set_freq(freq);
//			vol = 0.05;
//    	}

		last_buttons = buttons;
    }
}
