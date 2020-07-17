#include "data-client.h"

DataClient::DataClient() {
    // @TODO use preexisting code from eratosthene-client program
    // @TODO loop updating model data
}

DataClient::~DataClient() {
    // @TODO close socket
}

void DataClient::set_data_server(const unsigned char *data_server_ip, int data_server_port) {
    DataClient::data_server_ip = (unsigned char *) data_server_ip;
    DataClient::data_server_port = data_server_port;
}


