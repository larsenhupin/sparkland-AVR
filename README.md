## TODO

- [ ] Use ISR timer for interrupt
- [ ] Add multiple / dynamic ADC channels
- [ ] Send multiple values (csv style)
- [ ] Improve float_to_char_array
- [ ] Codify data transfer



## Reading

cat /dev/ttyUSB0

## Writing

echo '1' > /dev/ttyUSB0
echo '0' > /dev/ttyUSB0

# Ensure that icanon is off, disables canonical mode (processes input immediately).

stty -F /dev/ttyUSB0 9600 cs8 -cstopb -parenb -icanon -echo

# Unsure

sudo chmod 666 /dev/ttyUSB0
