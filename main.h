#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#define F_CPU 16000000UL
#define BUAD 9600
#define BRC ((F_CPU / 16 / BUAD) - 1)
#define TX_BUFFER_SIZE 128


typedef struct {
    char buffer[TX_BUFFER_SIZE];
    uint8_t readPos;
    uint8_t writePos;
} SerialTX;

void setupTimer(void);
void setupADC(void);
void setupUART(void);
void startADC(void);
void writeSerial(char c[]);
void millivoltToCharArray(uint16_t millivolt, char *millivoltBuffer);