#include <Arduino.h>

// ===== CMD WRITE FUNCTIONS BEGIN =====
void enviarDados() {
  noInterrupts();
  bool temPulso = _novoPulso;
  if (temPulso) {
    tempoPulsoSeguro = _tempoPulso;
    _novoPulso = false;
  }
  interrupts();

  if (temPulso) {
    float anguloRad = (2.0 * PI) / NUM_MAGNETOS;
    float dt = tempoPulsoSeguro / 1e6;

    omegaAnterior = omega;
    float omegaBruta = anguloRad / dt;
    omega = mediaMovel(omegaBruta);

    float rpm = (omega * 60.0) / (2.0 * PI);
    Serial.print("ω: ");         
    Serial.print(omega, 2);
    Serial.print(" rad/s | RPM: "); 
    Serial.println(rpm, 1);

  }

  // ── Detecta parada ────────────────────────────
  noInterrupts();
  unsigned long ultimoPulsoLocal = _ultimoPulso;
  interrupts();

  if (micros() - ultimoPulsoLocal > 500000UL) {
    omega = 0;
    omegaAnterior = 0;
    limparBuffer();
  }
}
// ===== CMD WRITE FUNCTIONS END =====