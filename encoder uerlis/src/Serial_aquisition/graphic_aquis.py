import serial
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from collections import deque
import csv
from datetime import datetime
import sys
import time
import threading
import re

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
    ser = serial.Serial(PORTA, BAUD, timeout=0)
    print(f"\n✓ CONECTADO na porta {PORTA} com baudrate {BAUD}")
    time.sleep(1)  # Aguarda inicialização
    start_time = time.time()
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
setpoint_data = deque(maxlen=MAX_PONTOS)
setpoint_lock = threading.Lock()
current_setpoint = None

# salva CSV automaticamente (tempo em segundos)
arquivo_csv = f"medicao_{datetime.now().strftime('%H%M%S')}.csv"
csvfile = open(arquivo_csv, 'w', newline='')
writer = csv.writer(csvfile)
writer.writerow(['tempo_s', 'rpm', 'omega_rad_s', 'setpoint'])
print(f"✓ Arquivo CSV criado: {arquivo_csv}\n")

fig, (ax1, ax2, ax3) = plt.subplots(3, 1, figsize=(10, 8), sharex=True)
fig.suptitle('Velocidade Angular — Sensor Hall')

ax1.set_ylabel('RPM')
ax1.set_ylim(bottom=0)
ax1.grid(True, alpha=0.4)
ax1.set_title('Rotações por Minuto')

ax2.set_ylabel('ω (rad/s)')
ax2.set_ylim(bottom=0)
ax2.grid(True, alpha=0.4)
ax2.set_title('Velocidade Angular')

ax3.set_ylabel('Setpoint')
ax3.set_xlabel('Tempo (s)')
ax3.set_title('Setpoint Enviado')
ax3.grid(True, alpha=0.4)

line_rpm, = ax1.plot([], [], color='royalblue', linewidth=1.5)
line_omega, = ax2.plot([], [], color='tomato', linewidth=1.5)
line_setpoint, = ax3.plot([], [], color='mediumseagreen', linewidth=1.5, linestyle='-', drawstyle='steps-post')

dados_recebidos = {'count': 0, 'erros': 0}
last_recv_ts = None
last_zero_appended = 0.0
zero_interval = 0.2  # segundos
recv_buffer = bytearray()

# controle de threads para envio de comandos
stop_event = threading.Event()

def sender(ser, stop_event):
    global current_setpoint
    """Thread que recebe comandos do console e os envia pela serial."""
    print("\nDigita um comando e press Enter para enviar. Digite 'exit' para sair do console de envio.")
    while not stop_event.is_set():
        try:
            cmd = input()
        except EOFError:
            break
        except Exception:
            break

        if cmd is None:
            continue
        cmd = cmd.strip()
        if not cmd:
            continue
        if cmd.lower() == 'exit':
            stop_event.set()
            break
        try:
            ser.write((cmd + '\n').encode('utf-8'))
            print(f"> enviado: {cmd}")
            try:
                set_value = float(cmd)
            except ValueError:
                set_value = None
            if set_value is not None:
                with setpoint_lock:
                    current_setpoint = set_value
        except Exception as e:
            print(f"Erro ao enviar: {e}")
            stop_event.set()
            break

def atualiza(frame):
    global last_recv_ts, last_zero_appended, recv_buffer
    # leitura não bloqueante da serial para evitar delay de timeout
    max_per_frame = 50
    data = ser.read(ser.in_waiting or 1)
    if data:
        recv_buffer.extend(data)
    lines = recv_buffer.split(b'\n')
    recv_buffer = lines.pop() if lines else bytearray()

    for raw_line in lines:
        if max_per_frame <= 0:
            break
        linha = raw_line.decode('utf-8', errors='ignore').strip()
        if not linha:
            continue
        max_per_frame -= 1

        partes = linha.split(',')
        if len(partes) == 3:
            try:
                t_raw = float(partes[0])
                rpm = float(partes[1])
                omega = float(partes[2])
                # converte para segundos se foi passado em ms
                t = t_raw / 1000.0 if t_raw > 1000.0 else t_raw
            except ValueError:
                dados_recebidos['erros'] += 1
                if dados_recebidos['erros'] <= 3:
                    print(f"  Erro ao converter CSV: {linha}")
                continue
        else:
            # tenta reconhecer formatos legíveis como: "ω: 52.36 rad/s | RPM: 500"
            m_omega = re.search(r'ω\s*[:=]?\s*([-+]?\d*\.?\d+(?:[eE][-+]?\d+)?)', linha)
            if not m_omega:
                m_omega = re.search(r'omega\s*[:=]?\s*([-+]?\d*\.?\d+(?:[eE][-+]?\d+)?)', linha, re.I)
            m_rpm = re.search(r'RPM\s*[:=]?\s*([-+]?\d*\.?\d+(?:[eE][-+]?\d+)?)', linha, re.I)

            if m_omega and m_rpm:
                try:
                    omega = float(m_omega.group(1))
                    rpm = float(m_rpm.group(1))
                    t = time.time() - start_time
                except ValueError:
                    dados_recebidos['erros'] += 1
                    if dados_recebidos['erros'] <= 3:
                        print(f"  Erro ao converter valores: {linha}")
                    continue
            else:
                nums = re.findall(r'[-+]?\d*\.?\d+(?:[eE][-+]?\d+)?', linha)
                if len(nums) == 2:
                    try:
                        a = float(nums[0])
                        b = float(nums[1])
                    except ValueError:
                        dados_recebidos['erros'] += 1
                        continue
                    lowline = linha.lower()
                    if 'rad' in lowline or 'ω' in lowline or 'omega' in lowline:
                        omega = a; rpm = b
                    elif 'rpm' in lowline:
                        rpm = a; omega = b
                    else:
                        omega = a; rpm = b
                    t = time.time() - start_time
                elif len(nums) == 1:
                    # linha com um único número — provavelmente eco de comando ou valor isolado; ignore
                    continue
                else:
                    dados_recebidos['erros'] += 1
                    if dados_recebidos['erros'] <= 3:
                        print(f"  Formato inválido: {linha}")
                    continue
        # grava dados (tempo em segundos)
        tempo_data.append(t)
        rpm_data.append(rpm)
        omega_data.append(omega)
        with setpoint_lock:
            setpoint_data.append(current_setpoint if current_setpoint is not None else 0.0)
            writer.writerow([t, rpm, omega, current_setpoint if current_setpoint is not None else ''])
            csvfile.flush()
        last_recv_ts = time.time()

        dados_recebidos['count'] += 1
        if dados_recebidos['count'] % 10 == 0:
            print(f"  {dados_recebidos['count']} pontos adquiridos")

    now = time.time()
    if last_recv_ts is None or (now - last_recv_ts) > zero_interval:
        if now - last_zero_appended > zero_interval:
            t = now - start_time
            tempo_data.append(t)
            rpm_data.append(0.0)
            omega_data.append(0.0)
            with setpoint_lock:
                setpoint_data.append(current_setpoint if current_setpoint is not None else 0.0)
                writer.writerow([t, 0.0, 0.0, current_setpoint if current_setpoint is not None else ''])
                csvfile.flush()
            last_recv_ts = now
            last_zero_appended = now

    if tempo_data:
        x_min = tempo_data[0]
        x_max = tempo_data[-1]

        ax1.set_xlim(x_min, x_max)
        ax2.set_xlim(x_min, x_max)
        ax3.set_xlim(x_min, x_max)

        rpm_max = max(rpm_data) if rpm_data else 1.0
        omega_max = max(omega_data) if omega_data else 1.0
        setpoint_min = min(setpoint_data) if setpoint_data else 0.0
        setpoint_max = max(setpoint_data) if setpoint_data else 1.0

        ax1.set_ylim(0, rpm_max * 1.1 if rpm_max > 0 else 1.0)
        ax2.set_ylim(0, omega_max * 1.1 if omega_max > 0 else 1.0)
        ax3.set_ylim(setpoint_min - abs(setpoint_min) * 0.1 - 0.1, setpoint_max + abs(setpoint_max) * 0.1 + 0.1)

        line_rpm.set_data(tempo_data, rpm_data)
        line_omega.set_data(tempo_data, omega_data)
        line_setpoint.set_data(tempo_data, setpoint_data)

    plt.tight_layout(rect=[0, 0, 1, 0.96])

try:
    print("\n" + "="*60)
    print("→ INICIANDO AQUISIÇÃO DE DADOS")
    print("="*60)
    print("  Aguardando dados na porta serial...")
    print("  (Pressione Ctrl+C para parar)\n")
    
    # inicia thread de envio (entrada do console -> serial)
    sender_thread = threading.Thread(target=sender, args=(ser, stop_event), daemon=True)
    sender_thread.start()

    ani = animation.FuncAnimation(fig, atualiza, interval=25, cache_frame_data=False)
    plt.show()
except KeyboardInterrupt:
    print("\n\n✓ Parado pelo usuário (Ctrl+C pressionado)")
except Exception as e:
    print(f"\n✗ Erro na animação: {e}")
finally:
    # sinaliza threads para encerrar
    stop_event.set()
    try:
        sender_thread.join(timeout=1)
    except Exception:
        pass
    try:
        ser.close()
    except Exception:
        pass
    try:
        csvfile.close()
    except Exception:
        pass
    
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