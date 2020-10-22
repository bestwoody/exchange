
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <stdlib.h>
#include <grpcpp/grpcpp.h>
#include <grpc/support/log.h>
#include <thread>

#include "exchange.grpc.pb.h"
#include "../../exchange.h"

using std::vector;
using std::string;
using grpc::Channel;
using grpc::ClientAsyncReader;
using grpc::ClientAsyncWriter;
using grpc::ClientContext;
using grpc::CompletionQueue;
using grpc::Status;
using exchange::ExchangeService;
using exchange::ReqChunk;
using exchange::Empty;


class GreeterClient {
public:
    explicit GreeterClient(std::shared_ptr<Channel> channel,CompletionQueue* cq,string id)
            : stub_(ExchangeService::NewStub(channel)),cq_(cq),client_id_(id) {}

    // Assembles the client's payload and sends it to the server.
    void SayHello(const std::string& user, const int num_greetings)
    {
        // Call object to store rpc data
        AsyncClientCall* call = new AsyncClientCall;
        call->request.set_name(user);
        call->reader = stub_->AsyncExchangeDataRet(&call->context, call->request ,cq_,(void*)call);
        call->state_type = AsyncClientCall::CONNECTED;
        call->times= 0;
        call->client_id = this->client_id_;
    }

    // Loop while listening for completed responses.
    // Prints out the response from the server.
    void AsyncCompleteRpc() {
        void* got_tag;
        bool ok = false;

        // Block until the next result is available in the completion queue "cq".
        while (cq_->Next(&got_tag, &ok)) {
            // The tag in this example is the memory location of the call object
            AsyncClientCall* call = static_cast<AsyncClientCall*>(got_tag);
#ifdef DEBUG_
            std::cout<<"client id = "<< call->client_id<<" : ";
#endif
            // Verify that the request was completed successfully. Note that "ok"
            // corresponds solely to the request for updates introduced by Finish().
            if(!ok) {
                call->state_type = AsyncClientCall::DONE;
            }
            switch (call->state_type) {
                case AsyncClientCall::CONNECTED: {
#ifdef DEBUG_
                    std::cout << call->request.name() <<" begin to read "<< std::endl;
#endif
                    call->reader->Read(&call->reply,(void*)call);
                    call->state_type = AsyncClientCall::TOREAD;
                }break;
                case AsyncClientCall::TOREAD: {
                    call->times++;
#ifdef DEBUG_
                    std::cout << "read a chunk "<< call->reply.chunk_id() <<std::endl;
#else
                    if (call->times% MOD_LIMIT == 0) {
                        std::cout << "read a chunk "<< call->reply.chunk_id() <<std::endl;
                    }
#endif
                    call->reader->Read(&call->reply,(void*)call);
                }break;
                case AsyncClientCall::DONE: {
                    std::cout << call->request.name() <<" done "<< std::endl;
                    delete call;
                }
            }
        }
    }

private:

    // struct for keeping state and data information
    struct AsyncClientCall {
        // Container for the data we expect from the server.
        ReqChunk reply;
        Empty request;
        // Context for the client. It could be used to convey extra information to
        // the server and/or tweak certain RPC behaviors.
        ClientContext context;

        // Storage for the status of the RPC upon completion.
        Status status;

        std::unique_ptr<ClientAsyncReader<ReqChunk> > reader;
        enum StateType {CONNECTED,TOREAD,DONE};
        StateType state_type;
        std::atomic_int times;
        string client_id;
    };

    // Out of the passed in Channel comes the stub, stored here, our view of the
    // server's exposed services.
    std::unique_ptr<ExchangeService::Stub> stub_;

    // The producer-consumer queue we use to communicate asynchronously with the
    // gRPC runtime.

    CompletionQueue* cq_;
    string client_id_;
};

int main(int argc, char** argv) {
    std::cout<<"./sender 'n servers' '1-th ip' '1-th port' ... 'req num'"<<std::endl;
    assert(argc>2);
    int client_num=atoi(argv[1]);
    assert(2*client_num+3==argc);
    for(int i=0; i<client_num; ++i) {
        addr[i].ip = argv[2*i+2];
        addr[i].port = argv[2*i+3];
    }
    int req_num=atoi(argv[argc-1]);
    vector<CompletionQueue*> cqs;
    std::vector<GreeterClient*> clients;
    vector<std::thread>threads;

    for (auto i=0;i < client_num; ++i) {
        cqs.emplace_back(new CompletionQueue);
    }
    for (int i = 0; i< client_num; ++i) {
        clients.emplace_back(new GreeterClient(grpc::CreateChannel(
                addr[i].ip+":"+addr[i].port, grpc::InsecureChannelCredentials()),cqs[i],addr[i].ip+":"+addr[i].port+":"+std::to_string(i)));
    }


    // Spawn reader thread that loops indefinitely
    for (int i = 0; i < client_num; ++i) {
        threads.emplace_back(std::thread(&GreeterClient::AsyncCompleteRpc, clients[i]));
    }

    for (int i = 0; i < client_num; i++) {
        for(int j=0; j < req_num;++j) {
            std::string user("world req id = " + std::to_string(j) + " client id = " + addr[i].ip+":"+addr[i].port+":"+std::to_string(i));
            clients[i]->SayHello(user,j);  // The actual RPC call!
        }
    }

    std::cout << "Press control-c to quit" << std::endl << std::endl;
    for (int i = 0; i <client_num ; ++i) {
        threads[i].join();
    }  //blocks forever

    return 0;
}
