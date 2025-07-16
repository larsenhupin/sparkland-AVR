## TODO

- [x] Make github repo for code I don't want to loose
- [x] Use ISR timer for interrupt
- [ ] Solve the stuff with 10-bit and 1023.0
- [ ] Improve float_to_char_array
- [ ] Add multiple / dynamic ADC channels
- [ ] Send multiple values (csv style)
- [ ] Codify data transfer
- [ ] Enable Asynchronus bidirection I/O (add RX with TX)
- [ ] Add columns titles


## Reading

cat /dev/ttyUSB0

## Writing

echo '1' > /dev/ttyUSB0 
echo '0' > /dev/ttyUSB0

# Ensure that icanon is off, disables canonical mode (processes input immediately).

stty -F /dev/ttyUSB0 9600 cs8 -cstopb -parenb -icanon -echo

# Unsure

sudo chmod 666 /dev/ttyUSB0
