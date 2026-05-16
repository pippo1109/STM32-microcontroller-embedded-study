import serial
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from collections import deque
import csv
from datetime import datetime
import sys
import time

print("""
╔══════════════════════════════════════════════════════════════╗
║     AQUISIÇÃO DE DADOS - SENSOR HALL (ENCODER)              ║
║     Velocidade Angular em RPM e rad/s                       ║
╚══════════════════════════════════════════════════════════════╝

INSTRUÇÕES:
1. Conecte o microcontroller (STM32) via USB
2. O programa aguardará dados na porta serial
3. Os dados serão salvos em um arquivo CSV
4. O gráfico será atualizado em tempo real

FORMATO ESPERADO DOS DADOS:
   tempo_ms,rpm,omega_rad_s
   Exemplo: 1000,500,52.36
""")

# ajuste a porta
PORTA = 'COM3'       # Windows
# PORTA = '/dev/ttyUSB0'  # Linux
BAUD  = 9600
MAX_PONTOS = 1000

try:
    ser = serial.Serial(PORTA, BAUD, timeout=1)
    print(f"\n✓ CONECTADO na porta {PORTA} com baudrate {BAUD}")
    time.sleep(1)  # Aguarda inicialização
except serial.SerialException as e:
    print(f"\n✗ ERRO: Não conseguiu conectar na porta {PORTA}")
    print(f"   Detalhes: {e}")
    print("\n   SOLUÇÃO:")
    print("   1. Verifique se o dispositivo está conectado via USB")
    print("   2. Abra o Gerenciador de Dispositivos e procure por 'COM3'")
    print("   3. Se estiver em outra porta, altere a variável PORTA no código")
    print("   4. Execute 'teste_porta_serial.py' para diagnosticar")
    sys.exit(1)

tempo_data = deque(maxlen=MAX_PONTOS)
rpm_data   = deque(maxlen=MAX_PONTOS)
omega_data = deque(maxlen=MAX_PONTOS)

# salva CSV automaticamente
arquivo_csv = f"medicao_{datetime.now().strftime('%H%M%S')}.csv"
csvfile = open(arquivo_csv, 'w', newline='')
writer = csv.writer(csvfile)
writer.writerow(['tempo_ms', 'rpm', 'omega_rad_s'])
print(f"✓ Arquivo CSV criado: {arquivo_csv}\n")

fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 6))
fig.suptitle('Velocidade Angular — Sensor Hall')

dados_recebidos = {'count': 0, 'erros': 0}

def atualiza(frame):
    while ser.in_waiting:
        try:
            linha = ser.readline().decode('utf-8').strip()
            if not linha:
                continue
            
            partes = linha.split(',')
            if len(partes) == 3:
                t     = float(partes[0])
                rpm   = float(partes[1])
                omega = float(partes[2])

                tempo_data.append(t)
                rpm_data.append(rpm)
                omega_data.append(omega)

                writer.writerow([t, rpm, omega])
                csvfile.flush()
                
                dados_recebidos['count'] += 1
                if dados_recebidos['count'] % 10 == 0:
                    print(f"  {dados_recebidos['count']} pontos adquiridos")
            else:
                dados_recebidos['erros'] += 1
                if dados_recebidos['erros'] <= 3:
                    print(f"  Formato inválido: {linha}")
        except ValueError as e:
            dados_recebidos['erros'] += 1
            if dados_recebidos['erros'] <= 3:
                print(f"  Erro ao converter: {e}")
        except Exception as e:
            dados_recebidos['erros'] += 1
            if dados_recebidos['erros'] <= 3:
                print(f"  Erro inesperado: {e}")

    ax1.clear()
    ax2.clear()

    if tempo_data:
        ax1.plot(tempo_data, rpm_data, color='royalblue', linewidth=1.5)
        ax1.set_ylabel('RPM')
        ax1.set_ylim(bottom=0)
        ax1.grid(True, alpha=0.4)
        ax1.set_title('Rotações por Minuto')

        ax2.plot(tempo_data, omega_data, color='tomato', linewidth=1.5)
        ax2.set_ylabel('ω (rad/s)')
        ax2.set_xlabel('Tempo (ms)')
        ax2.set_ylim(bottom=0)
        ax2.grid(True, alpha=0.4)
        ax2.set_title('Velocidade Angular')

    plt.tight_layout()

try:
    print("\n" + "="*60)
    print("→ INICIANDO AQUISIÇÃO DE DADOS")
    print("="*60)
    print("  Aguardando dados na porta serial...")
    print("  (Pressione Ctrl+C para parar)\n")
    
    ani = animation.FuncAnimation(fig, atualiza, interval=50, cache_frame_data=False)
    plt.show()
except KeyboardInterrupt:
    print("\n\n✓ Parado pelo usuário (Ctrl+C pressionado)")
except Exception as e:
    print(f"\n✗ Erro na animação: {e}")
finally:
    ser.close()
    csvfile.close()
    
    print("\n" + "="*60)
    print("RESUMO DA AQUISIÇÃO")
    print("="*60)
    print(f"  Dados adquiridos: {dados_recebidos['count']} pontos")
    print(f"  Erros de leitura: {dados_recebidos['erros']}")
    print(f"  Arquivo salvo: {arquivo_csv}")
    print("="*60)
    
    if dados_recebidos['count'] == 0:
        print("\n  ⚠ ATENÇÃO: Nenhum dado foi recebido!")
        print("     Verifique:")
        print("     - Se o microcontroller está enviando dados")
        print("     - Se a porta COM3 está correta")
        print("     - Se o baudrate é 9600")
        print("     - Execute 'teste_porta_serial.py' para diagnosticar")
    else:
        print(f"\n  ✓ Aquisição bem-sucedida!")
        print(f"    Dados salvos em: {arquivo_csv}")