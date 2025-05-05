#include "MAX30100_PulseOximeter.h"
#define REPORTING_PERIOD_MS     1000
PulseOximeter pox;
uint32_t tsLastReport = 0;
#include <SoftwareSerial.h>
SoftwareSerial mySerial(2, 3); // RX, TX
SoftwareSerial GPSSerial(7, 8); // RX, TX    // GPS
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);
#include <DHT.h>

#define DHTPIN A0      // DHT11 sensor pin
#define DHTTYPE DHT11  // DHT sensor type
DHT dht(DHTPIN, DHTTYPE);

char phone_number[20] = "9953986150"; // Change your no. here
//

float t;
float h;

char rec = ' ';
char arr_gps[200] = "";
uint8_t valid_data_gps_flag = 0;
uint8_t start_gps_reading = 0;
uint8_t gps_count = 0;
uint8_t arr_count_gps = 0;
#define TRUE   1
#define FALSE   0
char time[30];
char gps_valid;
char latitude[20];
char lat_ns;
char longitude[20];
char lon_ew;
float lat1, lon1, tem1, tem2, tem3;
unsigned long panicMillis = 0, outofrangeMillis = 0, connectionMillis = 0;
int read_flag = 0, connection_flag = 1;//
float HB , SPO2, temp;

void send_message(const char* number, const char* message) {
  mySerial.print("AT+CMGS=\"");
  mySerial.print(number);
  delay(1000);
  mySerial.println("\"");
  delay(1000);
  mySerial.print(message);
  delay(1000);
  mySerial.write(26); // Ctrl+Z to send the SMS
}

void initGSM()
{
  lcd.print(F("SMS "));
  mySerial.begin(38400);
  mySerial.println(F("AT"));
  delay(1000);
  mySerial.println("AT+CMGF=1");
  delay(1000);
  mySerial.println("AT+IPR=9600");
  delay(1000);
  mySerial.begin(9600);
  mySerial.println(F("AT"));
  delay(1000);
  mySerial.println("AT+CMGF=1");
  delay(1000);
  send_message(phone_number, "System Started");
  lcd.print(F("sent..OK"));
  delay(1000);
  lcd.clear();
}

void send_msg_location(const char* number, const char* message)
{
  mySerial.print(F("AT+CMGS=\""));
  mySerial.print(number);
  mySerial.println(F("\""));
  delay(1000);
  mySerial.print(message);
  mySerial.print(F("Temp :")); mySerial.print(t); mySerial.println(F("*C"));
  mySerial.print(F("Humidity :")); mySerial.print(t); mySerial.println(F("*C"));
  mySerial.print(F("HB :")); mySerial.println(HB);
  mySerial.print(F("http://maps.google.com/maps?q=+"));
  mySerial.print(lat1, 5); mySerial.print(",+"); mySerial.print(lon1, 5);
  mySerial.write(26); // Ctrl+Z to send the SMS
}
void onBeatDetected()
{
  Serial.println("Beat!");
}

//*************************************************
void process_gps_data(char *arr)
{
  uint8_t i;
  //Get time
  i = 0;
  while (*arr != ',')
  {
    time[i++] = *arr;
    arr++;
  }
  time[i] = '\0';

  //Get GPS_valid
  arr++;
  gps_valid = *arr;
  arr++;

  //Get latitude
  arr++;
  i = 0;
  while (*arr != ',')
  {
    latitude[i++] = *arr;
    arr++;
  }
  latitude[i] = '\0';

  //Get lat_ns
  arr++;
  if (*arr == ',')
  {
    lat_ns = '\0';
  }
  else
  { lat_ns = *arr;
    arr++;
  }

  //Get longitude
  arr++;
  i = 0;
  while (*arr != ',')
  {
    longitude[i++] = *arr;
    arr++;
  }
  longitude[i] = '\0';

  //Get lon_ew
  arr++;
  if (*arr == ',')
  {
    lon_ew = '\0';
  }
  else
  { lon_ew = *arr;
    arr++;
  }

  //Display GPS data on uart, for debugging only
//    Serial.println();
//    Serial.print("Time:");Serial.print(time);Serial.print("\r\n");
//    Serial.print("Data Valid:");Serial.print(gps_valid);Serial.print("\r\n");
//    Serial.print("Latitude:");Serial.print(latitude);Serial.print("\r\n");
//    Serial.print("Latitude N/S:");Serial.print(lat_ns);Serial.print("\r\n");
//    Serial.print("Longitude:");Serial.print(longitude);Serial.print("\r\n");
//    Serial.print("Longitude E/W:");Serial.print(lon_ew);Serial.print("\r\n");
}
//*****************
void read_gps2()
{
  if (GPSSerial.available() > 0)
  {
    rec = GPSSerial.read();
    //Serial.print(rec);
    if (gps_count == 7)
    {
      Serial.println("gps_count == 7");
      if (rec == 0x0D)
      {
        Serial.println("rec == 0x0D");
        gps_count = 0;
        arr_gps[arr_count_gps] = '\0';
        valid_data_gps_flag = TRUE;       //Valid GPS data is received in the array
        Serial.println();
        process_gps_data(arr_gps);
        start_gps_reading = 0;
      }
      else
      {
        arr_gps[arr_count_gps++] = rec;
        //Serial.print(rec);
        //uart_tx(rec);           //Debugging on uart
      }
    }
    //*****
    if (gps_count == 6)
    {
      Serial.println("rec == 6");
      if (rec == ',')
      {
        gps_count = 7;
        //uart1_puts("\r\n");         //Debugging on uart
      }
      else
      {
        gps_count = 0;
      }
    }
    //*****
    if (gps_count == 5)
    {
      Serial.println("rec == 5");
      if (rec == 'C')
      {
        gps_count = 6;
      }
      else
      {
        gps_count = 0;
      }
    }
    //*****
    if (gps_count == 4)
    {
      Serial.println("rec == 4");
      if (rec == 'M')
      {
        gps_count = 5;
      }
      else
      {
        gps_count = 0;
      }
    }
    //*****
    if (gps_count == 3)
    {
      Serial.println("rec == 3");
      if (rec == 'R')
      {
        gps_count = 4;
      }
      else
      {
        gps_count = 0;
      }
    }
    //*****
    if (gps_count == 2)
    {
      Serial.println("rec == 2");
      if (rec == 'P')
      {
        gps_count = 3;
      }
      else
      {
        gps_count = 0;
      }
    }
    //*****
    if (gps_count == 1)
    {
      Serial.println("rec == 1");
      if (rec == 'G')
      {
        gps_count = 2;
      }
      else
      {
        gps_count = 0;
      }
    }
    //*****
    if (rec == '$')
    {
      Serial.println("rec == $");
      if (start_gps_reading == 1)
      {
        gps_count = 1;
        arr_count_gps = 0;    // GPS data array count reset to 0
      }
    }
    //*****
    /////
  }
}
//*****************
void read_gps_location()
{
  GPSSerial.listen();
  start_gps_reading = 1;
  valid_data_gps_flag = FALSE;
  Serial.println("Read_GPS");
  while (valid_data_gps_flag != TRUE)
  {
    read_gps2();
    //Serial.println("Read_GPS");
  }
  Serial.println("Read_GPS OK");
  lat1 = atof(latitude);
  lon1 = atof(longitude);
  tem1 = (int)(lat1 / 100);
  tem2 = (lat1 - (tem1 * 100));
  tem3 = (tem2 * 10000) / 60;
  lat1 = tem1 + (tem3 / 10000);
  Serial.print("Lat1:");Serial.println(lat1,5);
  tem1 = (int)(lon1 / 100);
  tem2 = (lon1 - (tem1 * 100));
  tem3 = (tem2 * 10000) / 60;
  lon1 = tem1 + (tem3 / 10000);
  Serial.print("Lon1:");Serial.println(lon1,5);
  delay(1000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("Lat: ")); lcd.print(lat1, 5); lcd.print(F("   "));
  lcd.setCursor(0, 1);
  lcd.print(F("Lon: ")); lcd.print(lon1, 5); lcd.print(F("   "));
  delay(2000);
  lcd.clear();
}

void initPulseSensor()
{
  if (!pox.begin()) {
    Serial.println("FAILED");
    for (;;);
  } else {
    Serial.println("SUCCESS");
  }
  pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);
  // Register a callback for the beat detection
  pox.setOnBeatDetectedCallback(onBeatDetected);
}

void setup()
{
  Serial.begin(9600);
  pinMode(13,OUTPUT);
  pinMode(13,LOW);
  dht.begin();
  GPSSerial.begin(9600);
  lcd.init();                      // initialize the lcd
  lcd.backlight();
  lcd.print(F(" Smart Soldier  "));
  lcd.setCursor(0, 1);
  lcd.print(F("Health Monitorng"));
  delay(2000);  lcd.clear();
  Serial.print("Initializing pulse oximeter..");
  initPulseSensor();
  initGSM();
  initPulseSensor();
}

void loop()
{
  // Make sure to call update as fast as possible
  pox.update();
  if (millis() - tsLastReport > REPORTING_PERIOD_MS) {
    Serial.print("<H"); Serial.print(pox.getHeartRate()); Serial.print(">");
    Serial.print("<S"); Serial.print(pox.getSpO2()); Serial.println(">");
    lcd.setCursor(0, 0);
    lcd.print(F("HB:")); lcd.print(pox.getHeartRate()); lcd.print(F(" "));
    lcd.print(F("SP:")); lcd.print(pox.getSpO2()); lcd.print(F(" "));
    t = dht.readTemperature();
    h = dht.readHumidity();
    lcd.setCursor(0, 1);
    lcd.print(F("T:")); lcd.print(t); lcd.print(F(" "));
    lcd.print(F("H:")); lcd.print(h); lcd.print(F("   "));

    tsLastReport = millis();
  }
  if(t >45 || h > 80)
    {
      if(t > 45)
      {
        pinMode(13,HIGH);
        read_gps_location();
        send_msg_location(phone_number,"Temp Alert : ");
        initPulseSensor();
      }
      else if(h > 80)
      {
        pinMode(13,HIGH);
        read_gps_location();
        send_msg_location(phone_number,"Humidity Alert : ");
        initPulseSensor();
      }
    }
    else
    {
      pinMode(13,LOW);
    }
}
