//
// Created by fangzhuhe on 2020/10/12.
//

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <grpcpp/grpcpp.h>
#include <thread>
#include <stdlib.h>
#include "../../exchange.grpc.pb.h"
#include "../../exchange.h"

using std::vector;
using std::string;
using grpc::Channel;
using grpc::ClientAsyncReader;
using grpc::ClientAsyncWriter;
using grpc::ClientContext;
using grpc::CompletionQueue;
using grpc::Status;
using namespace exchange;

class GreeterClient {
public:
    explicit GreeterClient(std::shared_ptr<Channel> channel,CompletionQueue*cq, string id)
            : stub_(ExchangeService::NewStub(channel)),cq_(cq),client_id_(id) {}

    // Assembles the client's payload and sends it to the server.
    void SendData(string msg)
    {
        // Call object to store rpc data
        AsyncClientCall* call = new AsyncClientCall;
        call->request= GenChunk(0);
        call->chunk_list= GenChunkList(call->chunk_list_size);
        call->times= 0;
        call->writer = stub_->AsyncExchangeData(&call->context, &call->reply ,cq_,(void*)call);
        call->state_type = AsyncClientCall::CONNECTED;
        call->client_id= this->client_id_;
        std::cout<<msg<<" begin to send data!!"<<std::endl;
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
                    call->request= call->chunk_list[call->times%call->chunk_list_size];
                    call->request->set_chunk_id(call->times);
                    call->writer->Write(*call->request,(void*)call);
                    call->state_type = AsyncClientCall::TOREAD;
#ifdef DEBUG_
                    std::cout<< " send chunk id = "  << call->request->chunk_id() << std::endl;
#endif
                }break;
                case AsyncClientCall::TOREAD: {
                    call->times++;
                    if(call->times >= LIMIT) {
                        call->writer->Finish(&call->status,call);
                        call->state_type = AsyncClientCall::DONE;
                    }else {
                        call->request= call->chunk_list[call->times%call->chunk_list_size];
                        call->request->set_chunk_id(call->times);
                        call->writer->Write(*call->request,(void*)call);
#ifdef DEBUG_
                        std::cout<< " send chunk id = "  << call->request->chunk_id() << std::endl;
#else
                        if(call->request->chunk_id() % MOD_LIMIT == 0) {
                            std::cout<<"client id = "<< call->client_id<<" ";
                            std::cout<< " send chunk id = "  << call->request->chunk_id() << std::endl;
                        }
#endif
                    }
                }break;
                case AsyncClientCall::DONE: {
                    std::cout<<" send data "<< call->times<<" Done!"<<std::endl;
                    delete call;
                }
            }
        }
    }

private:

    // struct for keeping state and data information
    struct AsyncClientCall {
        // Container for the data we expect from the server.
        ReplySummary reply;
        ReqChunk* request;
        ReqChunk** chunk_list;
        int chunk_list_size;
        // Context for the client. It could be used to convey extra information to
        // the server and/or tweak certain RPC behaviors.
        ClientContext context;

        // Storage for the status of the RPC upon completion.
        Status status;

        std::unique_ptr<ClientAsyncWriter<ReqChunk> > writer;
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
    vector<std::thread>threads;
#if 0
    for (auto i=0;i< client_num* req_num; ++i) {
        cqs.emplace_back(new CompletionQueue);
    }
    std::vector<GreeterClient*> clients;
    for (int i = 0; i< client_num; ++i) {
        for(int j=0; j< req_num; ++j) {
            clients.emplace_back(new GreeterClient(grpc::CreateChannel(
                    addr[i].ip+":"+addr[i].port, grpc::InsecureChannelCredentials()),cqs[i*req_num+j],addr[i].ip+":"+addr[i].port+":"+std::to_string(i)));
        }
    }

    // Spawn reader thread that loops indefinitely
    for (auto i=0;i< client_num* req_num; ++i) {
        threads.emplace_back(std::thread(&GreeterClient::AsyncCompleteRpc, clients[i]));
    }

    for (int i = 0; i< client_num ; i++) {
        for(int j=0;j<req_num;++j) {
            clients[i*req_num+j]->SendData("Send data Req id = " + std::to_string(j) + " client id = " + addr[i].ip+":"+addr[i].port+":"+std::to_string(j));
        }
    }
    std::cout << "Press control-c to quit" << std::endl << std::endl;
    for (auto i=0;i< client_num* req_num; ++i) {
        threads[i].join();
    }
#else
    for (auto i=0;i < client_num; ++i) {
        cqs.emplace_back(new CompletionQueue);
    }
    std::vector<GreeterClient*> clients;
    for (int i = 0; i< client_num; ++i) {
        grpc::ChannelArguments  channelArgs;
        channelArgs.SetMaxReceiveMessageSize(MSG_SIZE);
        channelArgs.SetMaxSendMessageSize(MSG_SIZE);
        clients.emplace_back(new GreeterClient(grpc::CreateCustomChannel(
                    addr[i].ip+":"+addr[i].port, grpc::InsecureChannelCredentials(),channelArgs),cqs[i],addr[i].ip+":"+addr[i].port+":"+std::to_string(i)));
    }

    // Spawn reader thread that loops indefinitely
    for (auto i=0;i< client_num; ++i) {
        threads.emplace_back(std::thread(&GreeterClient::AsyncCompleteRpc, clients[i]));
    }
    
    for (int i = 0; i< client_num ; i++) {
        // send more requests to each client
        for(int j=0; j < req_num; ++j) {
            clients[i]->SendData("Send data Req id = " + std::to_string(j) + " client id = " + addr[i].ip+":"+addr[i].port+":"+std::to_string(j));
        }
    }
    std::cout << "Press control-c to quit" << std::endl << std::endl;
    for (auto i=0;i< client_num; ++i) {
        threads[i].join();
    }
#endif
    return 0;
}
