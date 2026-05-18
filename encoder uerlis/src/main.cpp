#include <Arduino.h>
#include <encoder_aux.h>

/*
 * Controle de Motor PWM + Aquisição de Dados do Encoder
 * Arduino Mega - Pino 9 (PWM) | Pino 2 (Encoder/INT0)
 *
 * CONTROLE PWM (Pino 9):
 *   0 a 100     → define a potência em porcentagem (ex: 75)
 *   +           → aumenta 5%
 *   -           → diminui 5%
 *   on/off      → liga/desliga
 *   status      → mostra status atual
 *
 * ENVIO DE DADOS (Automático):
 *   Formato: tempo_ms,rpm,omega_rad_s
 *   Intervalo: a cada 100ms
 */


void setup() {
  initialize();
}

void loop() {
  lerComandos();
  enviarDados();
 // motorLoop();

}






