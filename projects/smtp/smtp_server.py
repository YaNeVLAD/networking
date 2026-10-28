import socket

def run_server():
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    try:
        server_socket.bind(('127.0.0.1', 25))
    except PermissionError:
        print("Error: Running server on port 25 requires additional permissions (sudo or admin rights).")
        return

    server_socket.listen(1)
    print("SMTP server is serving on port 25...")

    while True:
        client_sock, addr = server_socket.accept()
        print(f"\n[+] Connection made with {addr}")

        client_sock.sendall(b"220 localhost ESMTP Custom Server\r\n")

        mail_from = ""
        rcpt_to = ""
        data_mode = False
        message_data = []

        try:
            while True:
                data = client_sock.recv(1024)
                if not data:
                    break
                line = data.decode('utf-8', errors='ignore')

                if data_mode:
                    if line.strip() == '.':
                        data_mode = False
                        print("--- MSG RECEIVED ---")
                        print("".join(message_data))
                        print("--------------------")
                        client_sock.sendall(b"250 OK: message accepted for delivery\r\n")
                        message_data = []
                    else:
                        message_data.append(line)
                    continue

                command = line.strip()
                print(f"C: {command}")

                if command.upper().startswith("HELO") or command.upper().startswith("EHLO"):
                    client_sock.sendall(b"250 Hello client\r\n")
                elif command.upper().startswith("MAIL FROM:"):
                    mail_from = command[10:].strip()
                    client_sock.sendall(b"250 OK\r\n")
                elif command.upper().startswith("RCPT TO:"):
                    rcpt_to = command[8:].strip()
                    client_sock.sendall(b"250 OK\r\n")
                elif command.upper() == "DATA":
                    data_mode = True
                    client_sock.sendall(b"354 Start mail input; end with <CRLF>.<CRLF>\r\n")
                elif command.upper() == "QUIT":
                    client_sock.sendall(b"221 Bye\r\n")
                    break
                else:
                    client_sock.sendall(b"500 Command unrecognized\r\n")
        except Exception as e:
            print(f"Error: {e}")
        finally:
            client_sock.close()
            print(f"[-] Connection closed with {addr}")

if __name__ == "__main__":
    run_server()