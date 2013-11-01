/*
 Blast ! debugger bridge
 Implements a bridge between the Genesis pad port and a TCP socket on the Ethernet shield or the USB serial I/O.
 */

// Define pinout of the genesis pad port (see doc for pin meaning)
// In direct mode, data lines are on port D 4..7 (pin 4 = D0 .. pin 7 = D3)

#define GEN_DIRECTPORT       // Direct port mode : use AVR PORT D directly (faster)
#define TCPBRIDGE        // Uncomment to enable TCP server (ethernet shield)

// Pinout
#define GEND0 4
#define GEND1 5
#define GEND2 6
#defibe GEND3 7
#define GENTL 8
#define GENTR 9
#define GENTH 3
#define BLSM_PACKET_MAX 40


#define GENCLK GENTH
#define GENACK GENTL
#define GENOUT GENTR

#include <avr/wdt.h>
#include <SPI.h>         // needed for Arduino versions later than 0018
#ifdef TCPBRIDGE
#include <Ethernet.h>
byte mac[] = { 
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };   // Set MAC address
IPAddress ip(192,168,0,177);              // Server IP
EthernetServer server(2612);            // TCP port
EthernetClient client;
#endif


class GenPort : 
public Stream {
public:
  GenPort() : 
  buflen(0), writeMode(0)
  {
  }

  void begin();

  virtual int available()
  {
    if(!buflen) {
      // Nothing in buffer - check real hardware
      if(writeMode == 0 && !digitalRead(GENCLK) && digitalRead(GENOUT)) {
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
    genWrite(c); 
    if(!writeMode) {
      genStopWrite();
    }
    return 1;
  }
  virtual size_t write(const uint8_t *buf, size_t size)
  {
    uint8_t s = size;
    if(!writeMode) {
      genStartWrite();
      writeMode = 1;
    }
    while(s--)
    {
      genWrite(*buf);
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
  void genWrite(uint8_t c);
  void genStartWrite();
  void genStopWrite();
  void resetPins();
};


void GenPort::begin() {
  pinMode(GENACK, INPUT);
  pinMode(GENOUT, INPUT);
  pinMode(GENCLK, INPUT);

  // Release control lines
  digitalWrite(GENACK, LOW);
  digitalWrite(GENOUT, LOW);
  digitalWrite(GENCLK, LOW);

  // Release data lines
#ifdef GEN_DIRECTPORT
  DDRD &=~ 0xF0;
  PORTD &=~ 0xF0;
#else
  pinMode(GEND0, INPUT);
  pinMode(GEND1, INPUT);
  pinMode(GEND2, INPUT);
  pinMode(GEND3, INPUT);
#endif

  buflen = 0;
  writeMode = 0;
  
  // Wait for stabilized all-high (neutral state)
  for(uint8_t j = 20; j; --j)
  {
    for(uint8_t i = 255; i; --i)
    {
#ifdef GEN_DIRECTPORT
      while((PIND & 0x0F) != 0x0F || !digitalRead(GENACK) || !digitalRead(GENOUT) || !digitalRead(GENCLK))
        i = 0;
#else
      while(!digitalRead(GEND0) || !digitalRead(GEND1) || !digitalRead(GEND2) || !digitalRead(GEND3) || !digitalRead(GENACK) || !digitalRead(GENOUT) || !digitalRead(GENCLK))
        i = 0;
#endif
      delayMicroseconds(200);
    }
  }
}

void GenPort::genWrite(uint8_t n) {
  while(!digitalRead(GENACK)); // Wait for ack to be released

  // Send high nybble
#ifdef GEN_DIRECTPORT
  PORTD = (PORTD & 0x0F) | (n & 0xF0);
#else
  digitalWrite(GEND3, (n & 0X80) ? HIGH : LOW);
  digitalWrite(GEND2, (n & 0X40) ? HIGH : LOW);
  digitalWrite(GEND1, (n & 0X20) ? HIGH : LOW);
  digitalWrite(GEND0, (n & 0X10) ? HIGH : LOW);
#endif
  digitalWrite(GENCLK, LOW); // Pulse clock until ack
  while(digitalRead(GENACK)); // Wait for ack

  // Send low nybble
#ifdef GEN_DIRECTPORT
  PORTD = (PORTD & 0x0F) | (n << 4);
#else
  digitalWrite(GEND3, (n & 0X08) ? HIGH : LOW);
  digitalWrite(GEND2, (n & 0X04) ? HIGH : LOW);
  digitalWrite(GEND1, (n & 0X02) ? HIGH : LOW);
  digitalWrite(GEND0, (n & 0X01) ? HIGH : LOW);
#endif
  digitalWrite(GENCLK, HIGH); // Release clock
}

void GenPort::genStartWrite() {
  // Set data pins to output high
#ifdef GEN_DIRECTPORT
  PORTD |= 0xF0;
  DDRD |= 0xF0;
#else
  pinMode(GEND0, OUTPUT);
  digitalWrite(GEND0, HIGH);
  pinMode(GEND1, OUTPUT);
  digitalWrite(GEND1, HIGH);
  pinMode(GEND2, OUTPUT);
  digitalWrite(GEND2, HIGH);
  pinMode(GEND3, OUTPUT);
  digitalWrite(GEND3, HIGH);
#endif

  digitalWrite(GENOUT, LOW);
  digitalWrite(GENCLK, HIGH);
  pinMode(GENOUT, OUTPUT);
  pinMode(GENCLK, OUTPUT);

  // Signal the genesis that a write is coming  
  digitalWrite(GENOUT, LOW);
}

void GenPort::genStopWrite() {
  while(!digitalRead(GENACK)); // Wait for ack to be released

  // Help the genesis pull-up to go high and release data pins
#ifdef GEN_DIRECTPORT
  PORTD |= 0xF0;
  DDRD &=~ 0xF0;
#else
  digitalWrite(GEND0, HIGH);
  digitalWrite(GEND1, HIGH);
  digitalWrite(GEND2, HIGH);
  digitalWrite(GEND3, HIGH);
  pinMode(GEND0, INPUT);
  pinMode(GEND1, INPUT);
  pinMode(GEND2, INPUT);
  pinMode(GEND3, INPUT);
#endif

  // Release clock
  pinMode(GENCLK, INPUT);  

  // Quick release output mode
  digitalWrite(GENOUT, HIGH);
  pinMode(GENOUT, INPUT);
}

uint8_t GenPort::genRead() {
  char v = 0;

  // Wait for clock
  while(digitalRead(GENCLK));

  // Read high nybble
#ifdef GEN_DIRECTPORT
  v = PIND & 0xF0;
#else
  v = digitalRead(GEND3) ? 0x80 : 0;
  v |= digitalRead(GEND2) ? 0x40 : 0;
  v |= digitalRead(GEND1) ? 0x20 : 0;
  v |= digitalRead(GEND0) ? 0x10 : 0;
#endif

  // Acknowledge nybble read
  pinMode(GENACK, OUTPUT);
  digitalWrite(GENACK, LOW); // Quick load to low

  // Wait for clock
  while(!digitalRead(GENCLK));

  // Read low nybble
#ifdef GEN_DIRECTPORT
  v |= PIND >> 4;
#else
  v |= digitalRead(GEND3) ? 0x08 : 0;
  v |= digitalRead(GEND2) ? 0x04 : 0;
  v |= digitalRead(GEND1) ? 0x02 : 0;
  v |= digitalRead(GEND0) ? 0x01 : 0;
#endif

  // Acknowledge
  digitalWrite(GENACK, HIGH); // Quick load to high
  pinMode(GENACK, INPUT);

  return v;
}

uint8_t computeDataLen(uint8_t header)
{
  if((header & 0xC0) && (header & 0x20)) {
    // Write command : data in header LSB
    char s = header & 0x1F;
    if(!s) { 
      return 32; 
    }
    return s;
  }
  
  // Other commands : no data
  return 0;
}

void forwardPacket(Stream *in, Stream *out)
{
  uint8_t packet[BLSM_PACKET_MAX];

  // Read header and compute packet length
  uint8_t header = in->read();
  uint8_t packetLen = computeDataLen(header) + 4;

  packet[0] = header;

  // Read address and payload
  in->readBytes((char*)&(packet[1]), packetLen - 1);

  // Write packet to other end
  out->write(packet, packetLen);
}

Stream *pc;
GenPort genport;
GenPort *gen = &genport;

void setup()
{ 
  Serial.begin(115200);
  pc = &Serial; // By default, use serial port
  genport.begin();
#ifdef TCPBRIDGE
  Ethernet.begin(mac, ip);
  server.begin();
#endif
//  wdt_enable(WDTO_8S); // Use the watchdog in case the Genesis hangs during a transfer
}

void loop()
{
//  wdt_reset();
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
    // Per-byte forward (slow)
    //gen->write(pc->read());

    // Store and forward packet
    forwardPacket(pc, gen);
  }
  if(gen->available())
  {
    // Per-byte forward (slow)
    //pc->write(gen->read());

    // Store and forward packet
    forwardPacket(gen, pc);
  }
}

