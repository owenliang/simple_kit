/***************************************************************************
 * 
 * Copyright (c) 2014 Baidu.com, Inc. All Rights Reserved
 * $Id$ 
 * 
 **************************************************************************/
 
 /**
 * @file test_sio_stream_multi_server.cpp
 * @author liangdong(liangdong01@baidu.com)
 * @date 2014/04/07 22:34:21
 * @version $Revision$ 
 * @brief 
 *  
 **/

#include <assert.h>
#include <iostream>
#include <vector>
#include <string>
#include <deque>
#include <map>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include "sio.h"
#include "sio_stream.h"

class SioThread;

class SioConnection
{
public:
    SioConnection(uint64_t id, SioThread* thread, struct sio* sio, struct sio_stream* stream) :
        _id(id),
        _thread(thread),
        _sio(sio),
        _stream(stream)
    {
        sio_stream_set(sio, stream, on_event, this);
        _last_ping = time(NULL);
        sio_start_timer(sio, &_timer, 1000, on_timer, this);
    }
    ~SioConnection()
    {
        sio_stop_timer(_sio, &_timer);
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
    void handle_timer();
    static void on_timer(struct sio* sio, struct sio_timer* timer, void* arg)
    {   
        SioConnection* conn = static_cast<SioConnection*>(arg);
        conn->handle_timer();
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
                abort(); // never reach
        }
    }
    time_t _last_ping;
    uint64_t _id;
    SioThread* _thread;
    struct sio* _sio;
    struct sio_stream* _stream;
    struct sio_timer _timer;
};

class SioMultiServer;

class SioAcceptor
{
public:
    SioAcceptor(SioMultiServer* server) :
        _server(server)
    {
    }
    bool start_acceptor(const std::string& ipv4, uint16_t port)
    {
        _sio = sio_new();
        if (!_sio)
        {
            return false;
        }
        _acceptor = sio_stream_listen(_sio, ipv4.c_str(), port, on_accept, this);
        if (!_acceptor)
        {
            sio_free(_sio);
            return false;
        }
        return true;
    }
    void stop_acceptor()
    {
        sio_stream_close(_sio, _acceptor);
        sio_free(_sio);
    }
    struct sio* get_sio()
    {
        return _sio;
    }
    void accept(struct sio_stream* stream);

private:
    static void on_accept(struct sio* sio, struct sio_stream* stream, enum sio_stream_event event, void* arg);
    
    SioMultiServer *_server;
    struct sio *_sio;
    struct sio_stream *_acceptor;
};

class SioThread
{
    typedef std::map<uint64_t, SioConnection*> ConnectionMap;
    typedef ConnectionMap::iterator ConnectionIterator;
    typedef ConnectionMap::value_type ConnectionPair;
public:
    SioThread() : 
        _id(0),
        _stop(false)
    {
        pthread_mutex_init(&_mutex, NULL);
    }
    ~SioThread()
    {
        pthread_mutex_destroy(&_mutex);
    }
    bool start_thread()
    {
        _sio = sio_new();
        if (!_sio)
        {
            return false;
        }
        pthread_create(&_tid, NULL, thread_main, this);
        return true;
    }
    void stop_thread()
    {
        _stop = true;
        pthread_join(_tid, NULL);
        while (!_new_conns.empty())
        {
            sio_stream_close(NULL, _new_conns.front());
            _new_conns.pop_front();
        }
        for (ConnectionIterator beg = _conns.begin(); beg != _conns.end(); ++beg)
        {
            SioConnection* conn = beg->second;
            delete conn;
        }
        sio_free(_sio);
    }
    void push_connection(struct sio_stream* stream)
    {
        pthread_mutex_lock(&_mutex);
        _new_conns.push_back(stream);
        pthread_mutex_unlock(&_mutex);
        sio_wakeup(_sio);
    }
    void close_connection(SioConnection* conn)
    {
        _conns.erase(conn->id());
        delete conn;
    }
private:
    void handle_new_conns()
    {
        std::deque<struct sio_stream*> q;
        pthread_mutex_lock(&_mutex);
        q.swap(_new_conns);
        pthread_mutex_unlock(&_mutex);
        while (!q.empty())
        {
            struct sio_stream* s = q.front();
            q.pop_front();
            if (sio_stream_attach(_sio, s) == -1)
            {
                sio_stream_close(NULL, s);
            }
            else
            {
                SioConnection* conn = new SioConnection(_id, this, _sio, s);
                _conns.insert(ConnectionPair(_id++, conn));
            }
        }
    }
    void main()
    {
        while (!_stop)
        {
            handle_new_conns();
            sio_run(_sio, 10);
        }
    }
    static void* thread_main(void* arg)
    {
        SioThread* t = static_cast<SioThread*>(arg);
        t->main();
        return NULL;
    }
    
    pthread_mutex_t _mutex;
    ConnectionMap _conns;
    std::deque<struct sio_stream*> _new_conns;
    
    uint64_t _id;
    volatile bool _stop;
    pthread_t _tid;
    struct sio* _sio;
};

class SioMultiServer
{
public:
    SioMultiServer() : 
        _dispatch_index(0)
    {
    }
    bool start_service(uint32_t thread_num, const std::string& ipv4, uint16_t port)
    {
        struct sigaction act;
        memset(&act, 0, sizeof(act));
        act.sa_handler = on_shut;
        sigaction(SIGINT, &act, NULL);
        sigaction(SIGTERM, &act, NULL);
        
        // 保证子线程不接收SIGINT/SIGTERM信号
        sigset_t set, oldset;
        sigfillset(&set);
        pthread_sigmask(SIG_SETMASK, &set, &oldset);

        _acceptor = new SioAcceptor(this);
        if (!_acceptor->start_acceptor(ipv4, port))
        {
            delete _acceptor;
            return false;
        }
        for (uint32_t i = 0; i < thread_num; ++i)
        {
            SioThread* t = new SioThread();
            assert(t->start_thread());
            _threads.push_back(t);
        }
        // 恢复主线程接收SIGINT/SIGTERM信号
        pthread_sigmask(SIG_SETMASK, &oldset, NULL); 
        return true;
    }
    void stop_service()
    {
        _acceptor->stop_acceptor();
        delete _acceptor;
        for (uint32_t i = 0; i < _threads.size(); ++i)
        {
            std::cerr << "closing thread " << i << std::endl;
            _threads[i]->stop_thread();
            delete _threads[i];
            std::cerr << "closed thread " << i << std::endl;
        }
        _threads.clear();
    }
    void run_service()
    {
        struct sio* sio = _acceptor->get_sio();
        while (!_s_shut)
        {
            sio_run(sio, 1000);
        }
    }
    void dispatch_connection(struct sio_stream* stream)
    {
        _threads[_dispatch_index]->push_connection(stream);
        _dispatch_index = (_dispatch_index + 1) % _threads.size();
    }
private:
    static void on_shut(int signo)
    {
        std::cerr << "on_shut" << std::endl;
        _s_shut = true;
    }
    static bool _s_shut;
    uint64_t _dispatch_index;
    SioAcceptor* _acceptor;
    std::vector<SioThread*> _threads;
};

bool SioMultiServer::_s_shut = false;

void SioAcceptor::accept(struct sio_stream* stream)
{
    sio_stream_detach(_sio, stream); // detach from current epoll
    _server->dispatch_connection(stream);
}
    
void SioAcceptor::on_accept(struct sio* sio, struct sio_stream* stream, enum sio_stream_event event, void* arg)
{
    SioAcceptor* acceptor = static_cast<SioAcceptor*>(arg);
    acceptor->accept(stream);
}

void SioConnection::on_data()
{
    _last_ping = time(NULL);

    struct sio_buffer* buffer = sio_stream_buffer(_stream);

    char* data;
    uint64_t size;
    sio_buffer_data(buffer, &data, &size);

    std::cout << "id:" << _id << " data size:" << size << std::endl;
    if (sio_stream_write(_sio, _stream, data, size) == -1) // echo
    {
        std::cout << "id:" << _id << " sio_write:-1" << std::endl;
        _thread->close_connection(this);
    } 
    else 
    {
        sio_buffer_erase(buffer, size);
    }
}
void SioConnection::on_error()
{
    std::cout << "id:" << _id << " on_error" << std::endl;
    _thread->close_connection(this);
}
void SioConnection::on_close()
{
    std::cout << "id:" << _id << " on_close" << std::endl;
    _thread->close_connection(this);
}
void SioConnection::handle_timer()
{   
    // std::cout << "fd(" << _stream->sfd->fd << ") on timer" << std::endl;
    sio_start_timer(_sio, &_timer, 1000, on_timer, this);
    if (time(NULL) - _last_ping > 5) // 5秒没ping
    {
        _thread->close_connection(this);
    }
}   

int main(int argc, char** argv)
{
    SioMultiServer server;
    if (!server.start_service(12, "0.0.0.0", 8989))
    {
        std::cerr << "start service fail" << std::endl;
        return -1;
    }
    server.run_service();
    server.stop_service();
    return 0;
}

/* vim: set ts=4 sw=4 sts=4 tw=100 */
