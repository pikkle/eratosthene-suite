#ifndef ERATOSTHENE_STREAM_DATA_CLIENT_H
#define ERATOSTHENE_STREAM_DATA_CLIENT_H

class DataClient {
public:
    DataClient();
    ~DataClient();

    /**
     * Sets the data server IP and port address for all video clients
     * @param data_server_ip the data server's IP
     * @param data_server_port the data server's Port number
     */
    static void set_data_server(const unsigned char * data_server_ip, int data_server_port);

private:
    static inline unsigned char * data_server_ip; // the data server IP
    static inline int data_server_port; // the data server Port
};

#endif
