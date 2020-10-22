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

class ExchangeServiceImp final : public ExchangeService::Service {
public: explicit ExchangeServiceImp(int client_num):client_num_(client_num),receive_chunk_num(0){}
    Status ExchangeData(ServerContext* context, ServerReader< ReqChunk>* reader,ReplySummary* response) override {
        ReqChunk chunk;
        while (reader->Read(&chunk)) {
            receive_chunk_num++;
            if(receive_chunk_num % MOD_LIMIT ==0) {
                cout << "exchange receives chunks = "<< receive_chunk_num<<endl;
            }
        }
        return Status::OK;
    }

private:
    std::atomic_int receive_chunk_num;
    int client_num_;
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
    RunServer(argv[1],argv[2],atoi(argv[3]));
    return 0;
}
