#include <WiFiS3.h> //Uses the built in Wifi 
#include "ThingSpeak.h" //Sends data to ThingSpeak
#include <DHT.h> // DHT sensor 

const char* ssid = "VM3867899"; //Wi-fi name
const char* password = "wxtj7XnFgcnj"; //Wi-fi password

unsigned long myChannelNumber = 3112033;        // ThingSpeak channel number
const char* myWriteAPIKey = "VFHX4G5N50PUHZSS"; // Write API key (used to upload data)

WiFiClient client; //Creates a Wi-fi client object to communicate with ThingSpeak

//DHT22 Set-up 
#define DHTPIN 2
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

//Soil Moisture Sensor Calibration Values
int DRY_CAL = 850;   //Reading when soil is dry (adjust later)
int WET_CAL = 280;   //Wet soil reading              

const int soilPin = A0;

// Light Sensor setup 
const int lightAnalogPin = A1;  // A0 -> A1
const int lightDigitalPin = 3;  // D0 -> D3 

// Relay / LEDs ('grow' light)  
const int GROW_LED_PIN = 7;   
const int ledPin = LED_BUILTIN; //LED

// Light control tuning
int darkOn  = 650;   // Below this = turn ON 'grow' light
int lightOff = 750;  // Above = turn OFF 'grow' light 
bool growOn = false;   

void setup() {
  Serial.begin(115200); 
  pinMode(LED_BUILTIN, OUTPUT); //Built-in LED used as a status indictator 
  pinMode(GROW_LED_PIN, OUTPUT);
  digitalWrite(GROW_LED_PIN, LOW);  // grow LED OFF at start

  // Wi-Fi connection
  WiFi.begin(ssid, password); //Connects to network
  Serial.print("Wi-Fi: ");

  unsigned long t0 = millis(); //Stores current time 
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 20000) {
    delay(500); Serial.print("."); //Waits half a second before checking again
  }
  //Prints if Wi-fi connection is a Success or Failure 
  Serial.println(WiFi.status()==WL_CONNECTED ? "\nConnected" : "\nFAILED");

// Initialises ThingSpeak 
  ThingSpeak.begin(client); //Start ThingSpeak communication
  dht.begin();   //Start DHT22 communication  
} 

void loop() {
  //Reads Soil Moisture Sensor 
  int soilValue = analogRead(soilPin); //Gets raw analog value 
  Serial.print("Soil raw: "); //Prints label
  Serial.println(soilValue); //Prints Raw Sensor value (The higher the number, the drier the soil)

  //Calculates Soil Moisture percentage 
  int moisturePercent = map(soilValue, DRY_CAL, WET_CAL, 0, 100);  // Converts the sensor reading to % (0 = dry, 100 = wet)
  moisturePercent = constrain(moisturePercent, 0, 100);            //Keeps value between 0-100
  
  Serial.println("moisturePercent"); //Prints second label 
  Serial.println("Moisture %");

  //DHT22 (Humidity % and Temp °C)
  float humidity = dht.readHumidity();        // %
  float temperature = dht.readTemperature();  // °C

   if (isnan(humidity) || isnan(temperature)) {
    Serial.println("DHT read failed this cycle.");
  } else {
    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.print(" %  | Temp: ");
    Serial.print(temperature);
    Serial.println(" °C");
  }

// Light
  int lightRaw = analogRead(lightAnalogPin);             
  int lightPercent = map(lightRaw, 0, 1023, 0, 100);     // Simple % scale
  lightPercent = constrain(lightPercent, 0, 100);

  Serial.print("Light raw: ");
  Serial.print(lightRaw);
  Serial.print(" | Light %: ");
  Serial.println(lightPercent);

// Grow LED control using LDR (simple)
  if (lightRaw < darkOn) {
    digitalWrite(GROW_LED_PIN, HIGH);  // ON when dark
    } else if (lightRaw > lightOff) {
    digitalWrite(GROW_LED_PIN, LOW);   // OFF when bright
    }

  //Turns LED on whilst uploading to ThingSpeak
  digitalWrite(LED_BUILTIN, HIGH);

  //Field Mapping 
    if (!isnan(temperature)) ThingSpeak.setField(1, temperature); // Field 1: Temperature (°C)
  if (!isnan(humidity))    ThingSpeak.setField(2, humidity); // Field 2:Humidity (%)
  ThingSpeak.setField(3, soilValue);        // Field 3:Raw value of Soil Moisture
  ThingSpeak.setField(4, moisturePercent);   // Field 4: % moisture
   ThingSpeak.setField(5, lightRaw);    //Field 5:Light(Raw)
   ThingSpeak.setField(6, lightPercent); //Field 6: Light (%)     
   

  int code = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);

  digitalWrite(LED_BUILTIN, LOW); //Turns LED after upload

//Checks ThingSpeak upload status
  if (code == 200) Serial.println("ThingSpeak OK");
  else { Serial.print("ThingSpeak error: "); Serial.println(code); }

  delay(20000); //Waits at leat 15 seconds before next upload
}