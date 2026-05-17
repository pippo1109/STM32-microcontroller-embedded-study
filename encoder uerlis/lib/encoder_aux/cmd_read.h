#include <Arduino.h>

// ===== CMD READ FUNCTIONS BEGIN =====
void exibirBarra(int pct) {
  int blocos = pct / 5;
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

  int pwmValor = (potencia == 0) ? 0 : map(potencia, 0, 100, 51, 255);
  analogWrite(PWM_PIN, pwmValor);

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

  } else if (cmd == "+") {
    definirPotencia(potencia + 5);

  } else if (cmd == "-") {
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
void lerComandos() {
  if (Serial.available() > 0) {
    String entrada = Serial.readStringUntil('\n');
    entrada.trim();
    entrada.toLowerCase();

    if (entrada.length() > 0) {
      processarComando(entrada);
    }
  }
}
// ===== CMD READ FUNCTIONS END =====