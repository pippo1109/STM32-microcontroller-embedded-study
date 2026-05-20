import serial
import serial.tools.list_ports
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from collections import deque
import csv
from datetime import datetime
import sys
import time
import threading
import re

# Listar portas disponíveis
ports = list(serial.tools.list_ports.comports())

# Tenta usar COM14 ou seleciona primeira disponível
PORTA = 'COM14'
if not any(p.device == PORTA for p in ports):
    if ports:
        PORTA = ports[0].device
    else:
        print("Erro: Nenhuma porta serial disponível!")
        sys.exit(1)

BAUD  = 9600
MAX_PONTOS = 1000

try:
    ser = serial.Serial(PORTA, BAUD, timeout=1)
    time.sleep(1)
    start_time = time.time()
except serial.SerialException as e:
    print(f"Erro: Não conectou em {PORTA}")
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
writer.writerow(['tempo_s', 'omega_rad_s', 'rpm', 'setpoint'])

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
        
        # Validar entrada
        try:
            set_value = float(cmd)
            if 0 <= set_value <= 100:
                ser.write((cmd + '\n').encode('utf-8'))
                with setpoint_lock:
                    current_setpoint = set_value
            else:
                print(f"Valor inválido (0-100)")
        except ValueError:
            print(f"Erro: Digite um número")
        except Exception as e:
            print(f"Erro: {e}")

def atualiza(frame):
    global last_recv_ts, last_zero_appended, recv_buffer
    # leitura não bloqueante da serial para evitar delay de timeout
    max_per_frame = 50
    
    try:
        data = ser.read(ser.in_waiting or 1)
        if data:
            recv_buffer.extend(data)
    except Exception:
        return
    
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
                    continue
            else:
                nums = re.findall(r'[-+]?\d*\.?\d+(?:[eE][-+]?\d+)?', linha)
                if len(nums) == 2:
                    try:
                        val1 = float(nums[0])
                        val2 = float(nums[1])
                    except ValueError:
                        dados_recebidos['erros'] += 1
                        continue
                    lowline = linha.lower()
                    if 'rad' in lowline or 'ω' in lowline or 'omega' in lowline:
                        omega = val1; rpm = val2
                    elif 'rpm' in lowline:
                        rpm = val1; omega = val2
                    else:
                        omega = val1; rpm = val2
                    t = time.time() - start_time
                elif len(nums) == 1:
                    continue
                else:
                    dados_recebidos['erros'] += 1
                    continue
        # grava dados (tempo em segundos)
        tempo_data.append(t)
        rpm_data.append(rpm)
        omega_data.append(omega)
        with setpoint_lock:
            setpoint_data.append(current_setpoint if current_setpoint is not None else 0.0)
            writer.writerow([t, omega, rpm, current_setpoint if current_setpoint is not None else ''])
            csvfile.flush()
        last_recv_ts = time.time()

        dados_recebidos['count'] += 1

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
    print(f"Porta: {PORTA} | Baudrate: {BAUD}")
    print("Digite valores (0-100):\n")
    
    # inicia thread de envio (entrada do console -> serial)
    sender_thread = threading.Thread(target=sender, args=(ser, stop_event), daemon=True)
    sender_thread.start()

    ani = animation.FuncAnimation(fig, atualiza, interval=25, cache_frame_data=False)
    plt.show()
except KeyboardInterrupt:
    pass
except Exception as e:
    print(f"Erro: {e}")
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
    
    if dados_recebidos['count'] > 0:
        print(f"\nArquivo: {arquivo_csv}")