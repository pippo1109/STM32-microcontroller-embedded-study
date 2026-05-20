#include <Arduino.h>

// ===== CMD WRITE FUNCTIONS BEGIN =====

void enviarDados() {
  // ── Detecta motor parado ──────────────────────────────────────────────────
  noInterrupts();
  unsigned long ultimoLocal = _ultimoPulso;
  interrupts();

  if (micros() - ultimoLocal > 500000UL) {
    // Sem pulso há mais de 500 ms → motor parado
    if (omega != 0.0f) {          // só zera/limpa uma vez
      omega         = 0.0f;
      omegaAnterior = 0.0f;
      limparBuffer();
      _voltasProcessadas = _voltasContadas;   // evita processar volta antiga
      Serial.println(F("[INFO] Motor parado"));
    }
    return;
  }

  // ── Aguarda uma volta completa nova ──────────────────────────────────────
  noInterrupts();
  int voltas = _voltasContadas;
  interrupts();

  if (voltas <= _voltasProcessadas) return;  // ainda na mesma volta
  _voltasProcessadas = voltas;

  // ── Calcula omega desta volta e aplica EMA ────────────────────────────────
  float omegaBruta = calcularOmega();

  // Sanidade: ignora valores fora do esperado
  // (ajuste os limites conforme a faixa real do seu motor)
  if (omegaBruta < 1.0f || omegaBruta > 2100.0f) return;  // ~10 – 20 000 RPM

  omegaAnterior = omega;

  if (!_emaIniciado) {
    _omegaEma    = omegaBruta;   // inicializa com valor real, não com zero
    _emaIniciado = true;
  } else {
    _omegaEma = EMA_ALPHA * omegaBruta + (1.0f - EMA_ALPHA) * _omegaEma;
  }

  omega = _omegaEma;

  // ── Saída serial (mesmo formato que antes) ────────────────────────────────
  float rpm = (omega * 60.0f) / (2.0f * PI);

  Serial.print(F("ω: "));
  Serial.print(omega, 2);
  Serial.print(F(" rad/s | RPM: "));
  Serial.println(rpm, 1);
}

// ===== CMD WRITE FUNCTIONS END =====
