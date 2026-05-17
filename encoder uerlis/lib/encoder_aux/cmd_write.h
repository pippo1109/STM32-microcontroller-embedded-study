#include <Arduino.h>

// ===== CMD WRITE FUNCTIONS BEGIN =====
void enviarDados() {
  unsigned long agora = millis();
  unsigned long tempo_atual = agora - tempo_inicio;
  unsigned long delta_t = agora - tempo_anterior;
  long pulsos = contador_pulsos;
  
  float rpm = calcularRPM(pulsos, delta_t);
  float omega = calcularOmega(rpm);
  
  // Envia: tempo_ms,rpm,omega_rad_s
  Serial.print(tempo_atual);
  Serial.print(",");
  Serial.print(rpm, 2);
  Serial.print(",");
  Serial.println(omega, 2);
  
  contador_pulsos = 0;
  tempo_anterior = agora;
}
// ===== CMD WRITE FUNCTIONS END =====