/***************************************************************************
 * 
 * Copyright (c) 2014 Baidu.com, Inc. All Rights Reserved
 * $Id$ 
 * 
 **************************************************************************/
 
 /**
 * @file test_sio_stream_client.cpp
 * @author liangdong(liangdong01@baidu.com)
 * @date 2014/03/31 12:56:05
 * @version $Revision$ 
 * @brief 
 *  
 **/

#include <iostream>
#include <string>
#include <vector>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include "sio.h"
#include "sio_stream.h"

class SioConnection
{
public:
    SioConnection(int id, struct sio* sio, const char* ipv4, uint16_t port) :
        _id(id),
        _sio(sio), 
        _stream(NULL),
        _ipv4(ipv4),
        _port(port)
    {
    }
    ~SioConnection()
    {
        sio_stop_timer(_sio, &_timer);
        if (_stream)
        {
            sio_stream_close(_sio, _stream);
        }
    }
    void launch()
    {
        sio_start_timer(_sio, &_timer, 1000, on_timer, this);
    }
    void on_data()
    {
        struct sio_buffer* buffer = sio_stream_buffer(_stream);
        char *data;
        uint64_t size;
        sio_buffer_data(buffer, &data, &size);
        std::cout << "id:" << _id << " data size:" << size << std::endl;
        if (sio_stream_write(_sio, _stream, data, size) == -1) 
        {
            std::cout << "id:" << _id << " sio_write:-1" << std::endl;
            close();
        } 
        else
        {
            sio_buffer_erase(buffer, size);
        }
    }
    void on_error()
    {
        std::cout << "id:" << _id << " on_error" << std::endl;
        close();
    }
    void on_close()
    {
        std::cout << "id:" << _id << "on_close" << std::endl;
        close();
    }
    void close()
    {
        sio_stream_close(_sio, _stream);
        _stream = NULL;
    }
private:
    static void on_timer(struct sio* sio, struct sio_timer* timer, void* arg)
    {
        SioConnection* conn = static_cast<SioConnection*>(arg);
        conn->check_connection_status();
    }
    static void on_event(struct sio* sio, struct sio_stream* stream, enum sio_stream_event event, void* arg)
    {
        SioConnection* conn = static_cast<SioConnection*>(arg);
        switch (event)
        {
            case SIO_STREAM_DATA:
                conn->on_data();
                break;
            case SIO_STREAM_ERROR:
                conn->on_error();
                break;
            case SIO_STREAM_CLOSE:
                conn->on_close();
                break;
            default:
                abort();
        }
    }
    void check_connection_status()
    {
        if (!_stream)
        {
            _stream = sio_stream_connect(_sio, _ipv4.c_str(), _port, on_event, this);
            if (_stream)
            {
                if (sio_stream_write(_sio, _stream, "ping\n", 5) == -1)
                {
                    sio_stream_close(_sio, _stream);
                    _stream = NULL;
                } 
                else
                {
                    std::cout << "stream_pending:" << sio_stream_pending(_stream) << std::endl;
                }
            }
            else
            {
                std::cout << "id:" << _id << " sio_connect fail" << std::endl;
            }
        }
        sio_start_timer(_sio, &_timer, 1000, on_timer, this);
    }
private:
    int _id;
    struct sio_timer _timer;
    struct sio* _sio;
    struct sio_stream* _stream;
    std::string _ipv4;
    uint16_t _port;
};

class SioConnManager
{
public:
    static void on_shut(int signo)
    {
        _s_shut = true;
    }
    bool start_bomb(const char* ipv4, uint16_t port, int concurrency)
    {
        struct sigaction act;
        memset(&act, 0, sizeof(act));
        act.sa_handler = on_shut;
        sigaction(SIGINT, &act, NULL);
        sigaction(SIGTERM, &act, NULL);

        _sio = sio_new();
        if (!_sio)
        {
            return false;
        }
        for (int i = 0; i < concurrency; ++i)
        {
            SioConnection* conn = new SioConnection(i, _sio, ipv4, port);
            conn->launch();
            _connections.push_back(conn);
        }
        return true;
    }
    void stop_bomb()
    {
        for (uint32_t i = 0; i < _connections.size(); ++i)
        {
            SioConnection* conn = _connections[i];
            delete conn;
        }
        sio_free(_sio);
    }
    void run_bomb()
    {
        while (!_s_shut)
        {
            sio_run(_sio, 1000);
        }
    }
private:
    static bool _s_shut;
    struct sio* _sio;
    std::vector<SioConnection*> _connections;
};

bool SioConnManager::_s_shut = false;

int main(int argc, char **argv)
{
    SioConnManager mgr;
    if (!mgr.start_bomb("127.0.0.1", 8989, 1000))
    {
        std::cout << "SioConnManager starts fail" << std::endl;
        return -1;
    }
    mgr.run_bomb();
    mgr.stop_bomb();
    std::cout << "SioConnManager stops" << std::endl;
    return 0;
}

/* vim: set ts=4 sw=4 sts=4 tw=100 */
