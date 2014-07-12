/*
 * Copyright (C) 2014-2015  liangdong <liangdong01@baidu.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <map>
#include <iostream>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include "sio.h"    // simple_io基础事件库
#include "sio_stream.h" // simple_io TCP库

class SioServer;

class SioConnection
{
public:
    SioConnection(uint64_t id, SioServer* server, struct sio* sio, struct sio_stream* stream) :
        _id(id),
        _server(server),
        _sio(sio),
        _stream(stream)
    {
        sio_stream_set(sio, stream, on_event, this);
    }
    ~SioConnection()
    {
        sio_stream_close(_sio, _stream);
    }
    uint64_t id()
    {
        return _id;
    }
    void on_data();
    void on_error();
    void on_close();
private:
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
                abort(); // never reach
        }
    }
    uint64_t _id;
    SioServer* _server;
    struct sio* _sio;
    struct sio_stream* _stream;
    struct sio_timer _timer;
};

class SioServer
{
    typedef std::map<uint64_t, SioConnection*> ConnectionMap;
    typedef ConnectionMap::iterator ConnectionIterator;
    typedef ConnectionMap::value_type ConnectionPair;
public:
    static void on_shut(int signo)
    {
        _s_shut = true;
    }
    bool start_service(const char* ipv4, uint16_t port)
    {
        struct sigaction act;
        memset(&act, 0, sizeof(act));
        act.sa_handler = on_shut;
        sigaction(SIGINT, &act, NULL);
        sigaction(SIGTERM, &act, NULL);
        _client_id = 0;
        _sio = sio_new();
        if (!_sio)
        {
            return false;
        }
        _acceptor = sio_stream_listen(_sio, ipv4, port, SioServer::on_accept, this);
        if (!_acceptor)
        {
            sio_free(_sio);
            return false;
        }
        return true;
    }
    void stop_service()
    {
        for (ConnectionIterator beg = _connections.begin(); beg != _connections.end(); ++beg)
        {
            SioConnection* conn = beg->second;
            delete conn;
        }
        sio_stream_close(_sio, _acceptor);
        sio_free(_sio);
    }
    void close_connection(SioConnection* conn)
    {
        _connections.erase(conn->id());
        delete conn;
    }
    void run_service()
    {
        while (!_s_shut)
        {
            sio_run(_sio, 1000);
        }
    }
private:
    static void on_accept(struct sio *sio, struct sio_stream *stream, enum sio_stream_event event, void* arg)
    {
        SioServer* server = static_cast<SioServer*>(arg);
        server->accept_connection(stream);
    }
    void accept_connection(struct sio_stream* stream)
    {
        std::cout << "id:" << _client_id++ << " on_accept" << std::endl;
        SioConnection* conn = new SioConnection(_client_id, this, _sio, stream);
        _connections.insert(ConnectionPair(_client_id, conn));
    }
    static bool _s_shut;
    struct sio* _sio;
    struct sio_stream* _acceptor;
    uint64_t _client_id;
    ConnectionMap _connections;
};

bool SioServer::_s_shut = false;

void SioConnection::on_data()
{
    struct sio_buffer* buffer = sio_stream_buffer(_stream);

    char* data;
    uint64_t size;
    sio_buffer_data(buffer, &data, &size);

    std::cout << "id:" << _id << " data size:" << size << std::endl;
    if (sio_stream_write(_sio, _stream, data, size) == -1) // echo
    {
        std::cout << "id:" << _id << " sio_write:-1" << std::endl;
        _server->close_connection(this);
    } 
    else 
    {
        sio_buffer_erase(buffer, size);
    }
}
void SioConnection::on_error()
{
    std::cout << "id:" << _id << " on_error" << std::endl;
    _server->close_connection(this);
}
void SioConnection::on_close()
{
    std::cout << "id:" << _id << " on_close" << std::endl;
    _server->close_connection(this);
}

int main(int argc, char **argv)
{
    SioServer server;
    if (!server.start_service("0.0.0.0", 8989))
    {
        std::cout << "start SioServer fail" << std::endl;
        return -1;
    }
    server.run_service();
    server.stop_service();
    std::cout << "SioServer stops" << std::endl;
    return 0;
}

/* vim: set ts=4 sw=4 sts=4 tw=100 */
