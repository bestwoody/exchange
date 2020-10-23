//
// Created by fangzhuhe on 2020/10/12.
//
#include <memory>
#include <iostream>
#include <string>
#include <grpcpp/grpcpp.h>
#include <grpc/support/log.h>
#include <thread>
#include <condition_variable>
#include "../../exchange.grpc.pb.h"
#include "../../exchange.h"
#include <grpcpp/resource_quota.h>

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
public: explicit ExchangeServiceImp(int client_num):client_num_(client_num),receive_chunk_num(0), connected_clients_(0){
    chunk_ = GenChunk(0);
        th = thread(&ExchangeServiceImp::SendData, this);
    }
    ~ExchangeServiceImp() {
        th.join();
    }
/*    Status ExchangeDataRet(ServerContext* context, const Empty* request, ServerWriter<ReqChunk>* writer) override {
        while (writer->Write(*chunk_)) {
            receive_chunk_num++;
            if(receive_chunk_num % MOD_LIMIT ==0) {
                cout << "exchange write chunks = "<< receive_chunk_num<<endl;
            }
        }
        return Status::OK;
    }*/
    Status ExchangeDataRet(ServerContext* context, const Empty* request, ServerWriter<ReqChunk>* writer) override {
        mtx.lock();
        writers.emplace_back(writer);
        connected_clients_++;
        mtx.unlock();
        if (connected_clients_ >= client_num_) {
            cv.notify_all();
        }
        // block for finish
        std::unique_lock<std::mutex> lck(mtx);
        cv_finish.wait(lck);
        return Status::OK;
    }
    void SendData() {
        std::unique_lock<std::mutex> lck(mtx);
        cv.wait(lck ,[&] {return connected_clients_ >= client_num_;});
        while (true) {
            for(auto i= 0 ;i< connected_clients_;++i) {
                writers[i]->Write(*chunk_);
            }
            receive_chunk_num ++;
            if(receive_chunk_num % MOD_LIMIT ==0) {
                cout << "exchange write chunks = "<< receive_chunk_num<<endl;
            }
        }

    }

private:
    std::mutex mtx;
    thread th;
    std::condition_variable cv, cv_finish;
    ReqChunk* chunk_;
    int client_num_;
    std::atomic_int receive_chunk_num;
    vector<ServerWriter<ReqChunk>* > writers;
    atomic_int  connected_clients_;
};
void RunServer(string ip, string port, int client_num) {
    std::string server_address(ip+ ":"+port);
    ExchangeServiceImp service(client_num);
    ServerBuilder builder;

    // set resource quota
//    grpc::ResourceQuota quota;
//    quota.SetMaxThreads(2);
//    builder.SetResourceQuota(quota);

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
