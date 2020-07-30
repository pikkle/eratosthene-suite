#include <stdexcept>
#include <iostream>

#include "data-client.h"

DataClient::DataClient(unsigned char *const data_server_ip, int data_server_port) {
    // open socket to data server
    dc_socket = le_client_create(data_server_ip, data_server_port);
    if (dc_socket == _LE_SOCK_NULL) {
        throw std::runtime_error("Error while opening socket to the data server");
    }

     // setup server configuration
    set_server();

    dc_push = ER_VIEW_C;

    // create client model
    cl_model = std::make_shared<er_model_t>(er_model_create(dc_socket, dc_scfg, dc_tcfg));
    if (!le_get_status(*cl_model)) {
        throw std::runtime_error("Error while creating the data model");
    }

}

DataClient::~DataClient() {
    // send termination signal
    le_array_t er_array = LE_ARRAY_C;
    le_array_set_size(&er_array, 0);
    le_array_io_write(&er_array, LE_MODE_NULL, dc_socket);
    le_array_delete(&er_array);

    // delete socket
    le_client_delete(dc_socket);

    // delete model
    er_model_delete(&*cl_model);
}

void DataClient::set_server() {
    le_array_t er_array = LE_ARRAY_C;
    le_enum_t er_message = _LE_FALSE;
    le_array_set_size(&er_array, 0);
    // send auth message to server to request configuration
    if (le_array_io_write(&er_array, LE_MODE_AUTH, dc_socket) == LE_MODE_AUTH) {
        // read configuration setup from data server
        if (le_array_io_read(&er_array, dc_socket) == LE_MODE_AUTH) {
            if (le_array_get_size(&er_array) == LE_ARRAY_AUTH) {
                er_message = _LE_TRUE;
                le_array_serial(&er_array, &dc_scfg, sizeof(le_size_t), 0, _LE_GET);
                le_array_serial(&er_array, &dc_tcfg, sizeof(le_time_t), sizeof(le_size_t), _LE_GET);
            }
        }
    }
    le_array_delete(&er_array);
    if (!er_message) throw std::runtime_error("Error fetching the server configuration");

}

bool DataClient::update_model(const er_view_t* view, le_size_t delay) {
    le_address_t er_address = LE_ADDRESS_C;

    /* motion detection */
    if (!er_view_get_equal(&dc_push, view)) {
        dc_push = *view;
        dc_last = clock();
    }

    if ((clock() - dc_last) > delay) {
        /* retreive address times */
        er_address = er_view_get_times(view);
        /* prepare model update */
        er_model_set_prep(&*cl_model);
        /* update model target */
        er_model_set_enum(&*cl_model, &er_address, 0, view);
        /* model/target fast synchronisation */
        er_model_set_fast(&*cl_model);
        /* target content detection */
        er_model_set_detect(&*cl_model);
        /* reset motion time */
        dc_last = _LE_TIME_MAX;
    }

    if (!er_model_get_sync( & *cl_model )) {
        /* model synchronisation process */
        er_model_set_sync( & *cl_model );
        return true;
    }

    return false;
}
