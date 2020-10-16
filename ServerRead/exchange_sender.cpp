//
// Created by fangzhuhe on 2020/10/12.
//

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <grpcpp/grpcpp.h>
#include <grpc/support/log.h>
#include <thread>
#include "exchange.grpc.pb.h"
#include "../exchange.h"

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
    explicit GreeterClient(std::shared_ptr<Channel> channel,CompletionQueue*cq, int id)
            : stub_(ExchangeService::NewStub(channel)),cq_(cq),client_id_(id) {}

    // Assembles the client's payload and sends it to the server.
    void SendData(string msg)
    {
        // Call object to store rpc data
        AsyncClientCall* call = new AsyncClientCall;
        call->request= GenChunk(0);
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
            std::cout<<"client id = "<< call->client_id<<" : ";
            // Verify that the request was completed successfully. Note that "ok"
            // corresponds solely to the request for updates introduced by Finish().
            if(!ok) {
                call->state_type = AsyncClientCall::DONE;
            }
            switch (call->state_type) {
                case AsyncClientCall::CONNECTED: {
                    call->request->set_chunk_id(call->times);
                    call->writer->Write(*call->request,(void*)call);
                    call->state_type = AsyncClientCall::TOREAD;
                    std::cout<< " send chunk id = "  << call->request->chunk_id() << std::endl;
                }break;
                case AsyncClientCall::TOREAD: {
                    call->times++;
                    if(call->times >= LIMIT) {
                        call->writer->Finish(&call->status,call);
                        call->state_type = AsyncClientCall::DONE;
                    }else {
                        call->request->set_chunk_id(call->times);
                        call->writer->Write(*call->request,(void*)call);
                        std::cout<< " send chunk id = "  << call->request->chunk_id() << std::endl;
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
        // Context for the client. It could be used to convey extra information to
        // the server and/or tweak certain RPC behaviors.
        ClientContext context;

        // Storage for the status of the RPC upon completion.
        Status status;

        std::unique_ptr<ClientAsyncWriter<ReqChunk> > writer;
        enum StateType {CONNECTED,TOREAD,DONE};
        StateType state_type;
        std::atomic_int times;
        int client_id;
    };

    // Out of the passed in Channel comes the stub, stored here, our view of the
    // server's exposed services.
    std::unique_ptr<ExchangeService::Stub> stub_;

    // The producer-consumer queue we use to communicate asynchronously with the
    // gRPC runtime.
    CompletionQueue* cq_;
    int client_id_;
};

int main(int argc, char** argv) {

    int client_num=3;
    int req_num=5;
    CompletionQueue cq;
    std::vector<GreeterClient*> clients;
    for (int i = 0; i< client_num; ++i) {
        clients.emplace_back(new GreeterClient(grpc::CreateChannel(
                "localhost:5005"+std::to_string(i), grpc::InsecureChannelCredentials()),&cq,i));
    }

    // Spawn reader thread that loops indefinitely
    std::thread thread_ = std::thread(&GreeterClient::AsyncCompleteRpc, clients[0]);

    for (int i = 5; i <= req_num; i++) {
        for(int j=0;j<client_num;++j) {
            clients[j]->SendData("Send data Req id = " + std::to_string(i) + " client id = " + std::to_string(j));
        }
    }

    std::cout << "Press control-c to quit" << std::endl << std::endl;
    thread_.join();  //blocks forever

    return 0;
}
