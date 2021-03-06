/*
 Blast ! debugger bridge
 Implements a bridge between the Genesis pad port and a TCP socket on the Ethernet shield or the USB serial I/O.
 */

// Define pinout of the genesis pad port (see protocol doc for pin meaning)
#define GEND0 4
#define GEND1 5
#define GEND2 6
#define GEND3 7
#define GENTL 8
#define GENTR 9
#define GENTH 3
#define TCPBRIDGE   // Uncomment to enable TCP server (ethernet shield)
#define BLINKLED 13 // Set to the pin of the led to blink it while running. Undefine to disable blink.

#define SERIAL_BAUDRATE 115200

#if defined (__AVR_ATmega168__) || defined (__AVR_ATmega328P__)
#if GEND0 == 4 && GEND1 == 5 && GEND2 == 6 && GEND3 == 7
// In direct AVR port mode, data lines are on port D 4..7 (pin 4 = D0 .. pin 7 = D3)
#define GEN_AVRPORT
#endif
#endif

#define BLSM_PACKET_MAX 36

#include <SPI.h>         // needed for Arduino versions later than 0018
#ifdef TCPBRIDGE
#include <Ethernet.h>
byte mac[] = { 
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };   // Set MAC address
IPAddress ip(192,168,0,177);              // Server IP
EthernetServer server(2612);            // TCP port
EthernetClient client;
#endif

#ifdef BLINKLED
#define LEDON() do { digitalWrite(BLINKLED, HIGH); } while(0)
#define LEDOFF() do { digitalWrite(BLINKLED, LOW); } while(0)
#else
#define LEDON() do {} while(0)
#define LEDOFF() do {} while(0)
#endif


class GenPort : 
public Stream {
public:
  GenPort() : 
  buflen(0), writeMode(0)
  {
  }

  void begin();
  
  operator bool() const {
    // Returns false if not connected
    if(digitalRead(GENTR) && digitalRead(GENTL) || !digitalRead(GENTH)) {
      // Either a packet is just about to arrive, or the port is not ready for debug.
      for(uint16_t i = 0; i < 10000; ++i) {
        if((!digitalRead(GENTR) || !digitalRead(GENTL)) && digitalRead(GENTH)) {
          // The Genesis is in debug mode !
          return true;
        }
        
#if F_CPU >= 20000000L
      // Don't poll too fast on fast CPUs
      delayMicroseconds(2);
#endif
      }
    } else {
      return true;
    }
    
    return false;
  }

  virtual int available()
  {
    if(!buflen) {
      // Nothing in buffer - check real hardware
      if(writeMode == 0 && digitalRead(GENTR) && !digitalRead(GENTL)) {
        // Genesis is writing : let's check this out
        peek();
      }
    }
    return buflen; // No internal buffering
  }
  virtual int read()
  {
    if(buflen) {
      buflen = 0; 
      return buffer;
    }
    if(writeMode) {
      genStopWrite();
    }
    buffer = genRead();
    if(writeMode) {
      genStartWrite();
    }
    return buffer;
  }
  virtual int read(uint8_t *buf, size_t size)
  {
    uint8_t s = (uint8_t)size;

    if(writeMode) {
      genStopWrite();
    }

    if(buflen)
    {
      // Read the first byte from peek buffer
      *buf = buffer;
      --s;
      buflen = 0;
    }

    while(s--)
    {
      *buf = genRead();
      ++buf;
    }

    if(writeMode) {
      genStartWrite();
    }
    return size;
  }
  virtual int peek()
  {
    if(!buflen) {
      if(writeMode) {
        genStopWrite();
      }
      buffer = genRead();
      if(writeMode) {
        genStartWrite();
      }
      buflen = 1;
    }
    return buffer;
  }
  virtual void flush() {
  }
  virtual size_t write(uint8_t c) { 
    if(!writeMode) {
      genStartWrite();
    }
    uint8_t written = genWrite(c);
    if(!writeMode) {
      genStopWrite();
    }
    return written;
  }
  virtual size_t write(const uint8_t *buf, size_t size)
  {
    uint8_t s = size;
    if(!writeMode) {
      genStartWrite();
      writeMode = 1;
    }
    while(s-- && genWrite(*buf))
    {
      ++buf;
    }
    genStopWrite();
    writeMode = 0;
    return size;
  }
  void setWriteMode(uint8_t newWriteMode)
  {
    if(writeMode == newWriteMode)
    {
      return;
    }
    if(newWriteMode)
    {
      genStartWrite();
    }
    else
    {
      genStopWrite();
    }
    writeMode = newWriteMode;
  }

private:
  uint8_t buffer;
  uint8_t buflen; // 0 or 1;
  uint8_t writeMode; // 0 or 1
  uint8_t genRead();
  uint8_t genWrite(uint8_t c);
  void genStartWrite();
  void genStopWrite();
  void resetPins();
};


void GenPort::begin() {
  pinMode(GENTH, INPUT);
  digitalWrite(GENTH, HIGH);
  pinMode(GENTL, INPUT);
  digitalWrite(GENTL, LOW);
  pinMode(GENTR, INPUT);
  digitalWrite(GENTR, LOW);

  // Release data lines
#ifdef GEN_AVRPORT
  DDRD &=~ 0xF0;
  PORTD &=~ 0xF0;
#else
  pinMode(GEND0, INPUT);
  digitalWrite(GEND0, LOW);
  pinMode(GEND1, INPUT);
  digitalWrite(GEND1, LOW);
  pinMode(GEND2, INPUT);
  digitalWrite(GEND2, LOW);
  pinMode(GEND3, INPUT);
  digitalWrite(GEND3, LOW);
#endif

  buflen = 0;
  writeMode = 0;

  // Wait for stabilized neutral state
  for(uint16_t i = 0; i < 10000; ++i)
  {
#ifdef GEN_AVRPORT
    while((PIND & 0xF0) != 0xF0 || !digitalRead(GENTH) || !digitalRead(GENTL) || digitalRead(GENTR)) {
      i = 0;
    }
#else
    while(!digitalRead(GEND0) || !digitalRead(GEND1) || !digitalRead(GEND2) || !digitalRead(GEND3) || !digitalRead(GENTH) || !digitalRead(GENTL) || digitalRead(GENTR))
      i = 0;
#endif

#if F_CPU >= 20000000L
    // Don't poll too fast on fast CPUs
    delayMicroseconds(2);
#endif
  }
}

uint8_t GenPort::genWrite(uint8_t n) {
  while(!digitalRead(GENTL)) { // Wait for ack to be released
    if(digitalRead(GENTR)) {
      // Genesis left listening mode !
      return 0;
    }
  } 
  if(digitalRead(GENTR)) {
    // Genesis left listening mode !
    return 0;
  }

  // Send high nybble
#ifdef GEN_AVRPORT
  PORTD = (PORTD & 0x0F) | (n & 0xF0);
#else
  digitalWrite(GEND3, (n & 0X80) ? HIGH : LOW);
  digitalWrite(GEND2, (n & 0X40) ? HIGH : LOW);
  digitalWrite(GEND1, (n & 0X20) ? HIGH : LOW);
  digitalWrite(GEND0, (n & 0X10) ? HIGH : LOW);
#endif
  digitalWrite(GENTH, LOW); // Pulse clock until ack
  while(digitalRead(GENTL)) { // Wait for ack
    if(digitalRead(GENTR)) {
      // Genesis left listening mode !
      return 0;
    }
  }

  // Send low nybble
#ifdef GEN_AVRPORT
  PORTD = (PORTD & 0x0F) | (n << 4);
#else
  digitalWrite(GEND3, (n & 0X08) ? HIGH : LOW);
  digitalWrite(GEND2, (n & 0X04) ? HIGH : LOW);
  digitalWrite(GEND1, (n & 0X02) ? HIGH : LOW);
  digitalWrite(GEND0, (n & 0X01) ? HIGH : LOW);
#endif

  digitalWrite(GENTH, HIGH); // Release clock
  return 1;
}

void GenPort::genStartWrite() {
  // Wait until genesis is ready to receive data
  while(digitalRead(GENTR))
    if(!digitalRead(GENTH)) return; // The genesis is polling a pad ... Ouch !

  // Set clock to output
  digitalWrite(GENTH, HIGH);
  pinMode(GENTH, OUTPUT);

  // Set data pins to output
#ifdef GEN_AVRPORT
  DDRD |= 0xF0;
#else
  pinMode(GEND0, OUTPUT);
  pinMode(GEND1, OUTPUT);
  pinMode(GEND2, OUTPUT);
  pinMode(GEND3, OUTPUT);
#endif
}

void GenPort::genStopWrite() {
  // Release clock
  digitalWrite(GENTH, HIGH);
  pinMode(GENTH, INPUT);

  while(!digitalRead(GENTL) && !digitalRead(GENTR)); // Wait for ack to be released

  // Release data pins
#ifdef GEN_AVRPORT
  DDRD &=~ 0xF0;
  PORTD |= 0xF0;
#else
  pinMode(GEND0, INPUT);
  digitalWrite(GEND0, HIGH);
  pinMode(GEND1, INPUT);
  digitalWrite(GEND1, HIGH);
  pinMode(GEND2, INPUT);
  digitalWrite(GEND2, HIGH);
  pinMode(GEND3, INPUT);
  digitalWrite(GEND3, HIGH);
#endif
}

uint8_t GenPort::genRead() {
  uint8_t v = 0;

  // Wait until the genesis is ready to send
  while(!digitalRead(GENTR))
    if(!digitalRead(GENTH)) return 0; // The genesis is polling a pad ... Ouch !

  // Wait for clock
  while(digitalRead(GENTL))
    if(!digitalRead(GENTH)) return 0; // The genesis is polling a pad ... Ouch !

  // Read high nybble
#ifdef GEN_AVRPORT
  v = PIND & 0xF0;
#else
  v = digitalRead(GEND3) ? 0x80 : 0;
  v |= digitalRead(GEND2) ? 0x40 : 0;
  v |= digitalRead(GEND1) ? 0x20 : 0;
  v |= digitalRead(GEND0) ? 0x10 : 0;
#endif

  // Acknowledge nybble read
  pinMode(GENTH, OUTPUT);
  digitalWrite(GENTH, LOW);

  // Wait for clock
  while(!digitalRead(GENTL))
  {
    if(!digitalRead(GENTR)) {
      // Genesis quit send mode
      digitalWrite(GENTH, HIGH);
      pinMode(GENTH, INPUT); // Reset TH to input mode
      return 0;
    }
  }

  // Read low nybble
#ifdef GEN_AVRPORT
  v |= PIND >> 4;
#else
  v |= digitalRead(GEND3) ? 0x08 : 0;
  v |= digitalRead(GEND2) ? 0x04 : 0;
  v |= digitalRead(GEND1) ? 0x02 : 0;
  v |= digitalRead(GEND0) ? 0x01 : 0;
#endif

  // Acknowledge
  digitalWrite(GENTH, HIGH); // Quick load to high
  pinMode(GENTH, INPUT);

  return v;
}

uint8_t computePacketLen(uint8_t header)
{
  if((header & 0x20) && (header & 0xC0)) {
    // Write command : data in header LSB
    uint8_t s = header & 0x1F;
    if(!s) { 
      return 36; 
    }
    return s + 4;
  }

  // Other commands : no data
  return 4;
}

void forwardPacket(Stream *in, Stream *out)
{
  uint8_t packet[BLSM_PACKET_MAX];

  // Read header and compute packet length
  uint8_t header = in->read();
  uint8_t packetLen = computePacketLen(header);

  packet[0] = header;

  // Read address and payload
  in->readBytes((char*)&(packet[1]), packetLen - 1);

  if(in == &Serial)
  {
    // Send acknowledge
    Serial.write(0x3F);
  }

  if(header == 0x20 && packet[1] == 'V' && packet[2] == 'Q' && packet[3] == '\n') {
    // Handle communication test
    packet[1] = '2'; // Protocol version (0..F)
    packet[2] = '1'; // Implementation version
    packet[3] = '0'; // Implementation revision
    in->write(packet, packetLen);
    return;
  }

  // Write packet to other end
  out->write(packet, packetLen);
}

Stream *pc;
GenPort genport;
GenPort *gen = &genport;

void setup()
{
#ifdef BLINKLED
  pinMode(BLINKLED, OUTPUT); 
#endif

  Serial.begin(SERIAL_BAUDRATE);
  pc = &Serial; // By default, use serial port

  LEDON();

  genport.begin();

#ifdef TCPBRIDGE
  Ethernet.begin(mac, ip);
  server.begin();
#endif
}

void loop()
{
#ifdef TCPBRIDGE
  if(!client.connected())
  {
    client.stop();
    pc = &Serial;
  }
  if(pc == &Serial) {
    client = server.available();
    if(client) {
      pc = &client;
    } 
  }
#endif

  if(pc->available()) {
    // Store and forward packet
    forwardPacket(pc, gen);
  }

  LEDON();

  while(!gen) {
    // Stay in idle mode when the Genesis is not in debug mode
    gen->begin();
  }

  if(gen->available())
  {
    // Store and forward packet
    forwardPacket(gen, pc);
  }

  LEDOFF();
}


