#include <FastLED.h>

#include <ArduinoJson.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <time.h>
#include <PubSubClient.h>


/** CONFIGURACION LAMPARA **/

//Estado de lampara
bool ESTADOLAMPARA = true;
//Modo lampara 
bool MODOLAMPARA = false;
//Intensidad en grados
float INTENSIDADLAMPARA = 50.0;

bool rojo= false;
bool verde= false;
bool azul= false;
bool amarillo= true;
bool violeta= false;
bool blanco= false;
bool naranja= false;
bool celeste= false;
bool rosado = false;


//La cantidad de LED en la configuración
#define NUM_LEDS 60
//The pin that controls the LEDs
#define LED_PIN 4
//El pin que controla los LED
#define ANALOG_READ A0

//Valor bajo del micrófono confirmado y valor máximo
#define MIC_LOW 550.0
#define MIC_HIGH 650.0
/** Other macros */
//¿Cuántos valores de sensor previos afectan el promedio operativo?
#define AVGLEN 5
//¿Cuántos valores de sensores anteriores se deciden si estamos en un pico / ALTO (por ejemplo, en una canción)?
#define LONG_SECTOR 15

//Mneumonics
#define HIGH 3
#define NORMAL 2

//¿Cuánto tiempo mantenemos el sonido de "promedio actual" antes de reiniciar la medición? 20*1000=20 segundos
#define MSECS 5 * 1000
#define CYCLES MSECS / DELAY

/*Sometimes readings are wrong or strange. How much is a reading allowed
to deviate from the average to not be discarded?
A veces las lecturas son incorrectas o extrañas. ¿Cuánto se permite una lectura?
¿Desviarse del promedio para no descartarlo?**/
#define DEV_THRESH 0.8

//Arduino loop delay
#define DELAY 1

float fscale( float originalMin, float originalMax, float newBegin, float newEnd, float inputValue, float curve);
void insert(int val, int *avgs, int len);
int compute_average(int *avgs, int len);
void visualize_music(bool estado, bool modoLampara, float grados, bool modoSirena);

//How many LEDs to we display
int curshow = NUM_LEDS;

/*Not really used yet. Thought to be able to switch between sound reactive
mode, and general gradient pulsing/static color
No realmente usado todavía. Se cree que es capaz de cambiar entre el sonido reactivo
modo, y gradiente general pulsando / color estático*/
int mode = 0;

//Mostrando diferentes colores según el modo.
int songmode = NORMAL;

//Average sound measurement the last CYCLES
//Medición de sonido promedio los últimos CICLOS
unsigned long song_avg;

//La cantidad de iteraciones desde que se reinició song_avg
int iter = 0;

//La velocidad con la que los LED se oscurecen si no se vuelven a encender
float fade_scale = 1.2;

//Led array
CRGB leds[NUM_LEDS];

/*Short sound avg used to "normalize" the input values.
We use the short average instead of using the sensor input directly 

Promedio corto de sonido utilizado para "normalizar" los valores de entrada.
Usamos el promedio corto en lugar de usar la entrada del sensor directamente*/
int avgs[AVGLEN] = {-1};

//Longer sound avg
int long_avg[LONG_SECTOR] = {-1};

//Keeping track how often, and how long times we hit a certain mode
//Hacer un seguimiento de la frecuencia y la cantidad de veces que alcanzamos un determinado modo
struct time_keeping {
  unsigned long times_start;
  short times;
};

//How much to increment or decrement each color every cycle
struct color {
  int r;
  int g;
  int b;
};

struct time_keeping high;
struct color Color; 
void fadeall() { for(int i = 0; i < NUM_LEDS; i++) { leds[i].nscale8(250); } }

/** FIN **/

/** CONFIGURACION ThingsBoard **/

const char* ssid = "XperiaZ3";
const char* password = "julian123";


int tim = 500; //the value of delay time
int timezone = -3 * 3600;
int dst = 0;

char thingsboardServer[] = "demo.thingsboard.io";

/*definir topicos.
 * telemetry - para enviar datos de los sensores
 * request - para recibir una solicitud y enviar datos 
 * attributes - para recibir comandos en baes a atributtos shared definidos en el dispositivo
 */
char telemetryTopic[] = "v1/devices/me/telemetry";
char requestTopic[] = "v1/devices/me/rpc/request/+";  //RPC - El Servidor usa este topico para enviar rquests, cliente response
char attributesTopic[] = "v1/devices/me/attributes";  // Permite recibir o enviar mensajes dependindo de atributos compartidos

// declarar cliente Wifi y PubSus
WiFiClient wifiClient;
PubSubClient client(wifiClient);

// declarar variables control loop (para no usar delay() en loop
unsigned long lastSend;
const int elapsedTime = 1000; // tiempo transcurrido entre envios al servidor

//  configuración datos thingsboard
#define NODE_NAME "NODEMCU LAMPARA"   //nombre que le pusieron al dispositivo cuando lo crearon
#define NODE_TOKEN "eWlcRRKOSGF8tk91NueQ"   //Token que genera Thingboard para dispositivo cuando lo crearon

/** FIN **/
void sonidoInicio(){
  tone(D8,880,100);
  delay(100);
  tone(D8,1046.50,100);
  delay(100);
  tone(D8,1174.66,100);
  delay(200);
}

void sonidoCambioALampara(){
  tone(D8,1174.66,100);
}

void sonidoApagado(){
  tone(D8,1174.66,100);
  delay(100);
  tone(D8,1046.50,100);
  delay(100);
  tone(D8,1174.66,100);
  delay(400);
}

void loopInicio() { 
  
  static uint8_t hue = 0;
  Serial.print("x");
  // First slide the led in one direction
  for(int i = 0; i < NUM_LEDS; i++) {
    // Set the i'th led to red.  HUE,SATURACION,BRILLO
    leds[i] = CHSV(hue++, 255, 255);
    // Show the leds
    FastLED.show(); 
    // now that we've shown the leds, reset the i'th led to black
    // leds[i] = CRGB::Black;
    fadeall();
    // Wait a little bit before we loop around and do it again
    delay(10);
  }
  Serial.print("x");
  sonidoInicio();
  // Now go in the other direction.  
  for(int i = (NUM_LEDS)-1; i >= 0; i--) {
    // Set the i'th led to red 
    leds[i] = CHSV(hue++, 255, 255);
    // Show the leds
    FastLED.show();
    // now that we've shown the leds, reset the i'th led to black
    // leds[i] = CRGB::Black;
    fadeall();
    // Wait a little bit before we loop around and do it again
    delay(10);
  }
}


int sensor_value;
void setup() {
  Serial.begin(9600);
  //Set all lights to make sure all are working as expected
  FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, NUM_LEDS);
  loopInicio();
  for (int i = 0; i < NUM_LEDS; i++) 
    leds[i] = CRGB(0, 0, 255);
 // FastLED.show(); 
  delay(1000);  

  //bootstrap average with some low values
  for (int i = 0; i < AVGLEN; i++) {  
    insert(250, avgs, AVGLEN);
  }

  //Initial values
  high.times = 0;
  high.times_start = millis();
  Color.r = 0;  
  Color.g = 0;
  Color.b = 3;



  /*SETUP CONNECTION */
  Serial.begin(9600);
  // inicializar wifi y pubsus
  connectToWiFi();
  client.setServer( thingsboardServer, 1883 );

  // agregado para recibir callbacks
  client.setCallback(on_message);
  
  lastSend = 0; // para controlar cada cuanto tiempo se envian datos
  configTime(timezone, dst, "pool.ntp.org","time.nist.gov");
  Serial.println("\nWaiting for Internet time");

  while(!time(nullptr)){
     Serial.print("*");
     delay(1000);
  }
  Serial.println("\nTime response....OK");  

}

/*With this we can change the mode if we want to implement a general 
lamp feature, with for instance general pulsing. Maybe if the
sound is low for a while? 

Con esto podemos cambiar el modo si queremos implementar un general
característica de la lámpara, con, por ejemplo, pulsación general. Tal vez si el
el sonido es bajo por un tiempo?*/
void loop() {
  switch(mode) {
    case 0:
      visualize_music(ESTADOLAMPARA, MODOLAMPARA, INTENSIDADLAMPARA);
      break;
    default:
      break;
  }
    delay(DELAY);       // delay in between reads for stability
    Serial.println(sensor_value);

  /*LOOP CONNECTION*/
     if ( !client.connected() ) {
    reconnect();
  }
  
  if ( millis() - lastSend > elapsedTime ) { // Update and send only after 1 seconds
    
    // FUNCION DE TELEMETRIA para enviar datos a thingsboard
    getAndSendData();   // FUNCION QUE ENVIA INFORMACIÓN DE TELEMETRIA
    
    lastSend = millis();
  }

  client.loop();
}


/**Función para verificar si la lámpara debe entrar en modo ALTO,
o revertir a NORMAL si ya está en ALTO. Si los sensores informan valores
que son más de 1,1 veces los valores promedio, y esto ha sucedido
más de 30 veces los últimos milisegundos, ingresará al modo ALTO.
POR HACER: No está muy bien escrito, elimina los valores codificados y lo hace más
reutilizable y configurable.*/
void check_high(int avg) {
  if (avg > (song_avg/iter * 1.0525))  {
    if (high.times != 0) {
      if (millis() - high.times_start > 100.0) {
        high.times = 0;
        songmode = NORMAL;
      } else {
        high.times_start = millis();  
        high.times++; 
      }
    } else {
      high.times++;
      high.times_start = millis();

    }
  }
  if (high.times > 30 && millis() - high.times_start < 50.0)
    songmode = HIGH;
  else if (millis() - high.times_start > 100.0) {
    high.times = 0;
    songmode = NORMAL;
  }
}

//Main function for visualizing the sounds in the lamp
void visualize_music(bool estado, bool modoLampara, float grados) {
  int mapped, avg, longavg;
  
  if(estado && !modoLampara){
    //Actual sensor value
    sensor_value = analogRead(ANALOG_READ);
    
    //If 0, discard immediately. Probably not right and save CPU.
    //Si es 0, descarte inmediatamente. Probablemente no sea correcto y ahorre CPU
    if (sensor_value == 0)
      return;
  
    //Discard readings that deviates too much from the past avg.
    //Deseche las lecturas que se desvía demasiado de la media pasada.
    mapped = (float)fscale(MIC_LOW, MIC_HIGH, MIC_LOW, (float)MIC_HIGH, (float)sensor_value, 2.0);
    avg = compute_average(avgs, AVGLEN);
  
    if (((avg - mapped) > avg*DEV_THRESH)) //|| ((avg - mapped) < -avg*DEV_THRESH))
      return;
    
    //Insert new avg. values
    insert(mapped, avgs, AVGLEN); 
    insert(avg, long_avg, LONG_SECTOR); 
  
    //Compute the "song average" sensor value
    song_avg += avg;
    iter++;
    if (iter > CYCLES) {  
      song_avg = song_avg / iter;
      iter = 1;
    }
      
    longavg = compute_average(long_avg, LONG_SECTOR);
  
    //Check if we enter HIGH mode 
    check_high(longavg);  
  
    if (songmode == HIGH) {
      fade_scale = 3;
      Color.r = 15;
      Color.g = -1;
      Color.b = -1;
    }
    else if (songmode == NORMAL) {
      fade_scale = 2;
      Color.r = -1;
      Color.b = 15 ;
      Color.g = -1;
    }

  
    //Decides how many of the LEDs will be lit
    curshow = fscale(MIC_LOW, MIC_HIGH, 0.0, (float)NUM_LEDS, (float)avg, -1);
  
    /*Set the different leds. Control for too high and too low values.
            Fun thing to try: Dont account for overflow in one direction, 
      some interesting light effects appear!
   
  Establecer los diferentes leds. Controla valores demasiado altos y muy bajos.
            Algo divertido que probar: no tengas en cuenta el desbordamiento en una dirección,
      ¡Aparecen algunos efectos de luz interesantes!
      */
    for (int i = 0; i < NUM_LEDS; i++) 
      //The leds we want to show
      if (i < curshow) {
        if (leds[i].r + Color.r > 255)
          leds[i].r = 255;
        else if (leds[i].r + Color.r < 0)
          leds[i].r = 0;
        else
          leds[i].r = leds[i].r + Color.r;
            
        if (leds[i].g + Color.g > 255)
          leds[i].g = 255;
        else if (leds[i].g + Color.g < 0)
          leds[i].g = 0;
        else 
          leds[i].g = leds[i].g + Color.g;
  
        if (leds[i].b + Color.b > 255)
          leds[i].b = 255;
        else if (leds[i].b + Color.b < 0)
          leds[i].b = 0;
        else 
          leds[i].b = leds[i].b + Color.b;  
        
      //All the other LEDs begin their fading journey to eventual total darkness
      } else {
        leds[i] = CRGB(leds[i].r/fade_scale, leds[i].g/fade_scale, leds[i].b/fade_scale);
      }
    FastLED.show(); 
  }
  if(!estado){
     for(int i=0; i<60; i++) {
     leds[i] = CRGB::Black;
     FastLED.show();
    }   
  }
   if(estado && modoLampara){
      if (rojo) {
         for(int i=0; i<60; i++) {
         leds[i] = CRGB::Red;
         leds[i] %= grados;
         FastLED.show();       
         }
      }
      if (verde) {
         for(int i=0; i<60; i++) {
         leds[i] = CRGB::Green;
         leds[i].setHue(100);
         leds[i] %= grados;
         FastLED.show();
         }
      }
      if (azul) {
         for(int i=0; i<60; i++) {
         leds[i] = CRGB::Blue;
         leds[i] %= grados;
         FastLED.show();
         }
      }
      if (amarillo) {
         for(int i=0; i<60; i++) {
         leds[i] = CRGB::Gold;
         leds[i] %= grados;
         FastLED.show();
         }
      }
      if (violeta) {
         for(int i=0; i<60; i++) {
         leds[i] = CRGB::Purple;
         leds[i].setHue(195);
         leds[i] %= grados;
         FastLED.show();
         }
      }
       if (naranja) {
         for(int i=0; i<60; i++) {
         leds[i] = CRGB::Orange;
         leds[i].setHue(23);
         leds[i] %= grados;
         FastLED.show();
         }
       }
       if (celeste) {
         for(int i=0; i<60; i++) {
         leds[i] = CRGB::Aqua;
         leds[i].setHue(140);
         leds[i] %= grados;
         FastLED.show();
         }
       }
       if (blanco) {
         for(int i=0; i<60; i++) {
         leds[i] = CRGB::White;
         leds[i] %= grados;
         FastLED.show();
         }
       }
        if (rosado) {
         for(int i=0; i<60; i++) {
         leds[i] = CRGB::Pink;
         leds[i].setHue(224);
         leds[i] %= grados;
         FastLED.show();
         }
       }
    }  
}   

//Compute average of a int array, given the starting pointer and the length
//Calcule el promedio de una matriz int, dado el puntero inicial y la longitud
int compute_average(int *avgs, int len) {
  int sum = 0;
  for (int i = 0; i < len; i++)
    sum += avgs[i];

  return (int)(sum / len);

}

//Insert a value into an array, and shift it down removing
//the first value if array already full 
//Inserte un valor en un array y desplace la eliminación hacia abajo
// el primer valor si la matriz ya está llena
void insert(int val, int *avgs, int len) {
  for (int i = 0; i < len; i++) {
    if (avgs[i] == -1) {
      avgs[i] = val;
      return;
    }  
  }

  for (int i = 1; i < len; i++) {
    avgs[i - 1] = avgs[i];
  }
  avgs[len - 1] = val;
}

//Function imported from the arduino website.
//Basically map, but with a curve on the scale (can be non-uniform).
//Función importada desde el sitio web arduino.
// Básicamente mapa, pero con una curva en la escala (puede ser no uniforme).
float fscale( float originalMin, float originalMax, float newBegin, float
    newEnd, float inputValue, float curve){

  float OriginalRange = 0;
  float NewRange = 0;
  float zeroRefCurVal = 0;
  float normalizedCurVal = 0;
  float rangedValue = 0;
  boolean invFlag = 0;


  // condition curve parameter
  // limit range

  if (curve > 10) curve = 10;
  if (curve < -10) curve = -10;

  curve = (curve * -.1) ; // - invert and scale - this seems more intuitive - postive numbers give more weight to high end on output 
  curve = pow(10, curve); // convert linear scale into lograthimic exponent for other pow function

  // Check for out of range inputValues
  if (inputValue < originalMin) {
    inputValue = originalMin;
  }
  if (inputValue > originalMax) {
    inputValue = originalMax;
  }

  // Zero Refference the values
  OriginalRange = originalMax - originalMin;

  if (newEnd > newBegin){ 
    NewRange = newEnd - newBegin;
  }
  else
  {
    NewRange = newBegin - newEnd; 
    invFlag = 1;
  }

  zeroRefCurVal = inputValue - originalMin;
  normalizedCurVal  =  zeroRefCurVal / OriginalRange;   // normalize to 0 - 1 float

  // Check for originalMin > originalMax  - the math for all other cases i.e. negative numbers seems to work out fine 
  if (originalMin > originalMax ) {
    return 0;
  }

  if (invFlag == 0){
    rangedValue =  (pow(normalizedCurVal, curve) * NewRange) + newBegin;

  }
  else     // invert the ranges
  {   
    rangedValue =  newBegin - (pow(normalizedCurVal, curve) * NewRange); 
  }

  return rangedValue;
}

/****************************** METODOS CONNECTION ************************************/
void reconnect() {
  int statusWifi = WL_IDLE_STATUS;
  // Loop until we're reconnected
  while (!client.connected()) {
    statusWifi = WiFi.status();
    connectToWiFi();
    
    Serial.print("Connecting to ThingsBoard node ...");
    // Attempt to connect (clientId, username, password)
    if ( client.connect(NODE_NAME, NODE_TOKEN, NULL) ) {
      Serial.println( "[DONE]" );
      
      // Suscribir al Topico de request
      client.subscribe(requestTopic); 
      
    } else {
      Serial.print( "[FAILED] [ rc = " );
      Serial.print( client.state() );
      Serial.println( " : retrying in 5 seconds]" );
      // Wait 5 seconds before retrying
      delay( 5000 );
    }
  }
}

/*
 * función para conectarse a wifi
 */
void connectToWiFi()
{
  Serial.println("Connecting to WiFi ...");
  // attempt to connect to WiFi network

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi");
}

String encenderApagarLampara(bool action)
{
  String returnValue = "NO ANDA";
  if (action == true) {
    ESTADOLAMPARA = true;
    sonidoInicio();
    Serial.println("Encendiendo lampara");
    returnValue = "ENCENDIDA";
  }
   else {    
    ESTADOLAMPARA = false; 
    sonidoApagado();
    Serial.println("Apagando lampara");
    returnValue = "APAGADA";
   }
   return returnValue;
}

String cambiarModoLampara(bool action)
{
  String returnValue = "NO ANDA";
  if (action == true) {
    MODOLAMPARA = true;
    Serial.println("Modo lampara ON");
    returnValue = "ENCENDIDA";
  }
   else {    
    MODOLAMPARA = false; 
    Serial.println("Modo lampara OFF");
    returnValue = "APAGADA";
   }
   return returnValue;
}

void colorModoLampara(String color, bool action)
{
  sonidoCambioALampara();
  if (color == "colorRojo") {
        rojo= true;
        verde= false;
        azul= false;
        amarillo= false;
        violeta= false;
        blanco= false;
        naranja= false;
        celeste= false;
        rosado= false;
   }     
      if (color == "colorVerde") {
        rojo= false;
        verde= true;
        azul= false;
        amarillo= false;
        violeta= false;
        blanco= false;
        naranja= false;
        celeste= false;
        rosado= false;
      }
      if (color == "colorAzul") {
        rojo= false;
        verde= false;
        azul= true;
        amarillo= false;
        violeta= false;
        blanco= false;
        naranja= false;
        celeste= false;
        rosado= false;
      }
      if (color == "colorAmarillo") {
        rojo= false;
        verde= false;
        azul= false;
        amarillo= true;
        violeta= false;
        blanco= false;
        naranja= false;
        celeste= false;
        rosado= false;
      }
      if (color == "colorVioleta") {
        rojo= false;
        verde= false;
        azul= false;
        amarillo= false;
        violeta= true;
        blanco= false;
        naranja= false;
        celeste= false;
        rosado= false;
      }
      if (color == "colorNaranja") {
        rojo= false;
        verde= false;
        azul= false;
        amarillo= false;
        violeta= false;
        blanco= false;
        naranja= true;
        celeste= false;
        rosado= false;
      }
      if (color == "colorCeleste") {
        rojo= false;
        verde= false;
        azul= false;
        amarillo= false;
        violeta= false;
        blanco= false;
        naranja= false;
        celeste= true;
        rosado= false;
      }
       if (color == "colorBlanco") {
        rojo= false;
        verde= false;
        azul= false;
        amarillo= false;
        violeta= false;
        blanco= true;
        naranja= false;
        celeste= false;
        rosado= false;
      }
      if (color == "colorRosado") {
        rojo= false;
        verde= false;
        azul= false;
        amarillo= false;
        violeta= false;
        blanco= false;
        naranja= false;
        celeste= false;
        rosado= true;
      }
}

void intensidadDeLampara(float grados)
{
    INTENSIDADLAMPARA = grados;
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
   
  if (methodName.equals("manejoDeLampara")) {
    bool action = data["params"];
    String estadoDeLampara = encenderApagarLampara(action);

  }

  if (methodName.equals("modoLampara")) {
    bool action = data["params"];
    String estadoDeLampara = cambiarModoLampara(action);

  }

  if (methodName.equals("colorRojo")) {
    bool action = data["params"];
    colorModoLampara("colorRojo",action);

  }
  if (methodName.equals("colorVerde")) {
    bool action = data["params"];
    colorModoLampara("colorVerde",action);

  }

  if (methodName.equals("colorAzul")) {
    bool action = data["params"];
    colorModoLampara("colorAzul",action);

  }

  if (methodName.equals("colorAmarillo")) {
    bool action = data["params"];
    colorModoLampara("colorAmarillo",action);

  }
  if (methodName.equals("colorVioleta")) {
    bool action = data["params"];
    colorModoLampara("colorVioleta",action);

  }
  if (methodName.equals("colorBlanco")) {
    bool action = data["params"];
    colorModoLampara("colorBlanco",action);
    
  }
  if (methodName.equals("colorNaranja")) {
    bool action = data["params"];
    colorModoLampara("colorNaranja",action);

  }
  if (methodName.equals("colorCeleste")) {
    bool action = data["params"];
    colorModoLampara("colorCeleste",action);
    
  }
  if (methodName.equals("colorRosado")) {
    bool action = data["params"];
    colorModoLampara("colorRosado",action);
    
  }
  if (methodName.equals("setIntensidad")) {
    float grados = data["params"];
    intensidadDeLampara(grados);    
  }
 
}

void getAndSendData()
{
 
}

