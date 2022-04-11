#include "mbed.h"

nsapi_size_or_error_t send_request(TLSSocket *socket, const char *request) {
    // The request might not be fully sent in one go, so keep track of how much we have sent
    nsapi_size_t bytes_to_send = strlen(request);
    nsapi_size_or_error_t bytes_sent = 0;

    printf("Sending message: \n%s", request);

    // Loop as long as there are more data to send
    while (bytes_to_send) {
        // Try to send the remaining data. send() returns how many bytes were actually sent
        bytes_sent = socket->send(request + bytes_sent, bytes_to_send);

        // Negative return values from send() are errors
        if (bytes_sent < 0) {
            return bytes_sent;
        } else {
            printf("sent %d bytes\n", bytes_sent);
        }

        bytes_to_send -= bytes_sent;
    }

    printf("Complete message sent\n");

    return 0;
}


nsapi_size_or_error_t read_response(TLSSocket *socket, char buffer[], int buffer_length) {
    memset(buffer, 0, buffer_length);

    int remaining_bytes = buffer_length;
    int received_bytes = 0;

    // Loop as long as there are more data to read. We might not read all in one call to recv()
    while (remaining_bytes > 0) {
        nsapi_size_or_error_t result = socket->recv(buffer + received_bytes, remaining_bytes);

        // If the result is 0 there are no more bytes to read
        if (result == 0) {
            break;
        }

        // Negative return values from recv() are errors
        if (result < 0) {
            return result;
        }

        received_bytes += result;
        remaining_bytes -= result;
    }

    printf("received %d bytes:\n%.*s\n", received_bytes,
                 strstr(buffer, "\n") - buffer, buffer);

    return 0;
}