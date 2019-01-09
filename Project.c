#include <SPI.h>
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_TSL2561_U.h>
#include <stdlib.h>
#include <string.h>

Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);

int sensor_reading = 6;
int sensor_led = 5;

int sampling_time = 280;
int delta_time = 40;
int sleep_time = 9680;
char temp1[50];
char temp2[50];
char temp3[50];
float raw_value = 0;
float real_voltage = 0;
int pd_factor = 5;
float dust_density = 0;
float power_consumption_sensor = 0;

int uv_reading = 2;
int raw_uv_value = 0;
float input_voltage = 0;
float uv_index = 0;

//network login credentials
char ssid[] = "HORIA 8600";
char password[] = "266U4{i6";


//thingspeak
char thingSpeakAddress[] = "api.thingspeak.com";
char apiKey[] = "E92BUGP2TX0A3ZAU";

const unsigned long postingInterval = 15L * 1000L; // delay between updates, in milliseconds

#define POSTSTRING "field1=%s&field2=%5.2f&field3=%5.2f"


//Initialise dataString char Array
//field1=dc85dec8f715&field2=00.00&field3=00.00
//char * dataString = (char *)malloc(sizeof(char)*1000);
char dataString[] = "field1=5c313e0555cf&field2=00.00&field3=00.00";
char alternate[]= "field1=5c313e0555cf&field2=00.00&field3=00.00";
char deviceId[] = "xxxxxxxxxxxx";

MACAddress mac;
WiFiClient client; 

void configureSensor(void)
{
  // manually set gain
  // tsl.setGain(TSL2561_GAIN_16X);     // higher gain used in low light
  tsl.enableAutoRange(true);            // switches automatically between 1x and 16x
  
  //integration time alters the resolution of the sensor
  tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_13MS); 

  /* Update these values depending on what you've set above! */  
  Serial.println("------------------------------------");
  Serial.print  ("Gain:         "); Serial.println("Auto");
  Serial.print  ("Timing:       "); Serial.println("13 ms");
  Serial.println("------------------------------------");
}

void setup()
{
  
  //Serial.println(dataString);
  uint8_t macOctets[6]; 
  Serial.begin(19200);          //Initialise serial port for local monitoring on the Serial Monitor via USB
  Serial.print("Attempting to connect to Network named: ");
  Serial.println(ssid);
  
  // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
  WiFi.begin(ssid, password);
  while ( WiFi.status() != WL_CONNECTED) {
    Serial.print(".");         // print dots while we wait to connect
    delay(300);
  }
  
  Serial.println("\nYou're connected to the network");
  Serial.println("Waiting for an ip address");
  
  while (WiFi.localIP() == INADDR_NONE) {
    Serial.print(".");        // print dots while we wait for an ip addresss
    delay(300);
  }

  // We are connected and have an IP address.
  Serial.print("\nIP Address obtained: ");
  Serial.println(WiFi.localIP());

  mac = WiFi.macAddress(macOctets);
  Serial.print("MAC Address: ");
  Serial.println(mac);
  
  // Use MAC Address as deviceId
  sprintf(deviceId, "%02x%02x%02x%02x%02x%02x", macOctets[0], macOctets[1], macOctets[2], macOctets[3], macOctets[4], macOctets[5]);
  //sprintf(dataString,"field1=%s&field2=00.00&field3=00.00",deviceId,00.00,00.00);
  //dataString ="field1="+deviceId+"&field2=00.00&field3=00.00";
  Serial.print("deviceId: ");
  Serial.println(deviceId);
  
  pinMode(sensor_led,OUTPUT);
  //opening the serial monitor and declaring led as OUT
  pinMode(uv_reading,INPUT);
  //pin used to take uv measurement
  configureSensor();
  //sets up the lux sensor
}

void loop()
{ //dataString="field1=5c313e0555cf&field2= 00.00&field3= 00.00";
  //Serial.println(dataString);
 // sprintf(dataString,POSTSTRING,"5c313e0555cf",dust_density,uv_index);
  //Serial.println(dataString);
 
  //dataString="field1=5c313e0555cf&field2=00.00&field3=00.00";
  digitalWrite(sensor_led,LOW);
  delayMicroseconds(sampling_time);
  //turning on the LED and waiting for the pulse to reach its maximum
  
  raw_value = analogRead(sensor_reading);
  //take reading
  
  delayMicroseconds(delta_time);
  digitalWrite(sensor_led,HIGH);
  delayMicroseconds(sleep_time);
  //sleep for 9.68 ms so that 10 ms cycle is complete
  
  real_voltage = raw_value * pd_factor * (1.43 / 4095);
  //finding the real value depending on potential divider and raw value
  dust_density = 0.17 * real_voltage - 0.1;  
  //calculating the dust density depending on output characteristic
  power_consumption_sensor = power_consumption_sensor + (0.02 * real_voltage * 0.32);
  //power consumed in one pulse (max current * voltage * pulse duration)

  raw_uv_value = analogRead(uv_reading);
  input_voltage = raw_uv_value * (1.4 / 4095);
  //works out the actual voltage given by the sensor
  uv_index = input_voltage / 0.1;
  //finds the uv index according to the formula found on adafruit website

  Serial.print(" -Voltage (V): ");
  Serial.print(real_voltage);
  
  Serial.print(" -Dust Density (mg/m^3): ");
  Serial.println(dust_density);

  Serial.print(" -Dust Sensor Power Consumption (mJ): ");
  Serial.println(power_consumption_sensor);

  Serial.print(" -Voltage (V): ");
  Serial.print(input_voltage);
  Serial.println();
  
  Serial.print(" -UV Index: ");
  Serial.print(uv_index);
  Serial.println();
  
  //new sensor event
  sensors_event_t event;
  tsl.getEvent(&event);
 
  // display the sensor measurement
  if (event.light)
  {
    Serial.print(event.light); 
    Serial.println(" lux");
  }
  else
  {
    //sensor is overly saturated (lux = 0)
    Serial.println("Sensor overload");
  }
  
  Serial.println("Preparing to post");
    if (client.connect(thingSpeakAddress,80)){
  
      Serial.println("Connected to server");
      sprintf(dataString,POSTSTRING,deviceId,dust_density,uv_index);
     // strcpy(dataString,""
      snprintf(temp1,50,"%.2f",dust_density);
      snprintf(temp2,50,"%.2f",uv_index);
      Serial.println(temp1);
      Serial.println(temp2);
      //field1=5c313e0555cf&field2= 00.00&field3= 00.00
      strcpy(alternate,"field1=");
      strcat(alternate,deviceId);
      strcat(alternate,"&field2=");
      strcat(alternate,temp1);
      strcat(alternate,"&field3=");
      strcat(alternate,temp2);
      Serial.println("*****");
      Serial.println(dataString);
      Serial.println(alternate);
      Serial.println("*****");
      
      //Other sensor/data sources
      // accSensor.readXData(), accSensor.readYData(),accSensor.readZData(),-WiFi.RSSI()
      Serial.print("Posting: ");
      Serial.println(dataString);
      client.print("POST /update HTTP/1.1\n");
      client.print("Host: api.thingspeak.com\n");
      client.print("Connection: close\n");
      client.print("X-THINGSPEAKAPIKEY: ");
      client.print(apiKey);
      client.print("\n");
      client.print("Content-Type: application/x-www-form-urlencoded\n");
      client.print("Content-Length: ");
      client.print(sizeof(dataString)-1);
      client.print("\n\n");
      client.print(dataString);
      client.stop();
    }
    else {
      // if you couldn't make a connection:
      Serial.println("connection failed");
    }
    
 delay(postingInterval);
}
