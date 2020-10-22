//
// Created by fangzhuhe on 2020/10/12.
//
#include <memory>
#include <iostream>
#include <string>
#include <grpcpp/grpcpp.h>
#include <grpc/support/log.h>
#include <thread>
#include "exchange.grpc.pb.h"
#include "../../exchange.h"

using std::string;
using grpc::Server;
using grpc::ServerWriter;
using grpc::ServerReader;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using namespace exchange;
using namespace std;
ReqChunk* GenChunk(int num) {
    if(num < 1) {
        num = CHUNK_NUM;
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
class ExchangeServiceImp final : public ExchangeService::Service {
public: explicit ExchangeServiceImp(int client_num):client_num_(client_num),receive_chunk_num(0){
    chunk_ = GenChunk(0);
    }
    Status ExchangeDataRet(ServerContext* context, const Empty* request, ServerWriter<ReqChunk>* writer) override {
        while (writer->Write(*chunk_)) {
            receive_chunk_num++;
            if(receive_chunk_num % MOD_LIMIT ==0) {
                cout << "exchange write chunks = "<< receive_chunk_num<<endl;
            }
        }
        return Status::OK;
    }

private:
    ReqChunk* chunk_;
    int client_num_;
    std::atomic_int receive_chunk_num;
};
void RunServer(string ip, string port, int client_num) {
    std::string server_address(ip+ ":"+port);
    ExchangeServiceImp service(client_num);
    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;
    server->Wait();
}

int main(int argc, char** argv) {
    assert(argc == 4);
    RunServer(argv[1],argv[2],atoi(argv[3]));
    return 0;
}
