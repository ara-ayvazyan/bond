// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "perf.h"
#include "perf_grpc.h"
#include "perf.grpc.pb.h"

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

    namespace grpc
    {
        class ServiceImpl : public PerfService::Service
        {
        private:
            ::grpc::Status Ping(
                ::grpc::ServerContext* /*context*/,
                const ::google::protobuf::BytesValue* request,
                ::google::protobuf::BytesValue* response) override
            {
                response->CopyFrom(*request);
                return ::grpc::Status::OK;
            }
        };

    } // namespace grpc

} // namespace perf

int main(int argc, char* argv[])
{
    try
    {
        perf::application app{ argc, argv };

        std::shared_ptr<bond::ext::grpc::io_manager> ioManager;

        app.add_transport(
            "proto-grpc",
            [](const std::string& ip, std::uint16_t port)
            {
                auto service = std::make_shared<perf::grpc::ServiceImpl>();

                std::shared_ptr<grpc::Server> server = grpc::ServerBuilder{}
                    .AddListeningPort(ip + ":" + std::to_string(port), grpc::InsecureServerCredentials())
                    .RegisterService(service.get())
                    .BuildAndStart();

                return std::shared_ptr<void>{
                    (void*)0x1,
                    [service, server](void*)
                    {
                        server->Shutdown();
                        server->Wait();
                    } };
            },
            [ioManager](const std::string& ip, std::uint16_t port) mutable
            {
                if (!ioManager)
                {
                    ioManager = std::make_shared<bond::ext::grpc::io_manager>();
                }

                std::shared_ptr<perf::grpc::PerfService::Stub> client = perf::grpc::PerfService::NewStub(
                    grpc::CreateChannel(ip + ":" + std::to_string(port), grpc::InsecureChannelCredentials()));

                return [client, ioManager](bond::blob request, const std::function<void(boost::optional<bond::blob>)>& callback)
                {
                    thread_local const void* s_blob = nullptr;
                    thread_local auto s_bytes = std::make_shared<const google::protobuf::BytesValue>();

                    if (request.data() != s_blob)
                    {
                        auto bytes = std::make_shared<google::protobuf::BytesValue>();
                        bytes->set_value(request.content(), request.size());
                        s_bytes = std::move(bytes);
                        s_blob = request.data();
                    }

                    class handler : public bond::ext::grpc::detail::io_manager_tag
                    {
                    public:
                        handler(
                            std::shared_ptr<const google::protobuf::BytesValue> request,
                            perf::grpc::PerfService::Stub& client,
                            grpc::CompletionQueue* cq,
                            const std::function<void(boost::optional<bond::blob>)>& callback)
                            : _request{ std::move(request) },
                              _rpc{ client.PrepareAsyncPing(&_context, *_request, cq) },
                              _callback{ callback }
                        {}

                        void invoke()
                        {
                            _rpc->StartCall();
                            _rpc->Finish(&_response, &_status, tag());
                        }

                    private:
                        void invoke(bool ok) override
                        {
                            std::unique_ptr<handler> self{ this };

                            if (ok && _status.ok())
                            {
                                // TODO: avoid copy?
                                _callback(bond::blob{ _response.value().data(), uint32_t(_response.value().size()) });
                            }
                            else
                            {
                                _callback({});
                            }
                        }

                        std::shared_ptr<const google::protobuf::BytesValue> _request;
                        google::protobuf::BytesValue _response;
                        grpc::Status _status;
                        grpc::ClientContext _context;
                        std::unique_ptr<grpc::ClientAsyncResponseReader<google::protobuf::BytesValue>> _rpc;
                        std::function<void(boost::optional<bond::blob>)> _callback;
                    };

                    std::unique_ptr<handler> h{ new handler{ s_bytes, *client, ioManager->cq(), callback } };
                    h->invoke();
                    h.release();
                };
            });

        boost::optional<bond::ext::grpc::thread_pool> threadPool;

        app.add_transport(
            "bond-grpc",
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
