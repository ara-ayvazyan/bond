// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#pragma once

#include <bond/core/blob.h>

#include <boost/make_shared.hpp>
#include <boost/optional.hpp>
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4505)
#endif
#include <boost/program_options.hpp>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <numeric>
#include <thread>
#include <vector>

namespace perf
{
    class options
    {
    public:
        options(int argc, char* argv[])
        {
            namespace po = boost::program_options;

            po::options_description desc{ "perf.exe program options" };
            desc.add_options()
                ("help,h", "Print usage message")
                ("transport,t", po::value<std::string>(&_transport)->required(), "Transport name")
                ("server,s", po::bool_switch(&_server)->default_value(false), "Host server")
                ("clients,c", po::value<std::uint16_t>(&_clients)->default_value(0), "Number of clients to use")
                ("ip", po::value<std::string>(&_ip)->default_value("127.0.0.1"), "IP Address")
                ("port", po::value<std::uint16_t>(&_port)->default_value(7000), "Port number")
                ("flights,f", po::value<std::uint32_t>(&_flights)->default_value(1000), "Number of active requests")
                ("payload,p", po::value<std::uint32_t>(&_payload)->default_value(1024), "Payload size in bytes")
                ("duration,d", po::value<std::uint32_t>(&_duration)->default_value(0), "Test execution duration in seconds")
                ("csv", po::bool_switch(&_csv)->default_value(false), "CSV format")
                ("csv-header", po::bool_switch(&_csv_header)->default_value(false), "CSV format header");

            po::variables_map vm;
            po::store(po::parse_command_line(argc, argv, desc), vm);

            if (vm.count("help"))
            {
                std::stringstream ss;
                ss << desc;
                throw std::logic_error{ ss.str() };
            }

            po::notify(vm);
        }

        const std::string& transport() const { return _transport; }

        bool host_server() const { return _server; }

        std::uint16_t clients() const { return _clients; }

        const std::string& ip() const { return _ip; }

        std::uint16_t port() const { return _port; }

        std::uint32_t payload() const { return _payload; }

        std::uint32_t flights() const { return _flights; }

        std::uint32_t duration() const { return _duration; }

        bool csv() const { return _csv; }

        bool csv_header() const { return _csv_header; }

    private:
        std::string _transport;
        bool _server;
        std::uint16_t _clients;
        std::string _ip;
        std::uint16_t _port;
        std::uint32_t _payload;
        std::uint32_t _flights;
        std::uint32_t _duration;
        bool _csv;
        bool _csv_header;
    };

    struct stats
    {
        std::chrono::seconds time{ std::chrono::seconds::zero() };
        std::uint64_t failed{ 0 };
        std::vector<std::chrono::microseconds> latencies;
        boost::optional<options> opts;
    };

    std::ostream& operator<<(std::ostream& os, const std::chrono::microseconds& d)
    {
        if (d.count() < 100ULL)
        {
            return os << d.count() << "micro";
        }

        if (d.count() < 1000000ULL)
        {
            return os << float(d.count()) / 1000.0f << "ms";
        }

        return os << float(d.count()) / 1000000.0f << "s";
    }

    std::ostream& operator<<(std::ostream& os, const stats& s)
    {
        const auto& opts = s.opts;
        if (opts && opts->csv())
        {
            const auto sep = ',';

            if (opts->csv_header())
            {
                os  << "Requests" << sep
                    << "Failed" << sep
                    << "Clients" << sep
                    << "Payload" << sep
                    << "Flights" << sep
                    << "Duration" << sep
                    << "Runtime" << sep
                    << "QPS" << sep
                    << "Min" << sep
                    << "Avg" << sep
                    << "50%" << sep
                    << "90%" << sep
                    << "95%" << sep
                    << "99%" << sep
                    << "Max" << std::endl;
            }

            os  << s.latencies.size() << sep
                << s.failed << sep
                << opts->clients() << sep
                << opts->payload() << sep
                << opts->flights() << sep
                << opts->duration() << sep
                << s.time.count() << sep
                << (s.time.count() != 0 ? s.latencies.size() / s.time.count() : 0) << sep;

            if (!s.latencies.empty())
            {
                os  << s.latencies.front().count() << sep
                    << (std::accumulate(s.latencies.begin(), s.latencies.end(), std::chrono::microseconds::zero()) / s.latencies.size()).count() << sep
                    << s.latencies[std::size_t(0.50 * s.latencies.size())].count() << sep
                    << s.latencies[std::size_t(0.90 * s.latencies.size())].count() << sep
                    << s.latencies[std::size_t(0.95 * s.latencies.size())].count() << sep
                    << s.latencies[std::size_t(0.99 * s.latencies.size())].count() << sep
                    << s.latencies.back().count();
            }
            else
            {
                os  << 0 << sep
                    << 0 << sep
                    << 0 << sep
                    << 0 << sep
                    << 0 << sep
                    << 0 << sep
                    << 0;
            }
        }
        else
        {
            os  << "Requests: " << s.latencies.size() << " [Failed:" << s.failed << "]" << std::endl
                << "Time: " << s.time.count() << "s" << std::endl;

            if (s.time.count() != 0)
            {
                os  << "QPS: " << s.latencies.size() / s.time.count() << std::endl;
            }

            if (!s.latencies.empty())
            {
                os  << "Latencies:"
                    << std::setprecision(2) << std::fixed
                    << "  Min:" << s.latencies.front()
                    << ", Avg:" << std::accumulate(s.latencies.begin(), s.latencies.end(), std::chrono::microseconds::zero()) / s.latencies.size()
                    << ", 50%:" << s.latencies[std::size_t(0.50 * s.latencies.size())]
                    << ", 90%:" << s.latencies[std::size_t(0.90 * s.latencies.size())]
                    << ", 95%:" << s.latencies[std::size_t(0.95 * s.latencies.size())]
                    << ", 99%:" << s.latencies[std::size_t(0.99 * s.latencies.size())]
                    << ", Max:" << s.latencies.back();
            }
        }

        return os;
    }

    class application
    {
    public:
        application(int argc, char* argv[])
            : _options{ argc, argv }
        {}

        using HostServerFunc = std::function<std::shared_ptr<void>(const std::string& ip, std::uint16_t port)>;
        using ClientFunc = std::function<void(bond::blob request, const std::function<void(boost::optional<bond::blob> response)>& callback)>;
        using MakeClientFunc = std::function<ClientFunc(const std::string& ip, std::uint16_t port)>;

        bool add_transport(const std::string& name, const HostServerFunc& hostServerFunc, const MakeClientFunc& makeClientFunc)
        {
            return _transports.emplace(name, std::make_pair(hostServerFunc, makeClientFunc)).second;
        }

        boost::optional<stats> run()
        {
            if (!_options.host_server() && !_options.clients())
            {
                throw std::logic_error{ "One of the --server or --clients arguments is required." };
            }

            HostServerFunc hostServer;
            MakeClientFunc makeClient;
            std::tie(hostServer, makeClient) = find_transport();

            std::shared_ptr<void> server;

            if (_options.host_server())
            {
                std::cout << "Hosting server at " << _options.ip() << ":" << _options.port() << std::endl;
                server = hostServer(_options.ip(), _options.port());
            }

            std::function<stats()> done;

            if (_options.clients())
            {
                std::cout << "Starting " << _options.clients() <<  " client(s) for " << _options.ip() << ":" << _options.port() << std::endl;
                done = start_clients(makeClient);
            }

            const auto clear_line = "\r                              \r";

            if (_options.duration())
            { 
                for (std::int64_t i = _options.duration(); i > 0; --i)
                {
                    std::cout << clear_line << "Waiting for " << std::chrono::seconds{ i };
                    std::this_thread::sleep_for(std::chrono::seconds{ 1 });
                }
            }
            else
            {
                std::cout << "Press ENTER to exit";
                std::cin.get();
            }

            std::cout << clear_line << "Stopping...";

            boost::optional<stats> result;
            if (done)
            {
                result = done();

                if (result)
                {
                    result->opts = _options;
                }
            }

            server.reset();

            std::cout << clear_line;
            return result;
        }

    private:
        using TransportFuncs = std::pair<HostServerFunc, MakeClientFunc>;

        TransportFuncs find_transport() const
        {
            auto it = _transports.find(_options.transport());

            if (it == _transports.end())
            {
                throw std::logic_error{ "Transport not found." };
            }

            return it->second;
        }

        std::function<stats()> start_clients(const MakeClientFunc& makeClient)
        {
            class counter : private std::mutex, private std::condition_variable
            {
            public:
                explicit counter(std::uint32_t value)
                    : _value{ value }
                {}

                void signal()
                {
                    bool zero;
                    {
                        std::lock_guard<std::mutex> guard{ *this };
                        zero = (--_value == 0);
                    }

                    if (zero)
                    {
                        notify_all();
                    }
                }

                void wait()
                {
                    std::unique_lock<std::mutex> guard{ *this };
                    std::condition_variable::wait(guard, [this] { return _value == 0; });
                }

            private:
                std::uint32_t _value;
            };

            class runner
            {
            public:
                runner(const MakeClientFunc& makeClient, const options& opt)
                    : _count{ opt.flights() },
                      _request{ boost::make_shared_noinit<std::uint8_t[]>(opt.payload()), opt.payload() },
                      _begin{ std::chrono::high_resolution_clock::now() }
                {
                    for (uint16_t i = 0; i < opt.clients(); ++i)
                    {
                        _clients.push_back(makeClient(opt.ip(), opt.port()));
                    }

                    for (uint16_t i = 0; i < opt.flights(); ++i)
                    {
                        invoke_client();
                    }
                }

                runner(const runner& other) = delete;
                runner& operator=(const runner& other) = delete;

                ~runner()
                {
                    if (!_stop)
                    {
                        stop();
                    }
                }

                stats stop()
                {
                    _stop = true;
                    _count.wait();

                    stats s;
                    s.time = std::chrono::duration_cast<std::chrono::seconds>(
                        std::chrono::high_resolution_clock::now() - _begin);
                    s.failed = _failed;
                    s.latencies = std::move(_latencies);

                    std::sort(s.latencies.begin(), s.latencies.end());

                    return s;
                }

            private:
                void invoke_client()
                {
                    auto begin = std::chrono::high_resolution_clock::now();

                    auto handler = [this, begin](boost::optional<bond::blob> response)
                    {
                        if (!_stop)
                        {
                            const auto latency = std::chrono::duration_cast<std::chrono::microseconds>(
                                std::chrono::high_resolution_clock::now() - begin);

                            invoke_client();

                            if (response)
                            {
                                std::lock_guard<std::mutex> guard{ _latenciesLock };
                                _latencies.push_back(latency);
                            }
                            else
                            {
                                ++_failed;
                            }
                        }
                        else
                        {
                            _count.signal();
                        }
                    };

                    _clients[++_index % _clients.size()](_request, handler);
                }

                std::vector<ClientFunc> _clients;
                std::atomic_size_t _index{ 0 };
                std::atomic_bool _stop{ false };
                counter _count;
                const bond::blob _request;
                std::atomic<std::uint64_t> _failed{ 0 }; // std::atomic_uint64_t is missing in libstdc++ < 7.1
                std::mutex _latenciesLock;
                std::vector<std::chrono::microseconds> _latencies;
                const std::chrono::high_resolution_clock::time_point _begin;
            };

            auto r = std::make_shared<runner>(makeClient, _options);

            return [r]{ return r->stop(); };
        }

        options _options;
        std::map<std::string, TransportFuncs> _transports;
    };

} // namespace perf
