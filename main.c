#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <string.h>

#define F_CPU 16000000UL
#define BUAD 9600
#define BRC ((F_CPU / 16 / BUAD) - 1)
#define TX_BUFFER_SIZE 128

#include <util/delay.h>

char serialBuffer[TX_BUFFER_SIZE];
uint8_t serialReadPos = 0;
uint8_t serialWritePos = 0;
volatile uint16_t adcValue = 0;

void appendSerial(char c);
void serialWrite(char c[]);
void setupADC(void);
void startADC(void);

int main(void) {
    // Set up UART
    UBRR0H = (BRC >> 8);
    UBRR0L = BRC;
    UCSR0B = (1 << TXEN0) | (1 << TXCIE0); // Enable transmitter and interrupt
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); // Set 8-bit data size

    // Set up ADC
    setupADC();

    sei(); // Enable global interrupts

    // serialWrite("ADC to UART transmission\n");

    while (1) {
        // Main loop can be used for other tasks if necessary
       startADC();
        _delay_ms(20);
    }

    return 0;
}

void appendSerial(char c) {
    serialBuffer[serialWritePos] = c;
    serialWritePos++;

    if (serialWritePos >= TX_BUFFER_SIZE) {
        serialWritePos = 0; // Wrap around if buffer is full
    }
}

void serialWrite(char c[]) {
    for (uint8_t i = 0; i < strlen(c); i++) {
        appendSerial(c[i]);
    }

    if (UCSR0A & (1 << UDRE0)) { // If transmit buffer is ready
        UDR0 = serialBuffer[serialReadPos];
        serialReadPos = (serialReadPos + 1) % TX_BUFFER_SIZE;
    }
}

// ADC setup function
void setupADC() {
    ADMUX = (1 << REFS0) | (1 << MUX0); // Use Vcc as reference and channel ADC1 (pin A1)
    ADCSRA = (1 << ADEN) | (1 << ADIE) | (1 << ADPS1) | (1 << ADPS2); // Enable ADC, ADC interrupt, and prescaler
    DIDR0 = (1 << ADC1D); // Disable digital input on ADC1 to reduce noise
    // startADC(); // Start the ADC conversion
}

// Start ADC conversion
void startADC() {
    ADCSRA |= (1 << ADSC); // Start the conversion
}

// UART transmit interrupt service routine
ISR(USART_TX_vect) {
    if (serialReadPos != serialWritePos) {
        UDR0 = serialBuffer[serialReadPos];
        serialReadPos++;
        if (serialReadPos >= TX_BUFFER_SIZE) {
            serialReadPos = 0; // Wrap around if we reach the end of the buffer
        }
    }

    // _delay_ms(100);
}

// ADC interrupt service routine
ISR(ADC_vect) {
    // if (ADCSRA & (1 << ADIF)) {
    adcValue = ADC; // Read ADC value
    float voltage = (adcValue / 1024.0) * 5.0; // Convert ADC value to voltage
    
    int voltage2 = (int) (voltage * 100);
    // Prepare a string with the ADC value in the format "Voltage: X.XXV"
    char voltageStr[30];
    snprintf(voltageStr, sizeof(voltageStr), "Voltage: %dV\r\n", voltage2);
    
    serialWrite(voltageStr); // Send the string over UART

       // ADCSRA |= (1 << ADIF);  // Clear the interrupt flag

    // }
    // Start the next ADC conversion
    // startADC();
}
