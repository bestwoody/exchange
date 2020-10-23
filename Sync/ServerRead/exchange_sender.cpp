//
// Created by fangzhuhe on 2020/10/12.
//
#include <memory>
#include <iostream>
#include <string>
#include <grpcpp/grpcpp.h>
#include <grpc/support/log.h>
#include <thread>
#include "../../exchange.grpc.pb.h"
#include "../../exchange.h"
using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Status;
using namespace exchange;
using namespace std;

class ExchangeClient {
public:
    ExchangeClient(std::shared_ptr<Channel> channel,int client_id)
    : stub_(ExchangeService::NewStub(channel)),client_id_(client_id) {
        chunk_ = GenChunk(0);
        chunk_num_ = 0;
    }
    void SendData() {
        ClientContext context;
        ReplySummary response;
        std::unique_ptr<ClientWriter<ReqChunk> > writer(stub_->ExchangeData(&context, &response));
        for(auto i=0; i< LIMIT; ++i) {
            if(!writer->Write(*chunk_)){
                break;
            }
            chunk_num_++;
            if(chunk_num_ % MOD_LIMIT == 0) {
                cout<< client_id_ <<" client send chunks = "<< chunk_num_<< endl;
            }
        }
        writer->WritesDone();
        std::cout <<"client write done" << std::endl;
        Status status = writer->Finish();
        if (status.ok()) {
            std::cout << "Finished Send data!"<< std::endl;
        } else {
            std::cout << "Send data rpc failed." << std::endl;
        }
    }

private:
    std::unique_ptr<ExchangeService::Stub> stub_;
    ReqChunk* chunk_;
    atomic_int chunk_num_;
    int client_id_;
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
    vector<thread>threads;
    vector<ExchangeClient*>clients;
    int req_num=atoi(argv[argc-1]);
    for (int i=0;i< client_num;++i) {
        ExchangeClient* new_client =new ExchangeClient(grpc::CreateChannel(addr[i].ip+":"+addr[i].port,
                                                                grpc::InsecureChannelCredentials()),i);
        clients.emplace_back(new_client);
        threads.emplace_back(thread(&ExchangeClient::SendData,new_client));
    }

    for(int i=0;i< client_num;++i) {
        threads[i].join();
    }
    return 0;
}