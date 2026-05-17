#include <Arduino.h>

// ===== CONTROLE PWM DO MOTOR =====
const int PWM_PIN = 9;                 // Pino PWM
int potencia = 0;                      // Potência em % (0-100)

// ===== SENSOR ENCODER =====
const int ENCODER_PIN = 2;             // Pino com interrupção (INT0)

// Diâmetro do círculo dos ímãs (mm) e raio em metros (para velocidade tangencial)
const float DIAMETRO_MAGNETOS_MM = 52.5; 
const float RAIO_MAGNETOS_M = (DIAMETRO_MAGNETOS_MM / 2.0) / 1000.0; // m

// ===== VARIÁVEIS DE TEMPO =====
unsigned long tempo_anterior = 0;
unsigned long tempo_inicio = 0;
unsigned long intervalo_envio = 100;   // Enviar dados a cada 100ms
const unsigned long TEMPO_POR_STEP_MS = 1000; // Duracao de cada degrau de potencia em ms

