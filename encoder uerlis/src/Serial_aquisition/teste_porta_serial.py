import serial
import serial.tools.list_ports

print("=" * 60)
print("TESTE DE PORTA SERIAL")
print("=" * 60)

# 1. Listar todas as portas disponíveis
print("\n1. Portas seriais disponíveis:")
portas = list(serial.tools.list_ports.comports())

if not portas:
    print("   ⚠ Nenhuma porta serial encontrada!")
else:
    for porta in portas:
        print(f"   ✓ {porta.device} - {porta.description}")

# 2. Tentar conectar em COM3
print("\n2. Tentando conectar em COM3...")
try:
    ser = serial.Serial('COM3', 9600, timeout=1)
    print("   ✓ Conectado com sucesso!")
    
    # 3. Ler dados
    print("\n3. Aguardando dados (10 segundos)...")
    print("   Dados recebidos:")
    
    import time
    inicio = time.time()
    linhas_recebidas = 0
    
    while time.time() - inicio < 10:
        if ser.in_waiting:
            linha = ser.readline().decode('utf-8').strip()
            if linha:
                print(f"      {linha}")
                linhas_recebidas += 1
    
    print(f"\n   Total de linhas recebidas: {linhas_recebidas}")
    
    if linhas_recebidas == 0:
        print("   ⚠ Nenhum dado recebido!")
        print("   Verifique se:")
        print("      - O microcontroller está ligado")
        print("      - Os cabos USB estão conectados")
        print("      - O baudrate está correto (9600)")
    
    ser.close()
    print("\n✓ Conexão fechada")
    
except serial.SerialException as e:
    print(f"   ✗ Erro: {e}")
    print("   Verifique se o dispositivo está conectado em COM3")

print("\n" + "=" * 60)
