import socket

# Server connection details
SERVER_ADDRESS = "127.0.0.1"
SERVER_PORT = 8080


def main():
    client_socket = None

    try:
        client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client_socket.connect((SERVER_ADDRESS, SERVER_PORT))

        print("========================================")
        print("       PYTHON TCP CLIENT")
        print("========================================")
        print()

        print(f"Connected to : {SERVER_ADDRESS}:{SERVER_PORT}")
        print()

        first_number = int(input("First Number : "))
        second_number = int(input("Second Number: "))

        message = f"{first_number} {second_number}"

        print()
        print(f"Sending      : {message}")

        client_socket.sendall(message.encode("utf-8"))

        response = client_socket.recv(1024).decode("utf-8")

        print()
        print(f"Server Result: {response}")
        print()
        print("Connection closed.")
        print("========================================")

    except ValueError:
        print("\nERROR: Please enter valid integers only.")

    except ConnectionRefusedError:
        print(f"\nERROR: Cannot connect to {SERVER_ADDRESS}:{SERVER_PORT}")

    except socket.gaierror:
        print("\nERROR: Invalid server address.")

    except socket.error as e:
        print(f"\nERROR: {e}")
        
    finally:
        # Always close the socket before exiting
        if client_socket is not None:
            client_socket.close()


if __name__ == "__main__":
    main()
