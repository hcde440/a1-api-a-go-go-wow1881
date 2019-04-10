/*
 * Do project description here
 *
 * Alex Banh, HCDE 440 Spring 2019
*/
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h> //provides the ability to parse and construct JSON objects
#include <Arduino.h>

const char* ssid = "Tell my Wi-fi love her";
const char* pass = "thirstybanana810";
const char* key = "1f05870a9580401660bcc2a80bbd7b65"; //API key for ipstack API call
String geocodeKey = "de96bc34489f464eb898d6414dcd50ab"; //API key for geocode call
String ipaddress;

typedef struct { //here we create a new data type definition, a box to hold other data types
  String ln;
  String lt;
} GeoData;     //then we give our new data structure a name so we can use it in our code

typedef struct { //A new data type to hold information from our International Space Station API
  String ln; //Current longitude of the ISS
  String lt; //Current latitude of the ISS
  int numberPpl; //Current number of people on the ISS
  String people[6]; //String array holding the names of people currently on the ISS
} ISSData;

GeoData location; //Instantiating our data types
ISSData ISSinfo;

void setup() {
  Serial.begin(115200);

  // Prints file name and compile time
  Serial.print("This board is running: ");
  Serial.println(F(__FILE__));
  Serial.print("Compiled: ");
  Serial.println(F(__DATE__ " " __TIME__));

  delay(10);
  Serial.print("Connecting to "); 
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(); Serial.println("WiFi connected"); Serial.println();
  Serial.print("Your ESP has been assigned the internal IP address ");
  Serial.println(WiFi.localIP());
  ipaddress = getIP();
  getGeo();
  Serial.println("Your external IP address is at (roughly) ");
  Serial.println(location.lt + " latitude by " + location.ln + " longitude.");
  Serial.println("\nLet's get into some space facts! We'll take a look at the International Space Station.");

  //Method call to get information on the current position of the ISS
  getISS();

  //Method call that returns the translated location of the ISS (From lat long to human readable country)
  String country = translateISS();

  //If the country is empty (meaning that data could not be found), assume that the ISS is over the ocean
  if (country == "") {
    country = "the Ocean";
  }
  Serial.println("The International Space Station is currently over " + country);
  Serial.println("This corresponds to a latitude of " + ISSinfo.lt + " and a longitude of " + ISSinfo.ln);

  //Method call to get information on the people currently in space
  getPeople();
  
  Serial.println("There are currently " + String(ISSinfo.numberPpl) + " people in space. Their names are:");

  //for loop that iterates through the people names array, printing each name.
  for (int i = 0; i < ISSinfo.numberPpl; i++ ) {
    Serial.println(ISSinfo.people[i]);
  }
  
} 

void loop() {
  // put your main code here, to run repeatedly:
}

String getIP() {
  HTTPClient theClient;
  String ipAddress;
  theClient.begin("http://api.ipify.org/?format=json");
  int httpCode = theClient.GET();
  if (httpCode > 0) {
    if (httpCode == 200) {
      DynamicJsonBuffer jsonBuffer;
      String payload = theClient.getString();
      JsonObject& root = jsonBuffer.parse(payload);
      ipAddress = root["ip"].as<String>();
    } else {
      Serial.println("Something went wrong with connecting to the endpoint.");
      return "error";
    }
  }
  return ipAddress;
}

void getGeo() {
  HTTPClient theClient;
  Serial.println("Making HTTP request");
  theClient.begin("http://api.ipstack.com/" + ipaddress + "?access_key=" + key); //return IP as .json object
  int httpCode = theClient.GET();
  if (httpCode > 0) {
    if (httpCode == 200) {
      Serial.println("Received HTTP payload.");
      DynamicJsonBuffer jsonBuffer;
      String payload = theClient.getString();
      Serial.println("Parsing...");
      JsonObject& root = jsonBuffer.parse(payload);
      // Test if parsing succeeds.
      if (!root.success()) {
        Serial.println("parseObject() failed");
        Serial.println(payload);
        return;
      }
      //Using .dot syntax, we refer to the variable "location" which is of
      //type GeoData, and place our data into the data structure.
      location.lt = root["latitude"].as<String>();
      location.ln = root["longitude"].as<String>();
    } else {
      Serial.println("Something went wrong with connecting to the endpoint.");
    }
  }
}

void getISS() {
  HTTPClient theClient;
  theClient.begin("http://api.open-notify.org/iss-now.json"); //return current ISS location data as a JSON object
  int httpCode = theClient.GET();
  if (httpCode > 0) {
    if (httpCode == 200) {
      DynamicJsonBuffer jsonBuffer;
      String payload = theClient.getString();
      JsonObject& root = jsonBuffer.parse(payload);
      // Test if parsing succeeds.
      if (!root.success()) {
        Serial.println("parseObject() failed");
        Serial.println(payload);
        return;
      }
      //Populate some of the values inside our ISSData object. We refer to the object by it's instantiated name (ISSinfo)
      ISSinfo.lt = root["iss_position"]["latitude"].as<String>();
      ISSinfo.ln = root["iss_position"]["longitude"].as<String>();
    } else {
      Serial.println("Something went wrong with connecting to the endpoint.");
    }
  }
}

String translateISS() {
  HTTPClient theClient;
  //With the given ISS latitude and longitude, make an API call to geocode the lat long data into a country name. 
  theClient.begin("http://api.opencagedata.com/geocode/v1/json?q=" + ISSinfo.lt + "%2C%20" + ISSinfo.ln + "&key=" + geocodeKey + "&pretty=1&no_annotations=1");
  int httpCode = theClient.GET();
  if (httpCode > 0) {
    if (httpCode == 200) {
      DynamicJsonBuffer jsonBuffer;
      String payload = theClient.getString();
      JsonObject& root = jsonBuffer.parse(payload);
      // Test if parsing succeeds.
      if (!root.success()) {
        Serial.println("parseObject() failed");
        Serial.println(payload);
        return "PAYLOAD FAILED";
      }
      //Returns the country name as a string
      return root["results"][0]["components"]["country"].as<String>();
  
    } else {
      Serial.println("Something went wrong with connecting to the endpoint.");
      return "PAYLOAD FAILED";
    }
  }
  return "PAYLOAD FAILED";

}


void getPeople() {
  HTTPClient theClient;
  theClient.begin("http://api.open-notify.org/astros.json"); //return current ISS people data as a JSON object
  int httpCode = theClient.GET();
  if (httpCode > 0) {
    if (httpCode == 200) {
      DynamicJsonBuffer jsonBuffer;
      String payload = theClient.getString();
      JsonObject& root = jsonBuffer.parse(payload);
      // Test if parsing succeeds.
      if (!root.success()) {
        Serial.println("parseObject() failed");
        Serial.println(payload);
        return;
      }
      //Populate some of the values inside our ISSData object. We refer to the object by it's instantiated name (ISSinfo)
      ISSinfo.numberPpl = root["number"];

      //For the number of people currently in space, iterate over the JSON object to retrieve their names
      for (int i = 0; i < ISSinfo.numberPpl; i++) {
        ISSinfo.people[i] = root["people"][i]["name"].as<String>();
      }
    } else {
      Serial.println("Something went wrong with connecting to the endpoint.");
    }
  }
}
