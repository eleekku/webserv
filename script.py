import socket
import threading

def send_request():
    """Función para crear y enviar una solicitud HTTP al servidor."""
    try:
        with socket.create_connection(("127.0.0.1", 8080)) as sock:
            # Construir una solicitud HTTP simple
            request = "GET / HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n"
            sock.sendall(request.encode())
            # Recibir la respuesta del servidor
            response = sock.recv(4096)
            print("Response received:")
            print(response.decode())
    except Exception as e:
        print(f"Connection failed: {e}")

def test_multiple_connections(num_connections=10):
    """Función para probar múltiples conexiones simultáneamente."""
    threads = []

    for _ in range(num_connections):
        thread = threading.Thread(target=send_request)
        threads.append(thread)
        thread.start()

    # Esperar a que todos los hilos terminen
    for thread in threads:
        thread.join()

if __name__ == "__main__":
    # Cambia el número de conexiones según sea necesario
    test_multiple_connections(num_connections=10)