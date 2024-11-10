import tkinter as tk
from tkinter import ttk, messagebox
import serial
import time

# Variables globales
ser = None
estado_bomba = "Desactivada"
estado_ventana = "Desconocido"


# Función para conectarse al ESP32 a través del puerto serial
def conectar_serial():
    global ser
    puerto = puerto_combobox.get()
    if not puerto or puerto == "Seleccionar puerto":
        messagebox.showwarning("Advertencia", "Seleccione un puerto válido")
        return
    try:
        ser = serial.Serial(puerto, 115200, timeout=1)  # Ajustar baud rate
        time.sleep(2)  # Espera para inicialización
        label_estado.config(text="Conectado al ESP32", fg="green", font=("Helvetica", 12))
        print("Conectado al ESP32")
    except Exception as e:
        label_estado.config(text="Error de conexión", fg="red", font=("Helvetica", 12))
        print("Error al conectar:", e)


# Función para desconectar el puerto serial
def desconectar_serial():
    global ser
    if ser and ser.is_open:
        ser.close()
        label_estado.config(text="Desconectado", fg="red")
        print("Desconectado del ESP32")


# Función para enviar comandos al ESP32
def enviar_comando(comando):
    if ser and ser.is_open:
        try:
            ser.write(comando)
        except Exception as e:
            print(f"Error al enviar comando: {e}")
    else:
        print("No hay conexión establecida")


# Funciones para controlar el riego
def activar_riego():
    enviar_comando(b'B')
    global estado_bomba
    estado_bomba = "Activa"
    etiqueta_estado_bomba.config(text=f"Bomba: {estado_bomba}")


def desactivar_riego():
    enviar_comando(b'b')
    global estado_bomba
    estado_bomba = "Desactivada"
    etiqueta_estado_bomba.config(text=f"Bomba: {estado_bomba}")


# Función para obtener datos de temperatura y humedad
def obtener_datos():
    if ser and ser.is_open:
        enviar_comando(b'G')  # Comando para obtener datos
        time.sleep(1)         # Esperar respuesta
        if ser.in_waiting > 0:
            data = ser.readline().decode('utf-8').strip()
            label_datos.config(text=f"Datos recibidos: {data}")
            print(f"Datos obtenidos: {data}")
        else:
            label_datos.config(text="No se recibieron datos")
    else:
        messagebox.showwarning("Advertencia", "No hay conexión con el ESP32")


def obtener_estado_ventana():
    if ser and ser.is_open:
        ser.write(b'LEER_ESTADO_VENTANA\n')  # Enviar comando para leer el estado de la ventana
        time.sleep(1)                        # Esperar respuesta
        if ser.in_waiting > 0:
            estado = ser.readline().decode('utf-8').strip()  # Leer estado
            label_estado_ventana.config(text=f"Estado de la Ventana: {estado}")
            print(f"Estado de la Ventana: {estado}")
        else:
            label_estado_ventana.config(text="No se recibió respuesta")
    else:
        messagebox.showwarning("Advertencia", "No hay conexión con el ESP32")


# Funciones para controlar la minibomba
def start_pump():
    if ser and ser.is_open:
        ser.write(b"START\n")
        print("Bomba Encendida")


def stop_pump():
    if ser and ser.is_open:
        ser.write(b"STOP\n")
        print("Bomba Apagada")


# Crear la ventana principal
root = tk.Tk()
root.title("Control de Minibomba y Ventana - ESP32")

# Configurar la interfaz gráfica
frame_conexion = tk.Frame(root)
frame_conexion.pack(pady=10)

# Combobox para seleccionar el puerto serial
label_puerto = tk.Label(frame_conexion, text="Seleccionar Puerto Serial:")
label_puerto.pack(side=tk.LEFT)
puerto_combobox = ttk.Combobox(frame_conexion, values=["COM3", "COM4", "COM5"])
puerto_combobox.set("Seleccionar puerto")
puerto_combobox.pack(side=tk.LEFT)

# Botones para conectar y desconectar
btn_conectar = tk.Button(frame_conexion, text="Conectar", command=conectar_serial)
btn_conectar.pack(side=tk.LEFT, padx=5)

btn_desconectar = tk.Button(frame_conexion, text="Desconectar", command=desconectar_serial)
btn_desconectar.pack(side=tk.LEFT)

# Estado de conexión
label_estado = tk.Label(root, text="Desconectado", fg="red", font=("Helvetica", 12))
label_estado.pack(pady=5)


# Botón para obtener datos de temperatura y humedad
btn_datos = tk.Button(root, text="Obtener Datos", command=obtener_datos)
btn_datos.pack(pady=10)

# Etiqueta para mostrar los datos de temperatura y humedad
label_datos = tk.Label(root, text="Temperatura y Humedad: N/A", font=("Helvica", 12))
label_datos.pack(pady=10)

# Etiqueta para el estado de la bomba
etiqueta_estado_bomba = tk.Label(root, text=f"Bomba: {estado_bomba}", font=("Helvetica", 12))
etiqueta_estado_bomba.pack(pady=5)

# Funciones para controlar la minibomba
bomba_frame = tk.Frame(root)
bomba_frame.pack(pady=10)

start_button = tk.Button(bomba_frame, text="Encender Bomba", command=start_pump, height=2, width=20)
start_button.grid(row=0, column=0, padx=5)

stop_button = tk.Button(bomba_frame, text="Apagar Bomba", command=stop_pump, height=2, width=20)
stop_button.grid(row=0, column=1, padx=5)

# Botón para obtener el estado de la ventana
btn_estado_ventana = tk.Button(root, text="Obtener Estado de Ventana", command=obtener_estado_ventana)
btn_estado_ventana.pack(pady=10)

# Etiqueta para mostrar el estado de la ventana
label_estado_ventana = tk.Label(root, text="Estado de la Ventana: Desconocido", font=("Helvica", 12))
label_estado_ventana.pack(pady=10)

# Ejecutar la interfaz gráfica
root.mainloop()

# Cerrar la conexión serial cuando termine
if ser and ser.is_open:
    ser.close()
