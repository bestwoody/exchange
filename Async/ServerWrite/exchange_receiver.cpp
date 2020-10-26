#include <memory>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include <grpcpp/grpcpp.h>
#include <grpc/support/log.h>
#include "../../exchange.h"
#include "../../exchange.grpc.pb.h"

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
using namespace std;

class ServerImpl final
{
public:
    ~ServerImpl()
    {
        for (int i = 0; i < cqs_.size(); i++) {
            cqs_[i]->Shutdown();
        }
        void* tag;
        bool ok;
        for (auto cq = cqs_.begin(); cq != cqs_.end(); ++cq) {
            while ((*cq)->Next(&tag, &ok)) {
                GPR_ASSERT(ok);
                static_cast<CallData*>(tag)->Proceed();
            }
        }
        server_->Shutdown();
    }

    void Run(string ip, string port, int thread_num)
    {
        std::string server_address(ip+":"+port);
        ServerBuilder builder;
        builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
        builder.RegisterService(&service_);
        builder.SetMaxReceiveMessageSize(MSG_SIZE);
        builder.SetMaxSendMessageSize(MSG_SIZE);
        for (int i = 0; i < thread_num; i++)
            cqs_.emplace_back(builder.AddCompletionQueue());
        server_ = builder.BuildAndStart();
        std::cout << "Server listening on " << server_address << std::endl;
        for(auto i=0; i< thread_num;++i) {
            works.emplace_back(thread(&ServerImpl::HandleRpcs, this, i));
        }
        for (int i = 0; i < thread_num; ++i) {
            works[i].join();
            std::cout <<"Server "<< i <<" is done!"<< std::endl;
        }
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
                    std::cout<< request_.name()  <<" write finished!!" <<std::endl;
                    responder_.Finish(Status::OK, this);
                }
                else
                {
                    chunk_->set_chunk_id(times_);
#ifdef DEBUG_
                    std::cout<< request_.name() <<"  "<< times_ <<" write a chunk." <<std::endl;
#else
                    if (times_ % MOD_LIMIT == 0) {
                        std::cout<< request_.name() <<"  "<< times_ <<" write a chunk." <<std::endl;
                    }
#endif
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


    void HandleRpcs(int id)
    {
        new CallData(&service_, cqs_[id].get());
        void* tag; // uniquely identifies a request.
        bool ok;
        while (true)
        {
            GPR_ASSERT(cqs_[id]->Next(&tag, &ok));
            GPR_ASSERT(ok);
            static_cast<CallData*>(tag)->Proceed();
        }
    }

    vector<std::unique_ptr<ServerCompletionQueue> > cqs_;
    ExchangeService::AsyncService service_;
    std::unique_ptr<Server> server_;
    vector<std::thread> works;

};

int main(int argc, char** argv)
{
    std::cout<<"input 'server ip' 'server port'"<<std::endl;
    ServerImpl server;
    assert(argc==4);
    server.Run(argv[1], argv[2], atoi(argv[3]));

    return 0;
}
