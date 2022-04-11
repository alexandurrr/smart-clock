#include "mbed.h"

nsapi_size_or_error_t send_request(TLSSocket *socket, const char *request);
nsapi_size_or_error_t read_response(TLSSocket *socket, char buffer[], int buffer_length);