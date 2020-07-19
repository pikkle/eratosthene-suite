#include <stdexcept>
#include <iostream>
#include "data-client.h"

#include "../../eratosthene-client/src/eratosthene-client.h"

DataClient::DataClient(unsigned char *const data_server_ip, int data_server_port) {
    if ((cl_socket = le_client_create(data_server_ip, data_server_port)) == _LE_SOCK_NULL) {
        throw std::runtime_error("Error while opening socket to the data server");
    }

    // @TODO loop updating model data
}

DataClient::~DataClient() {
    // @TODO close socket
}
