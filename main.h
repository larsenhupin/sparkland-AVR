#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#define F_CPU 16000000UL
#define BUAD 9600
#define BRC ((F_CPU / 16 / BUAD) - 1)
#define TX_BUFFER_SIZE 128
#define RX_BUFFER_SIZE 128

#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))

typedef struct {
    char buffer[TX_BUFFER_SIZE];
    uint8_t readPos;
    uint8_t writePos;
} SerialTX;


typedef struct {
    char buffer[RX_BUFFER_SIZE];
    uint8_t readPos;
    uint8_t writePos;
} SerialRX;

void setupTimer(void);
void setupPWM(void);
void setupADC(void);
void setupUART(void);
void startADC(void);
char peekCharSerial(void);
char readCharSerial(void);
void writeSerial(char c[]);
void millivoltToCharArray(uint16_t millivolt, char *millivoltBuffer);
void concatenateBufferToLine(char *line, char *buffer0, char *buffer1);