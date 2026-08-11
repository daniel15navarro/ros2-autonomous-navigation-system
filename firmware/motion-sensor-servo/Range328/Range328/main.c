#ifndef F_CPU
#define F_CPU 16000000UL
#endif
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <stdio.h>

#include "USART0.h"

FILE *fio_0 = &usart0_Stream;

/* HC-SR04 Ultrasonic Distance Sensor.
 Trig -> PORTD7
 Echo -> PORTB0
 */

// --- PIN DEFINITIONS ---
#define TRIG_PIN PIND7
#define ECHO_PIN PINB0
#define ALERT_LED PINB1

// --- PCA9685 I2C DEFINITIONS ---
#define PCA9685_ADDR 0x80 // 0x40 shifted left by 1 for 8-bit AVR TWI
#define SERVO_A 0         // Channel 0
#define SERVO_B 1         // Channel 1

// --- SERVO TICK VALUES (For 50Hz / 20ms Frame) ---
// INVERTED LOGIC: claws start CLOSED, open on detection, close again to grip
#define CLOSED_A_TICK 140  // Servo A CLOSED (was DEPLOY)
#define CLOSED_B_TICK 490  // Servo B CLOSED (was DEPLOY)
#define OPEN_A_TICK   510  // Servo A OPEN   (was NEUTRAL)
#define OPEN_B_TICK   110  // Servo B OPEN   (was NEUTRAL)
uint16_t tune_A = 140; // Servo A new silent closed value!
uint16_t tune_B = 490; // Servo B new silent closed value!

// --- GLOBAL VARIABLES ---
volatile unsigned char MIP = 0;
volatile unsigned int ECHOHigh = 0, ECHOLow = 0, ECHOHighPipe = 0;
volatile uint8_t timerFlag = 0;
volatile uint8_t system_armed = 0; // 0 = Safe (Transit/closed), 1 = Armed (searching)
volatile uint32_t initial_contact_distance = 0;

// --- FUNCTION PROTOTYPES ---
void Trigger(void);
uint32_t CalculateDistance(void);
void setupTimerInterrupt(void);

// TWI (I2C) Prototypes
void TWI_Init(void);
void TWI_Start(void);
void TWI_Stop(void);
void TWI_Write(uint8_t data);

// PCA9685 Prototypes
void PCA9685_WriteReg(uint8_t reg, uint8_t value);
void PCA9685_Init(void);
void PCA9685_SetServo(uint8_t channel, uint16_t off_tick);

// ==========================================
// 1. HC-SR04 ULTRASONIC ISRs & FUNCTIONS
// ==========================================
ISR (TIMER1_OVF_vect) {
	if(ECHOHighPipe >= 2) {
		TIMSK1 = 0;
		TCCR1B = 0;
		MIP = 0xFF; // Timeout
	}
	ECHOHighPipe++;
}

ISR(TIMER1_CAPT_vect) {
	if ((TCCR1B & (1 << ICES1)) != 0) {
		TCCR1B |= (1 << CS11);
		TCCR1B &= ~(1 << ICES1);
		} else {
		ECHOLow = TCNT1;
		ECHOHigh = ECHOHighPipe;
		TIMSK1 = (1 << OCIE1B);
	}
}

ISR(TIMER1_COMPA_vect) {
	PORTD &= ~(1 << TRIG_PIN);
	TIMSK1 = (1 << ICIE1) | (1 << TOIE1);
	TCCR1B = (1 << ICES1);
}

ISR (TIMER1_COMPB_vect) {
	TIMSK1 = 0;
	TCCR1B = 0;
	MIP = 0;
}

void Trigger(void) {
	if (MIP == 0) {
		MIP = 1;

		ECHOHighPipe = 0;
		ECHOHigh = 0;
		ECHOLow = 0;
		TCNT1 = 0;

		DDRD |= (1 << TRIG_PIN);
		DDRD &= ~(1 << ECHO_PIN);

		PORTD &= ~(1 << TRIG_PIN);
		PORTD |= (1 << TRIG_PIN);

		OCR1A = 20;
		TIFR1 = 0xFF;
		TCCR1A = 0;
		TCCR1B = (1 << WGM12) | (1 << CS11);
		TIMSK1 = (1 << OCIE1A);
	}
}

uint32_t CalculateDistance(void) {
	if (ECHOHigh > 0) return 999;
	uint32_t dist = (uint32_t)ECHOLow / 116;
	if (dist > 400) return 999;
	return dist;
}

// ==========================================
// 2. TIMERS & TWI (I2C) DRIVER
// ==========================================
void setupTimerInterrupt(void) {
	TCCR0A = (1 << WGM01);
	TCCR0B = (1 << CS02) | (1 << CS00);
	OCR0A = 156;
	TIMSK0 = (1 << OCIE0A);
}

ISR(TIMER0_COMPA_vect) {
	timerFlag = 1;
}

void TWI_Init(void) {
	TWSR = 0x00;
	TWBR = 72;
	TWCR = (1 << TWEN);
}

void TWI_Start(void) {
	TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
	while (!(TWCR & (1 << TWINT)));
}

void TWI_Stop(void) {
	TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);
}

void TWI_Write(uint8_t data) {
	TWDR = data;
	TWCR = (1 << TWINT) | (1 << TWEN);
	while (!(TWCR & (1 << TWINT)));
}

// ==========================================
// 3. PCA9685 DRIVER
// ==========================================
void PCA9685_WriteReg(uint8_t reg, uint8_t value) {
	TWI_Start();
	TWI_Write(PCA9685_ADDR);
	TWI_Write(reg);
	TWI_Write(value);
	TWI_Stop();
}

void PCA9685_Init(void) {
	PCA9685_WriteReg(0x00, 0x10);
	PCA9685_WriteReg(0xFE, 121);
	PCA9685_WriteReg(0x00, 0x20);
	_delay_ms(1);
}

void PCA9685_SetServo(uint8_t channel, uint16_t off_tick) {
	uint8_t reg_base = 0x06 + (4 * channel);
	TWI_Start();
	TWI_Write(PCA9685_ADDR);
	TWI_Write(reg_base);
	TWI_Write(0);
	TWI_Write(0);
	TWI_Write(off_tick & 0xFF);
	TWI_Write(off_tick >> 8);
	TWI_Stop();
}

// ==========================================
// 4. MAIN EXECUTION
// ==========================================
int main(void) {
	init_uart0(103);
	sei();

	fprintf_P(fio_0, PSTR("Initiating system. Please wait...\n\n\r"));

	// --- ALERT LED STARTUP SEQUENCE ---
	DDRB |= (1 << ALERT_LED);
	PORTB |= (1 << ALERT_LED);
	_delay_ms(750);
	PORTB &= ~(1 << ALERT_LED);
	_delay_ms(500);
	PORTB |= (1 << ALERT_LED);
	_delay_ms(500);
	PORTB &= ~(1 << ALERT_LED);
	_delay_ms(1000);

	// --- INIT HARDWARE ---
	setupTimerInterrupt();
	TWI_Init();
	PCA9685_Init();

	// Start with claws CLOSED (safe transit mode)
	PCA9685_SetServo(SERVO_A, CLOSED_A_TICK);
	PCA9685_SetServo(SERVO_B, CLOSED_B_TICK);

	fprintf_P(fio_0, PSTR("\nSystem ready. Claws CLOSED. Waiting for ROS2 ARM command...\n\r"));

	// State Tracking
	uint8_t ping_timer_count = 0;
	uint8_t ping_sent = 0;
	uint8_t consecutive_pings = 0;
	uint8_t claws_open = 0;      // 0 = closed, 1 = open (enveloping)
	uint8_t claws_gripping = 0;  // 0 = not gripping, 1 = gripping cargo

	while (1) {
		// --- 1. NON-BLOCKING TRIGGER (Every 100ms) ---
		if (timerFlag == 1) {
			timerFlag = 0;
			ping_timer_count++;

			if (ping_timer_count >= 10) {
				ping_timer_count = 0;
				if (MIP == 0) {
					Trigger();
					ping_sent = 1;
				}
			}
		}

		// --- 2. DETECTION & OPEN LOGIC (when armed, claws closed, not yet gripping) ---
		if (ping_sent == 1 && MIP == 0) {
			ping_sent = 0;

			if (system_armed == 1 && claws_open == 0 && claws_gripping == 0) {
				uint32_t distance = CalculateDistance();

				// DEBUGGING
				//fprintf_P(fio_0, PSTR("DEBUG - Sensor sees: %lu cm\n\r"), distance);

				// Detection window: 1cm to 12cm
				if (distance > 1 && distance <= 12) {
					if (consecutive_pings == 0) {
						initial_contact_distance = distance;
						consecutive_pings = 1;
						PORTB ^= (1 << ALERT_LED);
						fprintf_P(fio_0, PSTR("Life vessel detected! Ping 1/5 (Dist: %lu cm)\n\r"), distance);
					} else {
						consecutive_pings++;
						PORTB ^= (1 << ALERT_LED);
						fprintf_P(fio_0, PSTR("Life vessel ping %d/5 (Dist: %lu cm)\n\r"), consecutive_pings, distance);

						if (consecutive_pings >= 4) {
							// Kinematic check: object getting closer
							if (initial_contact_distance > distance && (initial_contact_distance - distance) >= 2) {
								fprintf_P(fio_0, PSTR("*** LIFE VESSEL CONFIRMED. OPENING CLAWS! ***\n\r"));
								PORTB |= (1 << ALERT_LED);

								_delay_ms(1000); // Brief stabilise

								// OPEN claws to envelope cargo
								PCA9685_SetServo(SERVO_A, OPEN_A_TICK);
								PCA9685_SetServo(SERVO_B, OPEN_B_TICK);

								claws_open = 1;
								system_armed = 0;
								consecutive_pings = 0;

								fprintf_P(fio_0, PSTR("Claws OPEN. Robot driving into position...\n\r"));
							} else {
								fprintf_P(fio_0, PSTR("Static object rejected. Resetting.\n\r"));
								consecutive_pings = 0;
								PORTB &= ~(1 << ALERT_LED);
							}
						}
					}
				} else {
					if (consecutive_pings > 0) {
						fprintf_P(fio_0, PSTR("Life Vessel lost. Resetting counter.\n\r"));
						consecutive_pings = 0;
						PORTB &= ~(1 << ALERT_LED);
					}
				}
			}
		} else if (MIP == 0xFF) {
			ping_sent = 0;
			MIP = 0;
			if (claws_gripping == 0 && consecutive_pings > 0) {
				consecutive_pings = 0;
			}
		}
		
		
		// --- 3. ROS2 SERIAL COMMANDS ---
		if (uart0_RxCount() > 0) {
			uint8_t key = uart0_getc();

			// 'A' = Arm system (start searching, claws still closed)
			if (key == 'A' || key == 'a') {
				system_armed = 1;
				consecutive_pings = 0;
				fprintf_P(fio_0, PSTR("\nROS2: ARMED. Searching for life vessel...\n\r"));
			}

			// 'C' = Close claws to grip cargo (robot has fully enveloped)
			if (key == 'C' || key == 'c') {
				PCA9685_SetServo(SERVO_A, CLOSED_A_TICK);
				PCA9685_SetServo(SERVO_B, CLOSED_B_TICK);
				claws_open = 0;
				claws_gripping = 1;
				PORTB |= (1 << ALERT_LED);
				fprintf_P(fio_0, PSTR("\nROS2: Claws CLOSED — life vessel gripped!\n\r"));
			}

			// 'D' = Drop/release cargo (at destination)
			if (key == 'D' || key == 'd') {
				PCA9685_SetServo(SERVO_A, OPEN_A_TICK);
				PCA9685_SetServo(SERVO_B, OPEN_B_TICK);
				claws_open = 1;
				claws_gripping = 0;
				system_armed = 0;
				PORTB &= ~(1 << ALERT_LED);
				fprintf_P(fio_0, PSTR("\nROS2: Claws OPEN — life vessel released.\n\r"));
			}

			// 'R' = Manual reset (close claws, disarm)
			if (key == 'R' || key == 'r') {
				PCA9685_SetServo(SERVO_A, CLOSED_A_TICK);
				PCA9685_SetServo(SERVO_B, CLOSED_B_TICK);
				claws_open = 0;
				claws_gripping = 0;
				system_armed = 0;
				consecutive_pings = 0;
				PORTB &= ~(1 << ALERT_LED);
				fprintf_P(fio_0, PSTR("\nManual Reset: Claws CLOSED.\n\r"));
			}

			// 'B' = Break/exit
			if (key == 'B' || key == 'b') {
				fprintf_P(fio_0, PSTR("\nExiting.\n\r"));
				PORTB &= ~(1 << ALERT_LED);
				break;
			}
		}
	
	//}
	/*
	// --- TEMPORARY LIVE TUNING MODE ---
		if (uart0_RxCount() > 0) {
			uint8_t key = uart0_getc();

			// 'W' and 'S' to micro-step Servo A
			if (key == 'w') tune_A += 2;
			if (key == 's') tune_A -= 2;

			// 'O' and 'L' to micro-step Servo B
			if (key == 'o') tune_B += 2;
			if (key == 'l') tune_B -= 2;

			// Apply the new ticks immediately
			PCA9685_SetServo(SERVO_A, tune_A);
			PCA9685_SetServo(SERVO_B, tune_B);

			// Print the current exact values to your serial monitor
			fprintf_P(fio_0, PSTR("Servo A: %u | Servo B: %u\n\r"), tune_A, tune_B);
			
		}*/
	}

	return 0;
}