#include <Arduino.h>

const int SENSOR_PIN = 2;
const int NUM_MAGNETOS = 20;

volatile unsigned long _tempoPulso = 0;
volatile bool _novoPulso = false;
volatile unsigned long _ultimoPulso = 0;

// Cópia segura lida no loop
unsigned long tempoPulsoSeguro = 0;

float omega = 0;
float omegaAnterior = 0;

// ── Limite físico de aceleração ───────────────
// Ajuste conforme seu motor. Ex: 95 rad/s² = limite razoável
const float DELTA_OMEGA_MAX = 95.0;

// ── Média móvel ───────────────────────────────
const int N = 16;
float bufOmega[N] = {};
int bufIdx = 0;
bool bufCheio = false;  // ← NOVO: saber se o buffer já foi preenchido

float mediaMovel(float novo) {
  bufOmega[bufIdx] = novo;
  bufIdx = (bufIdx + 1) % N;
  float soma = 0;
  for (int i = 0; i < N; i++) soma += bufOmega[i];
  return soma / N;
}

void limparBuffer() {
  for (int i = 0; i < N; i++) bufOmega[i] = 0;
  bufIdx = 0;
  bufCheio = false;
}

// ===== ENCODER MEASUREMENT FUNCTIONS BEGIN =====
void contarPulso() {
  unsigned long agora = micros();
  unsigned long intervalo = agora - _ultimoPulso;
  unsigned long minimoEsperado = max(_tempoPulso / 2, 1000UL);

  if (intervalo < minimoEsperado) return;

  _tempoPulso = intervalo;
  _ultimoPulso = agora;
  _novoPulso = true;
}
// ===== ENCODER MEASUREMENT FUNCTIONS END =====