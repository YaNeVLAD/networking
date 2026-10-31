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

        data_mode = False
        message_data = []

        try:
            while True:
                data = client_sock.recv(1024)
                if not data:
                    break

                text_chunk = data.decode('utf-8', errors='ignore')

                if data_mode:
                    lines = text_chunk.splitlines(keepends=True)
                    for line in lines:
                        if line.strip() == '.':
                            data_mode = False
                            print("--- MSG RECEIVED ---")
                            print("".join(message_data))
                            print("--------------------")
                            client_sock.sendall(b"250 OK: message accepted for delivery\r\n")
                            break
                        else:
                            message_data.append(line)
                    continue

                line = text_chunk.strip()
                if not line:
                    continue

                print(f"C: {line}")

                if line.upper().startswith("EHLO"):
                    client_sock.sendall(b"250-localhost ESMTP Custom Server\r\n250-SIZE 35600000\r\n250 OK\r\n")
                elif line.upper().startswith("HELO"):
                    client_sock.sendall(b"250 Hello client\r\n")
                elif line.upper().startswith("MAIL FROM:"):
                    client_sock.sendall(b"250 OK\r\n")
                elif line.upper().startswith("RCPT TO:"):
                    client_sock.sendall(b"250 OK\r\n")
                elif line.upper().startswith("DATA"):
                    data_mode = True
                    client_sock.sendall(b"354 Start mail input; end with <CRLF>.<CRLF>\r\n")
                elif line.upper().startswith("QUIT"):
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
