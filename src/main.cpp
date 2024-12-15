#include <Arduino.h>
#include "audio_constants.h"
#include "audio_data.h"  // Incluye tu archivo con los datos WAV
#include "ringtones-harry-potter_data_0.h"
#include "ringtones-harry-potter_data_1.h"
#include "ringtones-harry-potter_data_2.h"
#include "ringtones-harry-potter_data_3.h"
#include "ringtones-harry-potter_data_4.h"
#include "ringtones-harry-potter_data_5.h"
#include "ringtones-harry-potter_data_6.h"
#include "ringtones-harry-potter_data_7.h"

// Definimos los pines touch que vamos a usar
// Los números corresponden a GPIO físicos:
// T0=GPIO4, T3=GPIO15, T4=GPIO13, T5=GPIO12, T6=GPIO14, T7=GPIO27, T8=GPIO33, T9=GPIO32
const uint8_t touchPins[] = {4, 15, 13, 12, 14, 27, 33, 32};
const int numPins = 8;

// Umbral de detección (ajusta este valor según la sensibilidad deseada)
const int TOUCH_THRESHOLD = 60;

// Definimos el pin del LED
#define LED_PIN 2

// Definir el pin del relé
#define RELAY_PIN 17

// Calculamos la longitud del array test_wav
#define TEST_WAV_LEN sizeof(&test_wav)

// Variables globales para rastrear la secuencia
int lastChunkPlayed = -1;
bool sequenceCorrect = true;
unsigned long lastTouchTime = 0;
const unsigned long SEQUENCE_TIMEOUT = 3000; // 3 segundos de timeout

// Función para generar un tono usando DAC
void playNumber(int number) {
  Serial.print("Número: ");
  Serial.println(number + 1);
  
  int freq = frequencies[number];
  for(int i = 0; i < 1000; i++) {
    dacWrite(DAC1, (sin(2 * PI * freq * i / 8000.0) + 1) * 127);
    dacWrite(DAC2, (sin(2 * PI * freq * i / 8000.0) + 1) * 127);
    delayMicroseconds(125);
  }
  
  dacWrite(DAC1, 0);
  dacWrite(DAC2, 0);
}

void playWav() {
  const int sampleRate = 8000;  // Cambiamos a 44.1kHz (valor típico de WAV)
  const int delayTime = 1000000 / sampleRate;  // Esto dará ~23 microsegundos
  const int WAV_HEADER_SIZE = 44;
  
  for(unsigned int i = WAV_HEADER_SIZE; i < 197294; i++) {  // Ajusta este número según el tamaño de tu WAV
    uint8_t sample = pgm_read_byte(&test_wav[i]);
    dacWrite(DAC1, sample);
    dacWrite(DAC2, sample);
    delayMicroseconds(delayTime);
  }
  
  dacWrite(DAC1, 0);
  dacWrite(DAC2, 0);
}

void playHarryPotterChunk(int chunk) {
  // Si el chunk no es el siguiente esperado en la secuencia, reiniciamos
  if (chunk != lastChunkPlayed + 1) {
    sequenceCorrect = false;
    lastChunkPlayed = -1;
    Serial.println("Secuencia incorrecta - reiniciando");
    return;
  }
  Serial.println("Secuencia correcta  - continuando");
  Serial.println("Chunk: ");
  Serial.println(chunk);
  
  const int sampleRate = 8000;
  const int delayTime = 1000000 / sampleRate;
  const unsigned char* audio_data;
  unsigned int audio_len;
  
  // Seleccionar el chunk correcto
  switch(chunk) {
    case 0:
      audio_data = ringtones_harry_potter_chunk0_bin;
      audio_len = ringtones_harry_potter_chunk0_bin_len;
      break;
    case 1:
      audio_data = ringtones_harry_potter_chunk1_bin;
      audio_len = ringtones_harry_potter_chunk1_bin_len;
      break;
    case 2:
      audio_data = ringtones_harry_potter_chunk2_bin;
      audio_len = ringtones_harry_potter_chunk2_bin_len;
      break;
    case 3:
      audio_data = ringtones_harry_potter_chunk3_bin;
      audio_len = ringtones_harry_potter_chunk3_bin_len;
      break;
    case 4:
      audio_data = ringtones_harry_potter_chunk4_bin;
      audio_len = ringtones_harry_potter_chunk4_bin_len;
      break;
    case 5:
      audio_data = ringtones_harry_potter_chunk5_bin;
      audio_len = ringtones_harry_potter_chunk5_bin_len;
      break;
    case 6:
      audio_data = ringtones_harry_potter_chunk6_bin;
      audio_len = ringtones_harry_potter_chunk6_bin_len;
      break;
    case 7:
      audio_data = ringtones_harry_potter_chunk7_bin;
      audio_len = ringtones_harry_potter_chunk7_bin_len;
      break;
    default:
      return;
  }
  
  // Reproducir el audio
  for(unsigned int i = 0; i < audio_len; i++) {
    uint8_t sample = pgm_read_byte(&audio_data[i]);
    dacWrite(DAC1, sample);
    dacWrite(DAC2, sample);
    delayMicroseconds(delayTime);
  }
  
  // Silenciar los DACs al terminar
  dacWrite(DAC1, 0);
  dacWrite(DAC2, 0);

  // Actualizar el seguimiento de la secuencia
  lastChunkPlayed = chunk;
  lastTouchTime = millis();

  // Si completamos la secuencia (0-7), activamos el relé
  if (lastChunkPlayed == 7) {
    Serial.println("¡Secuencia completa! Activando relé");
    digitalWrite(RELAY_PIN, HIGH);
    delay(1000); // Mantener el relé activado por 1 segundo
    digitalWrite(RELAY_PIN, LOW);
    // Reiniciar la secuencia
    lastChunkPlayed = -1;
  }
}

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(0);  // Evita timeouts en la transmisión
  while(!Serial) {           // Espera a que el serial esté listo
    delay(100);
  }
  Serial.println("\n\nESP32 Touch Test iniciado");  // Doble salto de línea para mejor visibilidad
  pinMode(LED_PIN, OUTPUT);  // Configuramos el LED como salida
  delay(1000);
  
  // Configurar el pin del relé como salida
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);  // Inicialmente apagado
}

void loop() {
  // Verificar timeout
  if (lastChunkPlayed >= 0 && (millis() - lastTouchTime) > SEQUENCE_TIMEOUT) {
    Serial.println("Timeout - reiniciando secuencia");
    lastChunkPlayed = -1;
    sequenceCorrect = true;
  }

  bool touchDetected = false;
  
  for(int i = 0; i < numPins; i++) {
    int touchValue = touchRead(touchPins[i]);
    
    if(touchValue < TOUCH_THRESHOLD) {
      touchDetected = true;
      
      Serial.print("Touch detectado en pin GPIO");
      Serial.print(touchPins[i]);
      Serial.print(" (T");
      Serial.print(i);
      Serial.print(") - Valor: ");
      Serial.println(touchValue);
      
      // Reproducimos el chunk correspondiente de Harry Potter
      playHarryPotterChunk(i);
      delay(100); // Pequeño delay para evitar lecturas múltiples
    }
  }
  
  digitalWrite(LED_PIN, touchDetected ? HIGH : LOW);
  delay(100);
}