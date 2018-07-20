// Robo India Tutorials
// Hardware: NodeMCU
// simple Code for reading information from openweathermap.org 

#include <ESP8266WiFi.h>
#include <LiquidCrystal_I2C.h>

#include <ArduinoJson.h>
#include <Wire.h>
#include <time.h>
int tim = 500; //the value of delay time
int timezone = -3 * 3600; //Uruguay
int dst = 0;

const char* ssid     = "Julian Rezk";                 // SSID of local network
const char* password = "julianv106733";                    // Password on network
String APIKEY = "95de98929cb395e1140765e5aa3d6495";   
String Lenguaje = "es";                              
String CityID = "3441575";                                 //Montevideo defecto


WiFiClient client;
char servername[]="api.openweathermap.org";              // remote server we will connect to
String result;

int  counter = 60;                                      

String weatherDescription ="";
String weatherLocation = "";
String Country;
float Temperature;
float Humidity;
float Pressure;

LiquidCrystal_I2C lcd(0x27, 16, 2);    // Address of your i2c LCD back pack should be updated.

void setup() {
  Serial.begin(9600);
  int cursorPosition=0;
  lcd.begin(16, 2);
  lcd.init();
  lcd.backlight();
  lcd.print("   Conectando");  
  Serial.println("Conectando");
  WiFi.begin(ssid, password);
  
             while (WiFi.status() != WL_CONNECTED) 
            {
            delay(500);
            lcd.setCursor(cursorPosition,2); 
            lcd.print(".");
            cursorPosition++;
            }
  lcd.clear();
  lcd.print("   Conectado!");
  Serial.println("Conectado");
  configTime(timezone, dst, "pool.ntp.org","time.nist.gov");
   while(!time(nullptr)){
     Serial.print("*");
     delay(1000);
  }
  delay(1000);
}

void loop() {
    if(counter == 60)                                 //Get new data every 10 minutes
    {
      counter = 0;
      displayGettingData();
      delay(1000);
      getWeatherData();
    }else
    {
      counter++;
      displayWeather(weatherLocation,weatherDescription);
      delay(5000);
      displayConditions(Temperature,Humidity,Pressure);
      delay(5000);
      obtenerHora();
      delay(1000);
      obtenerHora();
      delay(1000);
      obtenerHora();
      delay(1000);
      obtenerHora();
      delay(1000);
      obtenerHora();
      delay(1000);
    }
}

void getWeatherData()                                //client function to send/receive GET request data.
{
  if (client.connect(servername, 80))   
          {                                         //starts client connection, checks for connection
          client.println("GET /data/2.5/weather?id="+CityID+"&units=metric&APPID="+APIKEY+"&lang="+Lenguaje);
          client.println("Host: api.openweathermap.org");
          client.println("User-Agent: ArduinoWiFi/1.1");
          client.println("Connection: close");
          client.println();
          } 
  else {
         Serial.println("connection failed");        //error message if no client connect
          Serial.println();
       }

  while(client.connected() && !client.available()) 
  delay(1);                                          //waits for data
  while (client.connected() || client.available())    
       {                                             //connected or data available
         char c = client.read();                     //gets byte from ethernet buffer
         result = result+c;
       }

client.stop();                                      //stop client
result.replace(' ',' ');
result.replace(' ',' ');
Serial.println(result);
char jsonArray [result.length()+1];
result.toCharArray(jsonArray,sizeof(jsonArray));
jsonArray[result.length() + 1] = '\0';
StaticJsonBuffer<1024> json_buf;
JsonObject &root = json_buf.parseObject(jsonArray);

if (!root.success())
  {
    Serial.println("parseObject() failed");
  }

String location = root["name"];
String country = root["sys"]["country"];
float temperature = root["main"]["temp"];
float humidity = root["main"]["humidity"];
String weather = root["weather"]["main"];
String description = root["weather"]["description"];
float pressure = root["main"]["pressure"];
weatherDescription = description;
weatherLocation = location;
Country = country;
Temperature = temperature;
Humidity = humidity;
Pressure = pressure;

}

void displayWeather(String location,String description)
{
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(location);
  lcd.print(", ");
  lcd.print(Country);
  lcd.setCursor(0,1);
  lcd.print(description);

}

void displayConditions(float Temperature,float Humidity, float Pressure)
{
 lcd.clear();                            //Printing Temperature
 lcd.print("T:"); 
 lcd.print(Temperature,1);
 lcd.print((char)223);
 lcd.print("C "); 
                                         
 lcd.print(" H:");                       //Printing Humidity
 lcd.print(Humidity,0);
 lcd.print(" %"); 
 
 lcd.setCursor(0,1);                     //Printing Pressure
 lcd.print("P: ");
 lcd.print(Pressure,1);
 lcd.print(" hPa");
}

void displayGettingData()
{
  lcd.clear();
  lcd.print("Obteniendo datos");
}

void obtenerHora(){
  time_t now = time(nullptr);
  struct tm* p_tm = localtime(&now);
  int dia = p_tm->tm_mday;
  int mes = p_tm->tm_mon + 1;
  int anio = p_tm->tm_year + 1900;
  
  int hora =(p_tm->tm_hour); 
  int minutos = (p_tm->tm_min);
  int segundos = (p_tm->tm_sec);
  
  lcd.setCursor(0,0);
  lcd.print("Fecha: ");
  lcd.print(dia);
  lcd.print("/");
  lcd.print(mes);
  lcd.print("/");
  lcd.print(anio);

  lcd.setCursor(0,1);
 
  lcd.print("Hora: ");
  if(hora<=9){
    lcd.print(0);
  }
  
  lcd.print(hora);
    lcd.print(":");
  if(minutos<=9){
    lcd.print(0);
  }
  lcd.print(minutos);
  lcd.print(":");
  if(segundos<=9){
    lcd.print(0);
  }
  lcd.print(segundos); 
   
    if(segundos==59){
    lcd.clear();
  }
}

void cambiarCiudad(String ciudad, bool action){
  if(ciudad == "setFrayBentos"){
      CityID =  "3442568";
    }
  
  if(ciudad == "setMontevideo"){
      CityID= "3441575";
    }

  if(ciudad == "setTreintaYTres"){
      CityID= "3439781";
    }
}


//METODO PARA PRENDER O APAGAR EL LED
/*
 *  Este callback se llama cuando se utilizan widgets de control que envian mensajes por el topico requestTopic
 *  Notar que en la función de reconnect se realiza la suscripción al topico de request
 *  
 *  El formato del string que llega es "v1/devices/me/rpc/request/{id}/Operación. donde operación es el método get declarado en el  
 *  widget que usan para mandar el comando e {id} es el indetificador del mensaje generado por el servidor
 */
void on_message(const char* topic, byte* payload, unsigned int length) 
{
  // Mostrar datos recibidos del servidor
  Serial.println("On message");

  char json[length + 1];
  strncpy (json, (char*)payload, length);
  json[length] = '\0';

  Serial.print("Topic: ");
  Serial.println(topic);
  Serial.print("Message: ");
  Serial.println(json);

  // Decode JSON request
  // Notar que a modo de ejemplo este mensaje se arma utilizando la librería ArduinoJson en lugar de desarmar el string a "mano"
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& data = jsonBuffer.parseObject((char*)json);

  if (!data.success())
  {
    Serial.println("parseObject() failed");
    return;
  }

  // Obtener el nombre del método invocado, esto lo envia el switch de la puerta y el knob del motor que están en el dashboard
  String methodName = String((const char*)data["method"]);
  Serial.print("Nombre metodo:");
  Serial.println(methodName);

  /* Responder segun el método 
   *  En este caso 
   *     el widget switch envia el comando openDoor con un paramtro true o false
   *     el widget gauge envian un comando rotateMotorValue con un int de los grados a rotar el motos (servo)
   *     Luego se responde con un mensaje para que actualice la Card correspondiente a la Door (doorState)
   */
   
  if (methodName.equals("setFrayBentos")) {
    bool action = data["params"];
    cambiarCiudad("setFrayBentos",action);

  }

  if (methodName.equals("setMontevideo")) {
    bool action = data["params"];
    cambiarCiudad("setMontevideo",action);

  }

  if (methodName.equals("setTreintaYTres")) {
    bool action = data["params"];
    cambiarCiudad("setTreintaYTres",action);
  }
}


