#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2); //Direccion de LCD, Pines A4 (SDA) y A5 (SCL)

#include <Wire.h>
#include <math.h>

#include <avr/pgmspace.h> // Función PROGMEM, sirve para que no ocupe datos en la memoria sdram, pero si la flash

#include <OneWire.h>
#include <DallasTemperature.h>

//--------------------MicroSD

#include <SPI.h>
#include <SD.h>
const int chipSelect = 4;
File myFile;

// -- Variables Globales--
byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;

//--------------------LED indicador de Grabacion

#define LED_GRABACION 8    // Funciona solo con el 8, 2 y 3 no responde
int ledState = LOW ; // Estado inicial para el parpadeo del led indicador de grabación

//--------------------RTC
#include "RTClib.h"
RTC_DS1307 rtc;
#define DS1307_I2C_ADDRESS 0x68  // Dirección I2C 

//------------------------------ BULBO SECO

OneWire ourWire1(2); // Pin 2 de entrada
DallasTemperature sensors1(&ourWire1); // se declara com ovariable u objeto

float Rseco;
float Vs;
float TSeco;
float R_ths;
float kelvinSeco;
float logRs;

//------------------------------PROMEDIO PARA EL BULBO SECO

const int numReadingsBS = 20;

int readingsBS[numReadingsBS];
int readIndexBS = 0;
int totalBS = 0;
int averageBS = 0;

//------------------------------ BULBO HUMEDO

OneWire ourWire2(3); //Pin 3 de entrada
DallasTemperature sensors2(&ourWire2);

float Rhumedo;
float Vh;
float THumedo;
float R_thh;
float kelvinHumedo;
float logRh;

//------------------------------PROMEDIO PARA EL BULBO HUMEDO

const int numReadingsBH = 20;

int readingsBH[numReadingsBH];
int readIndexBH = 0;
int totalBH = 0;
int averageBH = 0;

//------------------------------CONSTANTES UNIVERSALES

const float K = 4.0; //factor de disipacion en mW/C
const float Vcc = 6.64; //Aqui debemois medir la tensiónde la fuente que se va a utilizar
float HR;

//------------------------------SETEO DE TIEMPOS DE REFRESCO (Display)

unsigned long previousMillis = 0;
const long interval = 2500 ;// Se achico el tiempo para que se pueda ver el segundero del RTC

//------------------------------SETEO DE TIEMPOS DE REFRESCO (Datalogger)

unsigned long previousMillis_1 = 0 ;
const long interval_1 = 1800000;   // 1800000 -> 30 min

//------------------------------SETEO DE TIEMPOS DE REFRESCO (Cálculos)

unsigned long previousMillis_2 = 0;
const long interval_2 = 10 ;

//------------------------------SETEO DE TIEMPOS DE REFRESCO (Bink LED)

unsigned long previousMillis_3 = 0;
const long interval_3 = 250 ;

//------------------------------HUMEDAD RELATIVA

float pwsTSeco;

float TSecoK;

float pwsTHumedo;

float THumedoK;

float pv;

//----------------------------Configuración incial

void setup()
{
  //lcd.clear();
  lcd.init();
  lcd.begin(16, 2);
  lcd.backlight();// Indicamos medidas de LCD
  Wire.begin(); //configura el bus I2C estableciendo arduino como MASTER

  //----Mensaje de Bienvenida----//
  lcd.setCursor(4, 0);
  lcd.print("LAMyEN");

  lcd.setCursor(2, 1);
  lcd.print(" Cargando.");
  delay(1000);
  lcd.setCursor(2, 1);
  lcd.print(" Cargando..");
  delay(1000);
  lcd.setCursor(2, 1);
  lcd.print(" Cargando...");
  delay(1000);
  lcd.clear();

  lcd.setCursor(4, 0);
  lcd.print("LAMyEN");

  lcd.setCursor(2, 1);
  lcd.print(" Cargando.");
  delay(1000);
  lcd.setCursor(2, 1);
  lcd.print(" Cargando..");
  delay(1000);
  lcd.setCursor(2, 1);
  lcd.print(" Cargando...");
  delay(100);
  lcd.clear();

  //------------------------------ Inicio sensores

  sensors1.begin();  
  sensors2.begin();   

  //------------------------------ Configuramos LED indicador

  pinMode(LED_GRABACION, OUTPUT) ;
  digitalWrite(LED_GRABACION, LOW);

  //------------------------------  inicio MicroSD

  Serial.begin(9600);
  Serial.print(F("Iniciando SD ..."));
  
  if (!SD.begin(4)) {
    Serial.println(F("No se pudo inicializar"));
    return;
  }
  Serial.println(F("inicializacion exitosa"));
  if (!SD.exists("datalog.csv"))
  {
    myFile = SD.open("datalog.csv", FILE_WRITE);
    if (myFile) {
      Serial.println(F("Archivo nuevo, Escribiendo encabezado(fila 1)"));
      myFile.println("Fecha,Hora,TS,TH,HR");
      myFile.close();
    } else {

      Serial.println(F("Error creando el archivo datalog.csv"));
    }
  }
}

 //------------------------------  Conversor BCD ==> DECIMAL

    byte bcdToDec(byte val)
    {
      return ( (val / 16 * 10) + (val % 16) );
    }
    
    // -- Imprime con formato de 2 dígitos
    
    String fprint (int dato)
    {
      String retorno = String(dato);
      if (retorno.length() < 2)
        retorno = "0" + retorno;
      return retorno;
      
  //------------------------------ Inicio Promedio

  for (int thisReadingBS = 0; thisReadingBS < numReadingsBS; thisReadingBS++)  // Promedio Bulbo Seco
    readingsBS[thisReadingBS] = 0;
  for (int thisReadingBH = 0; thisReadingBH < numReadingsBH; thisReadingBH++)  // Promedio Bulbo Humedo
    readingsBH[thisReadingBH] = 0;
     
     }


void loop() {

  //------------------------------LED INDICADOR DE ESTADO MICROSD

  if (!SD.exists("datalog.csv")) { // Si el archivo está, queda encendido el led, sino titila
    GrabacionERROR ();
  } else {
    digitalWrite (LED_GRABACION, HIGH);
  }
  //------------------------------INICIAMOS LOS SENSORES

  sensors1.requestTemperatures();  
  sensors2.requestTemperatures();  

  //------------------------------INICIAMOS RTC
  
  DateTime now = rtc.now();
  
  //------------------------------COMIENZO DE LAS FUNCIONES

  //------------------------------RELOJ - Hora en monitor

   LCD_display_Tiempo ();
   RTC ();
  
  //------------------------------DISPLAY

  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    LCD_display ();
  }
    
  //------------------------------MICROSD

  unsigned long currentMillis_1 = millis();

  if (currentMillis_1 - previousMillis_1 >= interval_1) {
    previousMillis_1 = currentMillis_1;
    microSD ();
  }


  //------------------------------CALCULO HUMEDAD RELATIVA

  unsigned long currentMillis_2 = millis();

  if (currentMillis_2 - previousMillis_2 >= interval_2) {
    previousMillis_2 = currentMillis_2;
    HumedadRelativa ();
  }
}

//-------------Funciones-------------//

void HumedadRelativa () {

  //------------------------------Calculo Humedad Relativa--------------------------//

  TSeco = sensors1.getTempCByIndex(0);    // Sensor Bulbo Seco
  THumedo = sensors2.getTempCByIndex(0);  // Sensor Bulbo Humedo

  // FORMULA CIBSE-Facilitada por Marcos

  // Public Function HRpsi(ByVal tBS As Double, ByVal tBH As Double, ByVal p As Double, ByVal Kpsi As Double)
  // HRpsi = Humedad Relativa, calculada con método psicrométrico
  // pv = Presión parcial de vapor
  // Kpsi = Constante psicrométrica
  // Ejemplos
  //     6.66e-4     Sling - CIBSE Guide C 2007 (1.9) -> ********Se opta por este método********
  //     7.99e-4     Screen - CIBSE Guide C 2007 (1.10)

  const float p = 101325; // Valor de la presión atmosférica, obtenido del excel. Promedio anual de la provincia de Santa Fe (Capital)
  const float Kpsi = 6.66e-4; // Metodo Sling, constante

  //-------CONSTANTES

  const float C8 = -5800.2206;
  const float C9 = 1.3914493;
  const float C10 = -0.048640239;
  const float C11 = 0.000041764768;
  const float C12 = -0.000000014452093;
  const float C13 = 6.5459673;

  //-------FORMULA PARA EL CÁLCULO (CIBSE)

  TSecoK = TSeco + 273.15; // ok
  THumedoK = THumedo + 273.15;// ok

  pwsTSeco = exp(C8 / TSecoK + C9 + C10 * TSecoK + C11 * (TSecoK * TSecoK) + C12 * (TSecoK * TSecoK * TSecoK) + C13 * log(TSecoK)); // ok
  pwsTHumedo = exp(C8 / THumedoK + C9 + C10 * THumedoK + C11 * (THumedoK * THumedoK) + C12 * (THumedoK * THumedoK * THumedoK) + C13 * log(THumedoK)); //ok
  pv = pwsTHumedo - (Kpsi) * p * (TSecoK - THumedoK);
  HR = 100 * (pv / pwsTSeco);

}


void LCD_display_Tiempo () {

  //------------------------------MOSTRAR HORA

  lcd.setCursor(8, 0); // En caso de habilitar el segundero atrasar hasta el 8 o más
  lcd.print(fprint(hour)); //imprime hora
  lcd.print(':');
  lcd.print(fprint(minute)); //imprime minutos
  lcd.print(':');
  lcd.print(fprint(second)); //imprime segundos
}


void LCD_display () {

  //------------------------------MOSTRAR VALORES TERMISTOR
  //------------------------------bulbo seco

  if (TSeco < 50) {       // si es mayor a 50, OL, se hace por que se corre la letra TH o TS cuando empieza a contar para el promedio
    lcd.setCursor(0, 0);
    lcd.print(TSeco, 1); // le dejamos una sola décima
    lcd.print("Ts ");
  } else {
    lcd.setCursor(0, 0);
    lcd.print("OL");
    lcd.print("Ts ");
  }
  //------------------------------bulbo húmedo

  if (THumedo < 50) {     //si es mayor a 50, OL, se hace por que se corre la letra TH o TS cuando empieza a contar para el promedio
    lcd.setCursor(0, 1);
    lcd.print(THumedo, 1); // le dejamos una sola décima
    lcd.print("Th ");
  } else {
    lcd.setCursor(0, 1);
    lcd.print("OL");
    lcd.print("Th ");
  }
  //------------------------------humedad Relativa

  if (HR < 100) {
    lcd.setCursor(8, 1);
    lcd.print(HR); // con el floor deja una décima para visualizar (****floor (HR*10)/10) (****
    lcd.setCursor(12, 1);
    lcd.print(" HR%");
  } else {     // Con este IF, pone a 100 cuando la humedad es mayor a 100% (Cuando el cálculo bate fruta en realidad)
    lcd.setCursor(8, 1);
    lcd.print("100.0");
    lcd.setCursor(13, 1);
    lcd.print("HR%");
  }
}


void RTC () {

      // -- Proceso de Lectura --
    Wire.beginTransmission(DS1307_I2C_ADDRESS);   // Linea IC2 en modo escitura
    Wire.write((byte)0x00);                       // Setea el punto de registro a(0x00)
    Wire.endTransmission();                       // Final de la escritura de transmición
    Wire.requestFrom(DS1307_I2C_ADDRESS, 7);      // Abre I2C en modo de envío
    second     = bcdToDec(Wire.read() & 0x7f);    // Lee 7 bytes de datos
    minute     = bcdToDec(Wire.read());
    hour       = bcdToDec(Wire.read() & 0x3f);
    dayOfWeek  = bcdToDec(Wire.read());
    dayOfMonth = bcdToDec(Wire.read());
    month      = bcdToDec(Wire.read());
    year       = bcdToDec(Wire.read());
  
  }

void microSD () {

  //------------------------------Iniciamos RTC y MicroSD ----> VER SI INICIA AL INICIO!!!!

   /* // -- Proceso de Lectura --
    Wire.beginTransmission(DS1307_I2C_ADDRESS);   // Linea IC2 en modo escitura
    Wire.write((byte)0x00);                       // Setea el punto de registro a(0x00)
    Wire.endTransmission();                       // Final de la escritura de transmición
    Wire.requestFrom(DS1307_I2C_ADDRESS, 7);      // Abre I2C en modo de envío
    second     = bcdToDec(Wire.read() & 0x7f);    // Lee 7 bytes de datos
    minute     = bcdToDec(Wire.read());
    hour       = bcdToDec(Wire.read() & 0x3f);
    dayOfWeek  = bcdToDec(Wire.read());
    dayOfMonth = bcdToDec(Wire.read());
    month      = bcdToDec(Wire.read());
    year       = bcdToDec(Wire.read());
    */
    
     //-----------------------------------------ESCRITURA MICROSD 

    myFile = SD.open("datalog.csv", FILE_WRITE);//abrimos  el archivo
  
    if (myFile) {
      
    Serial.print("Escribiendo SD: ");
    myFile.print(fprint(dayOfMonth)); // Día
    myFile.print("/");
    myFile.print(fprint(month)); // Mes
    myFile.print("/");
    myFile.print(fprint(year)); // Año
    myFile.print(" ");
    myFile.print(fprint(hour)); // Hora
    myFile.print(":");
    myFile.print(fprint(minute)); // Minuto
    myFile.print(" ");
    myFile.print(TSeco);
    myFile.print(",");
    myFile.print(THumedo);
    myFile.print(",");
    myFile.println(HR);
    myFile.close(); //cerramos el archivo

    //-----------------------------------------MONITOR SERIAL

    Serial.print(F("Fecha "));
    Serial.print(fprint(dayOfMonth)); // Días
    Serial.print(F("/"));
    Serial.print(fprint(month)); // Mes
    Serial.print("/");
    Serial.print(fprint(year)); // Año
    Serial.print(F(" Hora "));
    Serial.print(fprint(hour)); // Hora
    Serial.print(":");
    Serial.print(fprint(minute)); // Minutos
    Serial.print(F(" "));
    Serial.print(F(" TS:"));
    Serial.print(TSeco);
    Serial.print(F(" TH:"));
    Serial.print(THumedo);
    Serial.print(F(" HR:"));
    Serial.println(HR);
  } else {
    // if the file didn't open, print an error:
    Serial.println("Error al abrir el archivo");
    GrabacionERROR (); // en el caso que de error la lectura del archivo datalog, siempre se comrpueba reiniciando el equipo
  }
}

void GrabacionERROR () {

  unsigned long currentMillis_3 = millis();

  if (currentMillis_3 - previousMillis_3 >= interval_3) {
    previousMillis_3 = currentMillis_3;

    if (ledState == LOW) {
      ledState = HIGH;
    } else {
      ledState = LOW;
    }
    digitalWrite(LED_GRABACION, ledState);
  }
}

