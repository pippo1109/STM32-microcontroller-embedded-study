#include <Arduino.h>

volatile long contador_pulsos = 0;
const int PULSOS_POR_ROTACAO = 20;     // 20 imãs → 20 pulsos por rotação
const float CONSTANTE_CONVERSAO = (60000.0 / PULSOS_POR_ROTACAO);
const float PI_2 = 2.0 * 3.14159265359;

// ===== ENCODER MEASUREMENT FUNCTIONS BEGIN =====
void contarPulso() {
  contador_pulsos++;
}
float calcularRPM(long pulsos, unsigned long tempo_ms) {
  if (tempo_ms == 0) return 0;
  return (pulsos * CONSTANTE_CONVERSAO) / tempo_ms;
}
float calcularOmega(float rpm) {
  return rpm * PI_2 / 60.0;
}
// ===== ENCODER MEASUREMENT FUNCTIONS END =====