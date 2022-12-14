/*
 * Copyright lizhirui
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-10-14     lizhirui     the first version
 */

#include "common.h"
#include "config.h"
#include "network/network.h"
#include "network/network_command.h"
#include "main.h"

static asio::io_context recv_ioc;
static asio::io_context send_ioc;
static std::atomic<bool> recv_thread_stop = false;
static std::atomic<bool> recv_thread_stopped = false;
static std::atomic<bool> server_thread_stopped = false;
static std::atomic<bool> program_stop = false;

static asio::io_context tcp_charfifo_thread_ioc;
static charfifo_send_fifo_t charfifo_send_fifo;
static charfifo_rev_fifo_t charfifo_rev_fifo;
static std::atomic<bool> charfifo_thread_stopped = false;
static std::atomic<bool> charfifo_recv_thread_stop = false;
static std::atomic<bool> charfifo_recv_thread_stopped = true;

static asio::io_context tcp_server_thread_ioc;

static asio::ip::tcp::socket *client_soc;

static std::unordered_map<std::string, socket_cmd_handler> socket_cmd_list;

void set_recv_thread_stop(bool value)
{
    recv_thread_stop = value;
}

bool get_recv_thread_stopped()
{
    return recv_thread_stopped;
}

void set_program_stop(bool value)
{
    program_stop = value;
}

bool get_program_stop()
{
    return program_stop;
}

bool get_server_thread_stopped()
{
    return server_thread_stopped;
}

bool get_charfifo_thread_stopped()
{
    return charfifo_thread_stopped;
}

bool get_charfifo_recv_thread_stopped()
{
    return charfifo_recv_thread_stopped;
}

charfifo_send_fifo_t *get_charfifo_send_fifo()
{
    return &charfifo_send_fifo;
}

charfifo_rev_fifo_t *get_charfifo_rev_fifo()
{
    return &charfifo_rev_fifo;
}

void recv_ioc_stop()
{
    recv_ioc.stop();
}

void debug_event_handle()
{
    try
    {
        auto guard = std::make_shared<asio::io_context::work>(recv_ioc);
        recv_ioc.run();
        recv_ioc.restart();
    }
    catch(const std::exception &ex)
    {
        std::cout << MESSAGE_OUTPUT_PREFIX << ex.what() << std::endl;
    }
}

void register_socket_cmd(std::string name, socket_cmd_handler handler)
{
    verify(socket_cmd_list.find(name) == socket_cmd_list.end());
    socket_cmd_list[name] = handler;
}

void send_cmd_result(asio::ip::tcp::socket &soc, std::string result)
{
    char *buffer = new char[result.length() + 4];
    *(uint32_t *)buffer = (uint32_t)result.length();
    memcpy(buffer + 4, result.data(), result.length());
    soc.send(asio::buffer(buffer, result.length() + 4));
    delete[] buffer;
}

void send_cmd(std::string prefix, std::string cmd, std::string arg)
{
    try
    {
        if(client_soc != nullptr)
        {
            asio::post(send_ioc, [prefix, cmd, arg]{send_cmd_result(std::ref(*client_soc), prefix + " " + cmd + " " + arg);});
        }
    }
    catch(const std::exception &e)
    {
    
    }
}

static void socket_cmd_handle(asio::ip::tcp::socket &soc, std::string rev_str)
{
    std::stringstream stream(rev_str);
    std::vector<std::string> cmd_arg_list;

    while(!stream.eof())
    {
        std::string t;
        stream >> t;
        cmd_arg_list.push_back(t);
    }

    if(cmd_arg_list.size() >= 2)
    {
        auto prefix = cmd_arg_list[0];
        auto cmd = cmd_arg_list[1];
        cmd_arg_list.erase(cmd_arg_list.begin());
        cmd_arg_list.erase(cmd_arg_list.begin());
        std::string ret = "notfound";

        if(socket_cmd_list.find(cmd) != socket_cmd_list.end())
        {
            ret = socket_cmd_list[cmd](cmd_arg_list);
        }

        asio::post(send_ioc, [&soc, prefix, cmd, ret]{send_cmd_result(soc, prefix + " " + cmd + " " + ret);});
    }
}

static void tcp_charfifo_recv_thread_entry(asio::ip::tcp::socket &soc)
{
    char buf[1024];
    std::cout << MESSAGE_OUTPUT_PREFIX << "tcp_charfifo_recv thread tid: " << gettid() << std::endl;

    while(!charfifo_recv_thread_stop)
    {
        try
        {
            auto length = soc.receive(asio::buffer(&buf, sizeof(buf)));

            for(size_t i = 0;i < length;i++)
            {
                while(!charfifo_rev_fifo.push(buf[i]));
            }
        }
        catch(const std::exception &ex)
        {
            std::cout << MESSAGE_OUTPUT_PREFIX << ex.what() << std::endl;
            break;
        }
    }

    charfifo_recv_thread_stopped = true;
}

static void tcp_charfifo_thread_entry(asio::ip::tcp::acceptor &&listener)
{
    std::cout << MESSAGE_OUTPUT_PREFIX << "tcp_charfifo thread tid: " << gettid() << std::endl;
    
    try
    {
        while(!program_stop)
        {
            std::cout << MESSAGE_OUTPUT_PREFIX "Wait Telnet Connection" << std::endl;
            auto soc = listener.accept();
            soc.set_option(asio::ip::tcp::no_delay(true));
            std::cout << MESSAGE_OUTPUT_PREFIX "Telnet Connected" << std::endl;
            charfifo_recv_thread_stop = false;
            charfifo_recv_thread_stopped = false;
            std::thread rev_thread(tcp_charfifo_recv_thread_entry, std::ref(soc));

            try
            {
                while(true)
                {
                    if(!charfifo_send_fifo.empty())
                    {
                        auto ch = charfifo_send_fifo.front();
                        soc.send(asio::buffer(&ch, 1));
                        while(!charfifo_send_fifo.pop());
                    }

                    if(charfifo_recv_thread_stopped)
                    {
                        break;
                    }

                    if(charfifo_send_fifo.empty())
                    {
                        std::this_thread::sleep_for(std::chrono::milliseconds(30));
                    }
                }
            }
            catch(const std::exception &ex)
            {
                std::cout << MESSAGE_OUTPUT_PREFIX << ex.what() << std::endl;
            }

            charfifo_recv_thread_stop = true;
            rev_thread.join();
            
            try
            {
                soc.shutdown(asio::socket_base::shutdown_both);
                soc.close();
            }
            catch(const std::exception &ex)
            {
            }
            
            std::cout << MESSAGE_OUTPUT_PREFIX "Telnet Disconnected" << std::endl;
        }

        listener.close();
        charfifo_thread_stopped = true;
    }
    catch(const std::exception &ex)
    {
        std::cout << MESSAGE_OUTPUT_PREFIX << ex.what() << std::endl;
    }
}

static void std_output_charfifo_thread_entry()
{
    std::cout << MESSAGE_OUTPUT_PREFIX << "std_output_charfifo thread tid: " << gettid() << std::endl;
    
    try
    {
        while(!program_stop)
        {
            while(true)
            {
                if(!charfifo_send_fifo.empty())
                {
                    auto ch = charfifo_send_fifo.front();
                    std::cout << ch;
                    while(!charfifo_send_fifo.pop());
                }
        
                if(charfifo_thread_stopped)
                {
                    break;
                }
        
                if(charfifo_send_fifo.empty())
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(30));
                }
            }
        }
        
        charfifo_thread_stopped = true;
    }
    catch(const std::exception &ex)
    {
        std::cout << MESSAGE_OUTPUT_PREFIX << ex.what() << std::endl;
    }
}

static void tcp_server_recv_thread_entry(asio::ip::tcp::socket &soc)
{
    uint32_t length = 0;
    size_t rev_length = 0;
    char *packet_payload = nullptr;
    char packet_length[4];
    std::string rev_str;
    std::cout << MESSAGE_OUTPUT_PREFIX << "tcp_server_recv thread tid: " << gettid() << std::endl;

    while(!recv_thread_stop)
    {
        try
        {
            if(packet_payload == nullptr)
            {
                rev_length += soc.receive(asio::buffer(packet_length + rev_length, sizeof(packet_length) - rev_length));
                auto finish = rev_length == sizeof(packet_length);

                if(finish)
                {
                    length = *(uint32_t *)packet_length;
                    rev_str.resize(length);
                    packet_payload = rev_str.data();
                    rev_length = 0;
                }
            }
            else
            {
                rev_length += soc.receive(asio::buffer(packet_payload + rev_length, length - rev_length));
                auto finish = rev_length == length;

                if(finish)
                {
                    packet_payload = nullptr;
                    rev_length = 0;

                    std::stringstream stream(rev_str);
                    std::vector<std::string> cmd_arg_list;

                    while(!stream.eof())
                    {
                        std::string t;
                        stream >> t;
                        cmd_arg_list.push_back(t);
                    }
                    
                    if(cmd_arg_list[0] == std::string("protocol"))
                    {
                        if((cmd_arg_list.size() >= 2) && (cmd_arg_list[1] == std::string("heart")))
                        {
                            send_cmd("protocol", "heart", "");
                        }
                        
                        continue;
                    }

                    if((cmd_arg_list.size() >= 2) && (cmd_arg_list[1] == std::string("pause") || cmd_arg_list[1] == std::string("p")))
                    {
                        set_pause_detected(true);
                    }

                    asio::post(recv_ioc, [&soc, rev_str]{socket_cmd_handle(soc, rev_str);});
                }
            }
        }
        catch(const std::exception &ex)
        {
            std::cout << MESSAGE_OUTPUT_PREFIX << ex.what() << std::endl;
            break;
        }
    }
    
    recv_thread_stopped = true;
    send_ioc.stop();
}

void tcp_server_thread_entry(asio::ip::tcp::acceptor &&listener)
{
    std::cout << MESSAGE_OUTPUT_PREFIX << "tcp_server thread tid: " << gettid() << std::endl;
    
    try
    {
        while(!program_stop)
        {
            std::cout << MESSAGE_OUTPUT_PREFIX "Wait Server Connection" << std::endl;
            auto soc = listener.accept();
            soc.set_option(asio::ip::tcp::no_delay(true));
            std::cout << MESSAGE_OUTPUT_PREFIX "Server Connected" << std::endl;
            recv_thread_stop = false;
            recv_thread_stopped = false;
            client_soc = &soc;
            std::thread rev_thread(tcp_server_recv_thread_entry, std::ref(soc));

            try
            {
                auto guard = asio::make_work_guard(send_ioc);
                send_ioc.run();
            }
            catch(const std::exception &ex)
            {
                std::cout << MESSAGE_OUTPUT_PREFIX << ex.what() << std::endl;
            }
    
            send_ioc.restart();
            client_soc = nullptr;
            recv_thread_stop = true;
            rev_thread.join();
            
            try
            {
                soc.shutdown(asio::socket_base::shutdown_both);
                soc.close();
            }
            catch(const std::exception &ex)
            {
            }
            
            std::cout << MESSAGE_OUTPUT_PREFIX "Server Disconnected" << std::endl;
        }

        listener.close();
        server_thread_stopped = true;
    }
    catch(const std::exception &ex)
    {
        std::cout << MESSAGE_OUTPUT_PREFIX << ex.what() << std::endl;
    }
}

static void telnet_init(const command_line_arg_t &arg)
{
    try
    {
        if(!arg.no_telnet)
        {
            asio::ip::tcp::acceptor listener(tcp_charfifo_thread_ioc, {asio::ip::address::from_string("0.0.0.0"), static_cast<asio::ip::port_type>(arg.telnet_port)});
            listener.set_option(asio::ip::tcp::no_delay(true));
            listener.listen();
            std::cout << MESSAGE_OUTPUT_PREFIX "Telnet Bind on 0.0.0.0:" << arg.telnet_port << std::endl;
            charfifo_thread_stopped = false;
            std::thread telnet_thread(tcp_charfifo_thread_entry, std::move(listener));
            telnet_thread.detach();
        }
        else
        {
            charfifo_thread_stopped = false;
            std::thread telnet_thread(std_output_charfifo_thread_entry);
            telnet_thread.detach();
        }
    }
    catch(const std::exception &e)
    {
        std::cout << MESSAGE_OUTPUT_PREFIX "Telnet Bind Port Failed:" << e.what() << std::endl;
        exit(EXIT_CODE_TELNET_PORT_ERROR);
    }
}

static void server_init(const command_line_arg_t &arg)
{
    try
    {
        if(!arg.no_controller)
        {
            asio::ip::tcp::acceptor listener(tcp_server_thread_ioc, {asio::ip::address::from_string("0.0.0.0"), static_cast<asio::ip::port_type>(arg.controller_port)});
            listener.set_option(asio::ip::tcp::no_delay(true));
            listener.listen();
            std::cout << MESSAGE_OUTPUT_PREFIX "Server Bind on 0.0.0.0:" << arg.controller_port << std::endl;
            server_thread_stopped = false;
            std::thread server_thread(tcp_server_thread_entry, std::move(listener));
            server_thread.detach();
        }
    }
    catch(const std::exception &e)
    {
        std::cout << MESSAGE_OUTPUT_PREFIX "Server Bind Port Failed:" << e.what() << std::endl;
        exit(EXIT_CODE_SERVER_PORT_ERROR);
    }
}

void network_init(const command_line_arg_t &arg)
{
    telnet_init(arg);
    network_command_init();
    server_init(arg);
}