#include <Wire.h>
#include "MMA7660.h"
#include "DS1307.h"
#include "DHT.h"
#include "ChainableLED.h"
MMA7660 acc;
DS1307 clock;
DHT dht;
ChainableLED leds;

#define SLAVE_ADDRESS 0x04
int number = 0;
int state = 0;

// which led are we setting in the next cmd
int led=0;

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

    // Grove Chainable RGB Led
    // http://www.seeedstudio.com/wiki/Grove_-_Chainable_RGB_LED
    // ci,di,vcc,gnd
    // co,do,vcc,gnd

    // this one works
    if(cmd[0]=60)
    {
      // ChainableLED leds(7, 8, NUM_LEDS);
      leds.begin(cmd[1], cmd[1]+1, cmd[2]);  // clock, data, num leds
      
      // light up chain of 6x rgb leds
      leds.setColorRGB(0, 255, 0, 0);
      leds.setColorRGB(1, 0, 255, 0);
      leds.setColorRGB(2, 0, 0, 255);
      leds.setColorRGB(3, 255, 255, 0);
      leds.setColorRGB(4, 255, 0, 255);
      leds.setColorRGB(5, 0, 255, 255);
    }

    // 7 bytes are required to light a specific rgb led in the chain:
    // cmd, clock, num-leds, which-led, red, green, blue

    // failed attempt to use 2x commands
    // 1) [60,data,count,led]
    // 2) [61,red,green,blue]

    // does not work
    // first command establishes connection
    //[61,data,num,which]
    if(cmd[0]=61)
    {
      // ChainableLED leds(7, 8, NUM_LEDS);
      leds.begin(cmd[1], cmd[1]+1, cmd[2]);  // clock, data, num leds

      // next command will write a colour to this specific led
      led = cmd[3];
    }
    // does not work
    // second command sets the colour on the previously defined led
    //[62,red,green,blue]
    if(cmd[0]=62)
    {
      leds.setColorRGB(led, cmd[1], cmd[2], cmd[3]);
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

