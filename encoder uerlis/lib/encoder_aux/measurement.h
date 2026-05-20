#include <Arduino.h>

// ===== ENCODER MEASUREMENT FUNCTIONS BEGIN =====

const int NUM_MAGNETOS = 20;

// Intervalo mínimo entre pulsos válidos (µs).
// Protege contra bounce e ruído elétrico.
// 300 µs → seguro até ~10 000 RPM com 20 imãs.
const unsigned long INTERVALO_MIN_US = 300UL;

// Buffer circular: armazena o intervalo de cada pulso.
// A soma dos NUM_MAGNETOS intervalos = tempo de 1 volta completa.
volatile unsigned long _intervalos[NUM_MAGNETOS];
volatile int           _idxPulso   = 0;
volatile unsigned long _ultimoPulso = 0;

// Contador de voltas completas (incrementado na ISR a cada NUM_MAGNETOS pulsos)
volatile int _voltasContadas   = 0;
int          _voltasProcessadas = 0;

// Variáveis de saída
float omega         = 0.0f;
float omegaAnterior = 0.0f;

// ── EMA sobre omega por volta ─────────────────
// Substitui a média móvel anterior.
// Alpha = 0.15: suave mas responsivo. Aumente para resposta mais rápida.
const float EMA_ALPHA = 0.15f;
float _omegaEma    = 0.0f;
bool  _emaIniciado = false;

// Compatibilidade: mantém assinatura de limparBuffer() usada em cmd_write.h
void limparBuffer() {
  noInterrupts();
  memset((void*)_intervalos, 0, sizeof(_intervalos));
  _idxPulso = 0;
  interrupts();
  _omegaEma    = 0.0f;
  _emaIniciado = false;
}

// ── ISR ──────────────────────────────────────
void contarPulso() {
  unsigned long agora    = micros();
  unsigned long intervalo = agora - _ultimoPulso;

  if (intervalo < INTERVALO_MIN_US) return;  // descarta bounce/ruído

  _intervalos[_idxPulso] = intervalo;
  _idxPulso = (_idxPulso + 1) % NUM_MAGNETOS;
  _ultimoPulso = agora;

  // Sinaliza volta completa a cada NUM_MAGNETOS pulsos válidos
  static int _contLocal = 0;
  if (++_contLocal >= NUM_MAGNETOS) {
    _contLocal = 0;
    _voltasContadas++;
  }
}

// ── Calcula omega (rad/s) somando 1 volta completa ───────────────────────────
// Cancela erros de espaçamento dos imãs: se um imã está adiantado,
// o seguinte está atrasado na mesma proporção — a soma se mantém estável.
float calcularOmega() {
  unsigned long copia[NUM_MAGNETOS];
  noInterrupts();
  memcpy((void*)copia, (const void*)_intervalos, sizeof(copia));
  interrupts();

  unsigned long totalUs = 0;
  for (int i = 0; i < NUM_MAGNETOS; i++) totalUs += copia[i];

  if (totalUs == 0) return 0.0f;

  // totalUs = período de 1 volta em µs → ω = 2π / T(s)
  return (2.0f * PI) / (totalUs / 1e6f);
}

// ===== ENCODER MEASUREMENT FUNCTIONS END =====
