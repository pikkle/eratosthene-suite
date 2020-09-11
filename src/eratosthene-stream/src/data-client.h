#ifndef ERATOSTHENE_STREAM_DATA_CLIENT_H
#define ERATOSTHENE_STREAM_DATA_CLIENT_H

#include <eratosthene-array.h>
#include <eratosthene-client.h>
#include <eratosthene-client-model.h>
#include <memory>

class DataClient {
public:
    DataClient(unsigned char * const data_server_ip, int data_server_port);
    ~DataClient();
    std::shared_ptr<er_model_t> get_model() { return cl_model; }

    bool update_model(const er_view_t* view, le_size_t delay);

private:
    void set_server();

    std::shared_ptr<er_model_t> cl_model; // Model sub-module structure

    le_sock_t dc_socket; // Socket toward remote server - main connection
    le_size_t dc_scfg; // Remote server spatial configuration parameter
    le_time_t dc_tcfg; // Remote server temporal configuration parameter

    er_view_t  dc_push; // Pushed point of view
    le_time_t  dc_last; // Delayed model update clock
};

#endif
