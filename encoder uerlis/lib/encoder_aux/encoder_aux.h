#include <Arduino.h>
#include <encoder_variables.h>
#include <cmd_read.h>
#include <measurement.h>
#include <cmd_write.h>

void initialize() {
  // Inicializa Serial
  Serial.begin(9600);
  
  // Configura pino PWM do motor
  pinMode(PWM_PIN, OUTPUT);
  analogWrite(PWM_PIN, 0);  // Começa desligado
  
  // Configura pino do encoder
  pinMode(ENCODER_PIN, INPUT_PULLUP);
  
  // Ativa interrupção para encoder (INT0 = Pino 2)
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN), contarPulso, FALLING);
  
  // Marca tempo inicial
  tempo_inicio = millis();
  tempo_anterior = millis();
  
  // Mensagem de início
  Serial.println(F("========================================"));
  Serial.println(F("  MOTOR PWM + AQUISIÇÃO DE ENCODER"));
  Serial.println(F("  PWM: Pino 9 | Encoder: Pino 2"));
  Serial.println(F("  Baud: 9600"));
  Serial.println(F("========================================"));
  Serial.println(F("  Enviando: tempo_ms,rpm,omega_rad_s"));
  Serial.println(F("========================================"));
  exibirAjuda();
}

void motorLoop() {
  for (int passo = 0; passo <= 100; passo += 10) {
    definirPotencia(passo);
    unsigned long inicio_step = millis();
    while (millis() - inicio_step < TEMPO_POR_STEP_MS) {
      lerComandos();
      if (millis() - tempo_anterior >= intervalo_envio) {
        enviarDados();
      }

    }
  }

  for (int passo = 100; passo >= 0; passo -= 10) {
    definirPotencia(passo);
    unsigned long inicio_step = millis();
    while (millis() - inicio_step < TEMPO_POR_STEP_MS) {
      lerComandos();
      if (millis() - tempo_anterior >= intervalo_envio) {
        enviarDados();
      }

    }
  }
}

