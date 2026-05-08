    #include "main.h"

    SerialTX serialTX = {.readPos = 0, .writePos = 0};
    SerialRX serialRX = {.readPos = 0, .writePos = 0};
    volatile uint16_t valuesADC[2];
    volatile uint8_t channelADC = 0;
    uint8_t firstPass = 1;

    int main(void) {
        setupTimer();
        // setupPWM();
        setupADC();
        setupUART();
        sei();

        while (1) {
            
            char c = readCharSerial();

            if (c == '1') {
                sbi(PORTB, PORTB0);
            }
            else if (c == '0') {
                cbi(PORTB, PORTB0);
            }

        }

        return 0;
    }

    void setupTimer() {
        TCCR0A = (1 << WGM01); // Set to CTC mode
        OCR0A = 195; // Count from 0 to 195
        TIMSK0 = (1 << OCIE0A); // Enable interrupt
        TCCR0B = (1 << CS02 | (1 << CS00)); // Start at 1024 prescalar
    }

    void setupPWM() {
        TCCR2A = (1 << WGM21) | (1 << WGM20) | (1 << COM2A1); // Use fast-PWM and non-inverting
        TCCR2B = (1 << CS22); // Start at 64 prescaler
        OCR2A = 0; // Set duty cycle (from 0 to 255)
        DDRB |= (1 << PB3); // Output on PB3

        TIMSK2 = (1 << OCIE2B); // Enabled interrupt one timer2
    }

    void setupADC() {
        ADMUX = (1 << REFS0); // Use Vcc as reference
        ADCSRA = (1 << ADEN) | (1 << ADIE) | (1 << ADPS1) | (1 << ADPS2); // Enable ADC, ADC interrupt, and prescaler
        DIDR0 = (1 << ADC0D) | (1 << ADC1D); // Disable digital input on both ADC0 and ADC1 to reduce noise
    }

    void setupUART() {
        UCSR0B = (1 << TXEN0) | (1 << TXCIE0) | (1 << RXEN0) | (1 << RXCIE0); // Enable transmitter/receiver and interrupt
        UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); // Set 8-bit data frame
        UBRR0H = (BRC >> 8);
        UBRR0L = BRC;

        DDRB = (1 << PORTB0); // Set the direction register to PORTB0
    }

    void startADC() {
        channelADC = 0; // Reset to ADC0
        ADMUX = (ADMUX & 0xF0) | channelADC; // Force start on ADC0, 0xF0 = 11110000
        ADCSRA |= (1 << ADSC);  // Start conversion
    }

    // Timer2
    ISR(TIMER2_COMPB_vect) {
        static uint8_t duty = 0;
        static int8_t step = 3;

        OCR2A = duty; // Update PWM duty cycle 
        duty += step;

        if (duty == 0 || duty == 255) {
            step = -step; // reverse direction
        }
    }

    // Timer0
    ISR(TIMER0_COMPA_vect) {
        startADC();
    }

    // Analog digital converter
    ISR(ADC_vect) {
        valuesADC[channelADC] = ADC;

        if (channelADC == 0) {
            channelADC = 1;
            ADMUX = (ADMUX & 0xF0) | 1; // Set to ADC1, 0xF0 = 11110000
            ADCSRA |= (1 << ADSC);
        }
        else {
            if (firstPass == 1) {
                writeSerial("\n\n"); // Flush the buffer
                writeSerial("adc0 (mv),adc1 (mv)\n");
                firstPass = 0;
            }

            channelADC = 0;
            char line[32];
            char buffer0[16], buffer1[16];

            uint16_t millivolt0 = (valuesADC[0] * 5000UL) / 1023;
            uint16_t millivolt1 = (valuesADC[1] * 5000UL) / 1023;

            millivoltToCharArray(millivolt0, buffer0);
            millivoltToCharArray(millivolt1, buffer1);
            concatenateBufferToLine(line, buffer0, buffer1); // Put both values on one line
            writeSerial(line);
        }
    }

    // UART RX
    ISR(USART_RX_vect) {
        
        serialRX.buffer[serialRX.writePos] = UDR0;
        serialRX.writePos++;

        if (serialRX.writePos >= RX_BUFFER_SIZE) {
            serialRX.writePos = 0;
        }
    }

    // UART TX
    ISR(USART_TX_vect) {
        if (serialTX.readPos != serialTX.writePos) {

            UDR0 = serialTX.buffer[serialTX.readPos];
            serialTX.readPos++;

            if (serialTX.readPos >= TX_BUFFER_SIZE) {
                serialTX.readPos = 0;
            }
        }
    }

    char peekCharSerial(void) {
        char ret = '\0';

        if (serialRX.readPos != serialRX.writePos) {

            ret = serialRX.buffer[serialRX.readPos];
        }

        return ret;
    }

    char readCharSerial(void) {
        char ret = '\0';

        if (serialRX.readPos != serialRX.writePos) {
            ret = serialRX.buffer[serialRX.readPos];

            serialRX.readPos++;

            if (serialRX.readPos >= RX_BUFFER_SIZE) {
                serialRX.readPos = 0;
            }
        }

        return ret;
    }

    void writeSerial(char c[]) {

        // Append serial
        for (uint8_t i = 0; i < strlen(c); i++) {
            serialTX.buffer[serialTX.writePos] = c[i];
            serialTX.writePos++;

            if (serialTX.writePos >= TX_BUFFER_SIZE) {
                serialTX.writePos = 0;
            }
        }

        // Transmit serial
        if (UCSR0A & (1 << UDRE0)) { // Wait for transmit buffer to be ready
            UDR0 = serialTX.buffer[serialTX.readPos];
            serialTX.readPos = (serialTX.readPos + 1) % TX_BUFFER_SIZE;
        }
    }

    void millivoltToCharArray(uint16_t millivolt, char *millivoltBuffer) {
        char buffer[6];

        if (millivolt == 0) {
            millivoltBuffer[0] = '0';
            millivoltBuffer[1] = '\0';
            return;
        }

        uint8_t i = 0;
        while (millivolt > 0) {
            buffer[i++] = (millivolt % 10) + '0';
            millivolt /= 10;
        }

        // Reverse
        for (uint8_t j = 0; j < i; j++) {
            millivoltBuffer[j] = buffer[i - j - 1];
        }

        millivoltBuffer[i] = '\0';
    }

    void concatenateBufferToLine(char *line, char *buffer0, char *buffer1) {
        
        uint8_t i = 0;
        while (buffer0[i] != '\n' && buffer0[i] != '\0') {
            line[i] = buffer0[i];
            i++;
        }
        line[i++] = ',';

        uint8_t j = 0;
        while (buffer1[j] != '\n' && buffer1[j] != '\0') {
            line[i++] = buffer1[j++];
        }
        
        line[i++] = '\n';
        line[i] = '\0';
    
        return;
    }