#include <Arduino.h>
#include <DHT.h>
#include <ESP32Servo.h>         // Librería para controlar el servomotor

// Pines de los sensores LM35 en el ESP32
int sensor1 = 36;  // Primer sensor en GPIO36
int sensor2 = 39;  // Segundo sensor en GPIO39
int sensor3 = 34;  // Tercer sensor en GPIO34

// Número de lecturas para promediar
const int numLecturas = 10;

// Configuración del servomotor
int pinServo = 15;  // Conectar el pin de control del servomotor al GPIO15

// Configuración del ventilador
const int fanPin = 12;  // Pin del ventilador
int cicloTrabajo = 0;   // Ciclo de trabajo inicial del ventilador
int incremento = 5;     // Paso de incremento para el ciclo de trabajo del ventilador
const int periodoPWM = 50000; // Periodo total del ciclo en microsegundos

// Configuración del foco
const int focoPin = 14;      // Pin de salida para el foco
bool focoEncendido = true;   // Estado inicial del foco
unsigned long tiempoApagado = 0;  // Tiempo de apagado del foco

// Definir pines para el sensor DHT11
#define DHTPIN 0                // Pin del sensor DHT11
#define DHTTYPE DHT11            // Tipo de sensor DHT
#define BOMBA_PIN 2             // Pin para controlar la bomba (base del transistor)
#define HUMIDITY_THRESHOLD 70    // Umbral de humedad para activar el riego

DHT dht(DHTPIN, DHTTYPE);        // Inicializar el sensor DHT

void setup() {
  Serial.begin(115200); // Inicializa la comunicación serial a 115200 bps

  // Inicializar el sensor DHT
  dht.begin();

  // Configuración de pines de salida
  pinMode(pinServo, OUTPUT);
  pinMode(fanPin, OUTPUT);
  pinMode(focoPin, OUTPUT);
  pinMode(BOMBA_PIN, OUTPUT);
  
  digitalWrite(focoPin, HIGH);  // Encender el foco al inicio
}

void loop() {
  // Leer cada sensor y calcular el promedio de las lecturas de temperatura
  float temp1 = leerTemperaturaPromedio(sensor1);
  float temp2 = leerTemperaturaPromedio(sensor2);
  float temp3 = leerTemperaturaPromedio(sensor3);

  // Calcular el promedio de las tres temperaturas
  float promedio = (temp1 + temp2 + temp3) / 3.0;

  // Imprimir las temperaturas y el promedio en el monitor serial
  Serial.print("Promedio de temperatura: ");
  Serial.print(promedio);
  Serial.println(" °C");

  // Control del servomotor y ventilador basado en el promedio de temperatura
  if (promedio >= 19.0) {
    moverServo(90);  // Mueve el servomotor a 90 grados
    Serial.println("Servomotor en 90 grados");

    // Incrementa el ciclo de trabajo del ventilador
    for (cicloTrabajo = 0; cicloTrabajo <= 100; cicloTrabajo += incremento) {
      generarPWMVentilador(cicloTrabajo);
      delay(20); // Pausa para hacer la transición más notoria
    }

    // Apagar el foco si está encendido
    if (focoEncendido) {
      digitalWrite(focoPin, LOW);
      focoEncendido = false;
      tiempoApagado = millis();  // Guardar el tiempo de apagado
      Serial.println("Foco apagado");
    }
  } else {
    moverServo(0);  // Mueve el servomotor a 0 grados
    Serial.println("Servomotor en 0 grados");

    // Reduce el ciclo de trabajo del ventilador
    for (cicloTrabajo = 100; cicloTrabajo >= 0; cicloTrabajo -= incremento) {
      generarPWMVentilador(cicloTrabajo);
      delay(20); // Pausa para hacer la transición más notoria
    }
  }

  // Verificar si han pasado 5 segundos desde que se apagó el foco para encenderlo nuevamente
  if (!focoEncendido && (millis() - tiempoApagado >= 20000)) {
    digitalWrite(focoPin, HIGH);
    focoEncendido = true;
    Serial.println("Foco encendido nuevamente después de 5 segundos");
  }

  // Comprobamos si hay datos disponibles en el puerto serial para el control de la bomba
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');  // Leer comando serial
    
    if (command == "START") {
      // Encender la minibomba
      digitalWrite(BOMBA_PIN, HIGH);
      Serial.println("Bomba encendida por comando serial");
    } 
    else if (command == "STOP") {
      // Apagar la minibomba
      digitalWrite(BOMBA_PIN, LOW);
      Serial.println("Bomba apagada por comando serial");
    }
  }

  // Leer humedad ambiental (DHT11)
  float h = dht.readHumidity();
  float temperatura = dht.readTemperature();

  // Verificar si las lecturas son válidas
  if (isnan(h)) {
    Serial.println("Error al leer el sensor DHT!");
  } else {
    // Imprimir valores en el monitor serial
    Serial.print("Humedad: ");
    Serial.print(h);
    Serial.print(" %\t");
  }

  delay(1000); // Esperar 1 segundo antes de tomar una nueva lectura
}

// Función que convierte la lectura analógica a temperatura en grados Celsius,
// con promediado para mejorar la precisión
float leerTemperaturaPromedio(int pin) {
  float suma = 0;

  for (int i = 0; i < numLecturas; i++) {
    int lectura = analogRead(pin);             // Lee el pin analógico
    float voltaje = lectura * (3.3 / 4095.0);  // Convierte la lectura en voltaje
    float temperatura = voltaje * 100;         // Convierte el voltaje a grados Celsius
    suma += temperatura;
    delay(10);  // Espera un corto tiempo entre lecturas para evitar ruido
  }

  return suma / numLecturas;
}

// Función para mover el servomotor a un ángulo específico (0-90 grados) sin usar librerías
void moverServo(int angulo) {
  int pulso = map(angulo, 0, 90, 500, 1500); // Mapea el ángulo a ancho de pulso (500 a 1500 microsegundos)

  for (int i = 0; i < 50; i++) { // Enviar 50 pulsos para mantener la posición durante 1 segundo
    digitalWrite(pinServo, HIGH);
    delayMicroseconds(pulso);
    digitalWrite(pinServo, LOW);
    delay(20 - pulso / 1000);  // Completa el periodo de 20 ms
  }
}

// Función para generar PWM manual en el ventilador según el ciclo de trabajo
void generarPWMVentilador(int cicloTrabajo) {
  int tiempoAlto = (cicloTrabajo * periodoPWM) / 100;
  int tiempoBajo = periodoPWM - tiempoAlto;

  digitalWrite(fanPin, HIGH);
  delayMicroseconds(tiempoAlto);
  digitalWrite(fanPin, LOW);
  delayMicroseconds(tiempoBajo);
}
