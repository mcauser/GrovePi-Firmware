#include <Wire.h>
#include "MMA7660.h"
#include "DS1307.h"
#include "DHT.h"
#include "TM1637Display.h"
MMA7660 acc;
DS1307 clock;
DHT dht;
TM1637Display seg;

#define SLAVE_ADDRESS 0x04
int number = 0;
int state = 0;

int cmd[5];
int index=0;
int flag=0;
int i;
byte val=0,b[9],float_array[4];
int aRead=0;
byte accFlag=0,clkFlag=0;
int8_t accv[3];

void setup() 
{
    Serial.begin(9600);         // start serial for output
    Wire.begin(SLAVE_ADDRESS);

    Wire.onReceive(receiveData);
    Wire.onRequest(sendData);

    Serial.println("Ready!");
}
int pin;
int j;
void loop()
{
  long dur,RangeCm;
  if(index==4 && flag==0)
  {
    flag=1;
    //Digital Read
    if(cmd[0]==1)
      val=digitalRead(cmd[1]);
      
    //Digital Write
    if(cmd[0]==2)
      digitalWrite(cmd[1],cmd[2]);
      
    //Analog Read
     if(cmd[0]==3)
     {
      aRead=analogRead(cmd[1]);
      b[1]=aRead/256;
      b[2]=aRead%256;
     }
      
    //Set up Analog Write
    if(cmd[0]==4)
      analogWrite(cmd[1],cmd[2]);
        
    //Set up pinMode
    if(cmd[0]==5)
      pinMode(cmd[1],cmd[2]);
    
    //Ultrasonic Read
    if(cmd[0]==7)
    {
      pin=cmd[1];
      pinMode(pin, OUTPUT);
      digitalWrite(pin, LOW);
      delayMicroseconds(2);
      digitalWrite(pin, HIGH);
      delayMicroseconds(5);
      digitalWrite(pin,LOW);
      pinMode(pin,INPUT);
      dur = pulseIn(pin,HIGH);
      RangeCm = dur/29/2;
      b[1]=RangeCm/256;
      b[2]=RangeCm%256;
      //Serial.println(b[1]);
      //Serial.println(b[2]);
    }
    //Accelerometer x,y,z, read
    if(cmd[0]==20)
    {
      if(accFlag==0)
      {
        acc.init();
        accFlag=1;
      }
      acc.getXYZ(&accv[0],&accv[1],&accv[2]);
      b[1]=accv[0];
      b[2]=accv[1];
      b[3]=accv[2];
    }
    //RTC tine read
    if(cmd[0]==30)
    {
      if(clkFlag==0)
      {
        clock.begin();
        //Set time the first time
        //clock.fillByYMD(2013,1,19);
        //clock.fillByHMS(15,28,30);//15:28 30"
	//clock.fillDayOfWeek(SAT);//Saturday
	//clock.setTime();//write time to the RTC chip
        clkFlag=1;
      }
      clock.getTime();
      b[1]=clock.hour;
      b[2]=clock.minute;
      b[3]=clock.second;
      b[4]=clock.month;
      b[5]=clock.dayOfMonth;
      b[6]=clock.year;
      b[7]=clock.dayOfMonth;
      b[8]=clock.dayOfWeek;  
    }
    //Grove temp and humidity sensor pro
    //40- Temperature
    if(cmd[0]==40)
    {
      if(cmd[2]==0)
        dht.begin(cmd[1],DHT11);
      else if(cmd[2]==1)
        dht.begin(cmd[1],DHT22);
      else if(cmd[2]==2)
        dht.begin(cmd[1],DHT21);
      else if(cmd[2]==3)
        dht.begin(cmd[1],AM2301);
      float t= dht.readTemperature();
      float h= dht.readHumidity();
      //Serial.print(t);
      //Serial.print("#");
      byte *b1=(byte*)&t;
      byte *b2=(byte*)&h;
      for(j=0;j<4;j++)
        b[j+1]=b1[j];
      for(j=4;j<8;j++)
        b[j+1]=b2[j-4];
    }
    
    
    // Grove 4 digit display (7 segment)
    // http://www.seeedstudio.com/wiki/Grove_-_4-Digit_Display
    // clock,dio,vcc,gnd
    // cmd 70	pin	dec1	dec2	dec value without leading zeros
    // cmd 71	pin	dec1	dec2	dec value with leading zeros
    // cmd 72	pin	bright	skip	set brightness
    // cmd 73	pin	index	dec	set individual digit
    // cmd 74	pin	index	binary	set individual segment
    // cmd 75	pin	left	right	set left and right with colon
    // cmd 76	pin	an pin	seconds	analog read
    // cmd 77	pin	skip	skip	display on
    // cmd 78	pin	skip	skip	display off
    if(cmd[0]==70)
    {
      // show decimal value without leading zeros
      seg.begin(cmd[1], cmd[1]+1);  // clock, dio
      seg.showNumberDec((cmd[2] * 256) + cmd[3], false);  // number, leading_zero
    }
    if(cmd[0]==71)
    {
      // show decimal value with leading zeros
      seg.begin(cmd[1], cmd[1]+1);  // clock, dio
      seg.showNumberDec((cmd[2] * 256) + cmd[3], true);  // number, leading_zero
    }
    if(cmd[0]==72)
    {
      // set brightness
      seg.begin(cmd[1], cmd[1]+1);  // clock, dio
      seg.setBrightness(cmd[2]);  // brightness
    }
    if(cmd[0]==73)
    {
      // set individual digit
      seg.begin(cmd[1], cmd[1]+1);  // clock, dio
      uint8_t data[] = {};
      data[0] = seg.encodeDigit(cmd[3]);  // number
      seg.setSegments(data, 1, cmd[2]);  // segments[], length, position
    }
    if(cmd[0]==74)
    {
      // set individual segment
      // 0xFF = 0b11111111 = Colon,G,F,E,D,C,B,A
      // Colon only works on 2nd segment (index 1)
      //     -A-
      //  F |   | B
      //     -G-
      //  E |   | C
      //     -D-
      seg.begin(cmd[1], cmd[1]+1);  // clock, dio
      uint8_t data[] = {};
      data[0] = cmd[3];  // byte
      seg.setSegments(data, 1, cmd[2]);  // segments[], length, position
    }
    if(cmd[0]==75)
    {
      // set left and right with colon separator
      seg.begin(cmd[1], cmd[1]+1);  // clock, dio
      uint8_t data[] = {};
      // first two segments
      data[0] = seg.encodeDigit(cmd[2]/10);  // 1st digit
      data[1] = seg.encodeDigit(cmd[2]%10);  // 2nd digit
      // colon
      data[1] |= 0x80;
      // next two segments
      data[2] = seg.encodeDigit(cmd[3]/10);  // 3rd digit
      data[3] = seg.encodeDigit(cmd[3]%10);  // 4th digit
      // send
      seg.setSegments(data, 4, 0);  // segments[], length, position
    }
    if(cmd[0]==76)
    {
      // analog read
      seg.begin(cmd[1], cmd[1]+1);  // clock, dio
      
      int pin = cmd[2];
      int reads = 4 * cmd[3];  // 1000/250 * cmd[3]
      
      // reading analog pin 4x per second
      for(int i = 0; i < reads; i++) {
        seg.showNumberDec(analogRead(pin), false);
        delay(250);
      }
    }
    if(cmd[0]==77)
    {
      // display on
      seg.begin(cmd[1], cmd[1]+1);  // clock, dio
      uint8_t data[] = { 0xFF, 0xFF, 0xFF, 0xFF };
      seg.setSegments(data, 4, 0);  // segments[], length, position
    }
    if(cmd[0]==78)
    {
      // display off
      seg.begin(cmd[1], cmd[1]+1);  // clock, dio
      uint8_t data[] = { 0x00, 0x00, 0x00, 0x00 };
      seg.setSegments(data, 4, 0);  // segments[], length, position
    }
  }
}

void receiveData(int byteCount)
{
    while(Wire.available()) 
    {
      if(Wire.available()==4)
      { 
        flag=0;
        index=0;
      }
        cmd[index++] = Wire.read();
    }
}

// callback for sending data
void sendData()
{
  if(cmd[0]==1)
    Wire.write(val);
  if(cmd[0]==3 ||cmd[0]==7)
    Wire.write(b, 3);
  if(cmd[0]==20)
    Wire.write(b, 4);
  if(cmd[0]==30||cmd[0]==40)
    Wire.write(b, 9);
}

