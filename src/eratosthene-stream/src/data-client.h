#ifndef ERATOSTHENE_STREAM_DATA_CLIENT_H
#define ERATOSTHENE_STREAM_DATA_CLIENT_H

#include <eratosthene-client.h>

class DataClient {
public:
    DataClient(unsigned char * const data_server_ip, int data_server_port);
    ~DataClient();

private:
    le_sock_t cl_socket; // Socket toward remote server - main connection
    le_size_t cl_scfg; // Remote server spatial configuration parameter
    le_time_t cl_tcfg; // Remote server temporal configuration parameter
};

#endif
