#include <memory>
#include <iostream>
#include <string>
#include <thread>

#include <grpcpp/grpcpp.h>
#include <grpc/support/log.h>
#include "../exchange.h"
#include "exchange.grpc.pb.h"

using std::string;
using grpc::Server;
using grpc::ServerAsyncWriter;
using grpc::ServerAsyncReader;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerCompletionQueue;
using grpc::Status;
using exchange::ExchangeService;
using exchange::ReqChunk;
using exchange::Empty;

ReqChunk* GenChunk(int num) {
    //num = rand() % 10000;
    if(num < 1) {
        num = 1024;
    }
    ReqChunk* chk = new ReqChunk;
    chk->set_num(num);
    for(auto j=0; j< num; ++j) {
        chk->add_id(j);
        chk->add_name("abc");
        chk->add_score(rand()*1.0);
        chk->add_comment("abcaserfewqradf   adfawewerfasdgffasdfopi[15979841616dasfgdldkfgnvn k zsfgdzff454saf+89g165dvb");
    }
    return chk;
}

class ServerImpl final
{
public:
    ~ServerImpl()
    {
        server_->Shutdown();
        cq_->Shutdown();
    }

    void Run(string port)
    {
        std::string server_address("0.0.0.0:"+port);

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
                , responder_(&ctx_)
                , status_(CREATE)
                , times_(0)
        {
            Proceed();
        }

        void Proceed()
        {
            if (status_ == CREATE)
            {
                status_ = PROCESS;
                service_->RequestExchangeDataRet(&ctx_, &request_, &responder_, cq_, cq_, this);
                this->chunk_ = GenChunk(0);
            }
            else if (status_ == PROCESS)
            {
                // Now that we go through this stage multiple times,
                // we don't want to create a new instance every time.
                // Refer to gRPC's original example if you don't understand
                // why we create a new instance of CallData here.
                if (times_ == 0)
                {
                    new CallData(service_, cq_);
                    std::cout<< "request: "<< request_.name() << std::endl;
                }

                if (times_++ >= LIMIT)
                {
                    status_ = FINISH;
                    std::cout<< request_.name() <<"  "<< times_ <<" write finished!!" <<std::endl;
                    responder_.Finish(Status::OK, this);
                }
                else
                {
                    std::string prefix("Hello ");
                    chunk_->set_chunk_id(times_);
                    responder_.Write(*chunk_, this);
                }
            }
            else
            {
                GPR_ASSERT(status_ == FINISH);
                delete this;
            }
        }

    private:
        ExchangeService::AsyncService* service_;
        ServerCompletionQueue* cq_;
        ServerContext ctx_;

        Empty request_;
        ReqChunk* chunk_;

        ServerAsyncWriter<ReqChunk> responder_;

        int times_;

        enum CallStatus
        {
            CREATE,
            PROCESS,
            FINISH
        };
        CallStatus status_; // The current serving state.
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
    ServerImpl server;
    assert(argc==2);
    server.Run((argv[1]));

    return 0;
}
