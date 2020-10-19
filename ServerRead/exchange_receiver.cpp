//
// Created by fangzhuhe on 2020/10/12.
//


#include <memory>
#include <iostream>
#include <string>
#include <grpcpp/grpcpp.h>
#include <grpc/support/log.h>

#include "exchange.grpc.pb.h"
#include "../exchange.h"

using std::string;
using grpc::Server;
using grpc::ServerAsyncWriter;
using grpc::ServerAsyncReader;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerCompletionQueue;
using grpc::Status;
using namespace exchange;
using namespace std;

class ServerImpl final
{
public:
    ~ServerImpl()
    {
        server_->Shutdown();
        cq_->Shutdown();
    }

    void Run(string ip, string port)
    {
        std::string server_address(ip+":"+port);

        ServerBuilder builder;
        builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
        builder.RegisterService(&service_);

        cq_ = builder.AddCompletionQueue();
        server_ = builder.BuildAndStart();
        std::cout << "Server listening on " << server_address << std::endl;

        HandleRpcs();
    }

private:
    class CallData
    {
    public:
        CallData(ExchangeService::AsyncService* service, ServerCompletionQueue* cq)
                : service_(service)
                , cq_(cq)
                , reader_(&ctx_)
                , state_(CREATE)
                , times_(0)
        {
            Proceed();
        }

        void Proceed()
        {
            if (state_ == CREATE)
            {
                state_ = PROCESS;
                service_->RequestExchangeData(&ctx_,  &reader_, cq_, cq_, this);
            }
            else if (state_ == PROCESS)
            {
                // Now that we go through this stage multiple times,
                // we don't want to create a new instance every time.
                // Refer to gRPC's original example if you don't understand
                // why we create a new instance of CallData here.
                if (times_ == 0)
                {
                    new CallData(service_, cq_);
                    reader_.Read(&chunk_, this);
                    times_++;
                    return;
                }
#ifdef DEBUG_
                // process received chunks
                cout<<"receive a chunk: "<< chunk_.chunk_id()<< endl;
#else
                if (chunk_.chunk_id() %MOD_LIMIT==0) {
                    cout<<"receive a chunk: "<< chunk_.chunk_id()<< endl;
                }
#endif
                if (times_>= LIMIT)
                {
                    state_ = FINISH;
                    reply_.set_received_chunks(times_);
                    std::cout<<times_<<" read finish!!!"<< std::endl;
                    reader_.Finish(reply_,Status::OK, this);
                }
                else
                {
                    // read one more
                    ++times_;
                    reader_.Read(&chunk_, this);
                }
            }
            else
            {
#ifdef DEBUG_
                std::cout<<"delete this!!!"<< std::endl;
#endif
                GPR_ASSERT(state_ == FINISH);
                delete this;
            }
        }

    private:
        ExchangeService::AsyncService* service_;
        ServerCompletionQueue* cq_;
        ServerContext ctx_;
        ReqChunk chunk_;
        ReplySummary reply_;
        ServerAsyncReader<ReplySummary, ReqChunk> reader_;

        int times_;

        enum CallStates
        {
            CREATE,
            PROCESS,
            FINISH
        };
        CallStates state_; // The current serving state.
    };

    void HandleRpcs()
    {
        new CallData(&service_, cq_.get());
        void* tag; // uniquely identifies a request.
        bool ok;
        while (true)
        {
            GPR_ASSERT(cq_->Next(&tag, &ok));
            GPR_ASSERT(ok);
            static_cast<CallData*>(tag)->Proceed();
        }
    }

    std::unique_ptr<ServerCompletionQueue> cq_;
    ExchangeService::AsyncService service_;
    std::unique_ptr<Server> server_;
};

int main(int argc, char** argv)
{
    std::cout<<"input 'server ip' 'server port'"<<std::endl;
    ServerImpl server;
    assert(argc==3);
    server.Run(argv[1], argv[2]);

    return 0;
}
