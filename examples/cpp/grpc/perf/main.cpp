// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "perf.h"
#include "perf_grpc.h"

#include <bond/core/box.h>
#include <bond/ext/grpc/server.h>
#include <bond/ext/grpc/thread_pool.h>

#include <iostream>

namespace perf
{
    class ServiceImpl : public PerfService::Service
    {
    public:
        using PerfService::Service::Service;

    private:
        void Ping(bond::ext::grpc::unary_call<bond::Box<bond::blob>, bond::Box<bond::blob>> call) override
        {
            call.Finish(call.request().Deserialize<bond::Box<bond::blob>>());
        }
    };

} // namespace perf

int main(int argc, char* argv[])
{
    try
    {
        perf::application app{ argc, argv };

        boost::optional<bond::ext::grpc::thread_pool> threadPool;
        std::shared_ptr<bond::ext::grpc::io_manager> ioManager;

        app.add_transport(
            "grpc",
            [threadPool](const std::string& ip, std::uint16_t port) mutable
            {
                if (!threadPool)
                {
                    threadPool.emplace();
                }

                grpc::ServerBuilder builder;
                builder.AddListeningPort(ip + ":" + std::to_string(port), grpc::InsecureServerCredentials());

                return std::make_shared<bond::ext::grpc::server>(
                    bond::ext::grpc::server::Start(
                        builder,
                        std::unique_ptr<perf::ServiceImpl>{ new perf::ServiceImpl{ *threadPool } }));
            },
            [threadPool, ioManager](const std::string& ip, std::uint16_t port) mutable
            {
                if (!threadPool)
                {
                    threadPool.emplace();
                }

                if (!ioManager)
                {
                    ioManager = std::make_shared<bond::ext::grpc::io_manager>();
                }

                auto client = std::make_shared<perf::PerfService::Client>(
                    grpc::CreateChannel(ip + ":" + std::to_string(port), grpc::InsecureChannelCredentials()),
                    ioManager,
                    *threadPool);

                return [client](bond::blob request, const std::function<void(boost::optional<bond::blob>)>& callback)
                {
                    client->AsyncPing(
                        bond::make_box(std::move(request)),
                        [callback](bond::ext::grpc::unary_call_result<bond::Box<bond::blob>> result)
                        {
                            if (result.status().ok())
                            {
                                callback(result.response().Deserialize<bond::Box<bond::blob>>().value);
                            }
                            else
                            {
                                callback({});
                            }
                        });
                };
            });

        if (auto stats = app.run())
        {
            std::cout << *stats << std::endl;
        }
    }
    catch (const std::exception& e)
    {
        std::cout << e.what() << std::endl;
    }
}
