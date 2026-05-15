#include <Arduino.h>

/*
 * Controle de Potência PWM via Terminal Serial
 * Arduino Mega - Pino 9 (PWM)
 *
 * Comandos disponíveis:
 *   0 a 100     → define a potência em porcentagem (ex: 75)
 *   +           → aumenta 5%
 *   -           → diminui 5%
 *   status      → exibe a potência atual
 *   help        → exibe os comandos disponíveis
 *   off         → desliga (0%)
 *   on          → liga ao máximo (100%)
 */

const int PWM_PIN = 9;       // Pino PWM (pode usar: 2-13 no Mega)
int potencia = 0;            // Potência atual em porcentagem (0-100)


void exibirBarra(int pct) {
  int blocos = pct / 5;   // até 20 blocos
  Serial.print(F("  ["));
  for (int i = 0; i < 20; i++) {
    Serial.print(i < blocos ? "#" : ".");
  }
  Serial.print(F("] "));
  Serial.print(pct);
  Serial.println(F("%"));
}

void definirPotencia(int novaPotencia) {
  novaPotencia = constrain(novaPotencia, 0, 100);
  potencia = novaPotencia;

  // IRLZ44N: VGS(th) ~1.0V, condução plena em 5V
  // 1.0V / 5.0V * 255 = 51  → PWM mínimo para abrir o gate
  // 5.0V             = 255  → gate totalmente aberto
  // Faixa útil: PWM 51–255 (VGS ~1.0V a 5.0V)
  // potencia == 0 → analogWrite(0) garante gate em 0V (MOSFET fechado)
  int pwmValor = (potencia == 0) ? 0 : map(potencia, 0, 100, 51, 255);
  analogWrite(PWM_PIN, pwmValor);

  // Calcula VGS médio real na saída do Arduino
  float vgs = (pwmValor / 255.0) * 5.0;
  float tensaoMedia = (potencia / 100.0) * 12.0;

  Serial.print(F("[PWM] Potencia: "));
  Serial.print(potencia);
  Serial.print(F("% | PWM: "));
  Serial.print(pwmValor);
  Serial.print(F("/255 | VGS: ~"));
  Serial.print(vgs, 2);
  Serial.print(F("V | Carga: ~"));
  Serial.print(tensaoMedia, 1);
  Serial.println(F("V"));

  exibirBarra(potencia);
}

void exibirStatus() {
  int pwmValor = map(potencia, 0, 100, 0, 255);
  Serial.println(F("--- Status Atual ---"));
  Serial.print(F("  Pino PWM : "));
  Serial.println(PWM_PIN);
  Serial.print(F("  Potência : "));
  Serial.print(potencia);
  Serial.println(F("%"));
  Serial.print(F("  Valor PWM: "));
  Serial.print(pwmValor);
  Serial.println(F(" / 255"));
  exibirBarra(potencia);
}

void exibirAjuda() {
  Serial.println(F(""));
  Serial.println(F("  Comandos disponíveis:"));
  Serial.println(F("  ----------------------"));
  Serial.println(F("  0 a 100  → define a potência (%)"));
  Serial.println(F("  +        → aumenta 5%"));
  Serial.println(F("  -        → diminui 5%"));
  Serial.println(F("  on       → potência máxima (100%)"));
  Serial.println(F("  off      → desliga (0%)"));
  Serial.println(F("  status   → exibe situação atual"));
  Serial.println(F("  help     → exibe esta ajuda"));
  Serial.println(F(""));
}

void processarComando(String cmd) {
  if (cmd == "help" || cmd == "?") {
    exibirAjuda();

  } else if (cmd == "status") {
    exibirStatus();

  } else if (cmd == "off") {
    definirPotencia(0);

  } else if (cmd == "on") {
    definirPotencia(100);

  } else if (cmd == "h") {
    definirPotencia(potencia + 5);

  } else if (cmd == "l") {
    definirPotencia(potencia - 5);

  } else if (cmd.toInt() != 0 || cmd == "0") {
    int valor = cmd.toInt();
    definirPotencia(valor);

  } else {
    Serial.print(F("[ERRO] Comando desconhecido: \""));
    Serial.print(cmd);
    Serial.println(F("\" — Digite 'help' para ver os comandos."));
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(PWM_PIN, OUTPUT);
  analogWrite(PWM_PIN, 0);

  Serial.println(F("========================================"));
  Serial.println(F("  Controle de PWM - Arduino Mega"));
  Serial.println(F("  Pino: 9 | Baud: 9600"));
  Serial.println(F("========================================"));
  exibirAjuda();
}

void loop() {
  if (Serial.available() > 0) {
    String entrada = Serial.readStringUntil('\n');
    entrada.trim();   // remove espaços e \r
    entrada.toLowerCase();

    if (entrada.length() == 0) return;

    processarComando(entrada);
  }
}





