#include <stdexcept>
#include <iostream>

#include "data-client.h"

DataClient::DataClient(unsigned char *const data_server_ip, int data_server_port) {
    if ((cl_socket = le_client_create(data_server_ip, data_server_port)) == _LE_SOCK_NULL) {
        throw std::runtime_error("Error while opening socket to the data server");
    }
    set_server();

    // @TODO loop updating model data
}

DataClient::~DataClient() {
    // @TODO close socket
}

void DataClient::set_server() {
    le_array_t er_array = LE_ARRAY_C;
    le_enum_t er_message = _LE_FALSE;
    le_array_set_size( & er_array, 0 );
    if ( le_array_io_write( & er_array, LE_MODE_AUTH, cl_socket ) == LE_MODE_AUTH ) {
        if ( le_array_io_read( & er_array, cl_socket ) == LE_MODE_AUTH ) {
            if ( le_array_get_size( & er_array ) == LE_ARRAY_AUTH ) {
                er_message = _LE_TRUE;
                le_array_serial( & er_array, & cl_scfg, sizeof( le_size_t ), 0, _LE_GET );
                le_array_serial( & er_array, & cl_tcfg, sizeof( le_time_t ), sizeof( le_size_t ), _LE_GET );
            }
        }
    }
    le_array_delete( & er_array );
    if (!er_message) throw std::runtime_error("Error fetching the server configuration");

}
