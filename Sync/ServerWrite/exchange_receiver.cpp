//
// Created by fangzhuhe on 2020/10/12.
//
#include <memory>
#include <iostream>
#include <string>
#include <grpcpp/grpcpp.h>
#include <grpc/support/log.h>
#include <thread>
#include <vector>
#include <condition_variable>
#include <cstdlib>
#include "unistd.h"
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
public: explicit ExchangeServiceImp(int msg_size = 0):receive_chunk_num(0), connected_clients_(0){
    chunk_ = GenChunkList(CHUNK_CAP, msg_size);
//        stop_fg= 0;
    }
    ~ExchangeServiceImp() {
//        stop_fg = 1;
//      for(int i=0;i< client_num_;++i) {
//        threads[i].join();
//      }
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

    virtual ::grpc::Status ExchangeDataOnce(::grpc::ServerContext* context, const ::exchange::Empty* request, ::exchange::ReqChunk* response) override {
        SendDatam(response);
        return grpc::Status::OK;
    }
    Status ExchangeDataRet(ServerContext* context, const Empty* request, ServerWriter<ReqChunk>* writer) override {
//        mtx.lock();
        connected_clients_++;
//        mtx.unlock();
        SendData(writer);
        // block for finish
//        std::unique_lock<std::mutex> lck(mtx);
//        cv_finish.wait(lck);
//        while(!stop_fg) sleep(1);
        connected_clients_--;
//        writer->WriteDone
        return Status::OK;
    }
    void SendDatam(::exchange::ReqChunk* response) {
        auto id =receive_chunk_num%CHUNK_CAP;
        if (id < 0) id+=CHUNK_CAP;

        auto ch = chunk_[id];
        response->CopyFrom(*ch);
//          cout<< ch->ByteSizeLong() << " send id = "<< id << " times = " << send_times <<endl;
//        bool ret = writer->Write(*ch);
//        if (!ret) {
//            cout<<"write failed"<<endl;
//            break;
//        }
//        send_times++;
//        receive_chunk_num ++;
//        if(receive_chunk_num % MOD_LIMIT ==0) {
//            cout << "exchange write chunks = "<< receive_chunk_num<<endl;
//        }
    }

    void SendData(ServerWriter<ReqChunk>* writer) {
        uint64_t send_times=0;
        while (true) {
          auto id =receive_chunk_num%CHUNK_CAP;
          if (id < 0) id+=CHUNK_CAP;
          auto ch = chunk_[id];
//          cout<< ch->ByteSizeLong() << " send id = "<< id << " times = " << send_times <<endl;
            bool ret = writer->Write(*ch);
            if (!ret) {
                cout<<"write failed"<<endl;
                break;
            }
            send_times++;
            receive_chunk_num ++;
            if(receive_chunk_num % MOD_LIMIT ==0) {
                cout << "exchange write chunks = "<< receive_chunk_num<<endl;
            }
        }

    }

private:
//    std::atomic_int stop_fg;
//    std::mutex mtx;
//    vector<thread> threads;
//    std::condition_variable cv_finish;
    ReqChunk** chunk_;
//    int chunk_list_size_=MOD_LIMIT;
//    int client_num_;
    std::atomic_int receive_chunk_num;
    atomic_int  connected_clients_;
};
void RunServer(string ip, string port, int msg_size, int numcqs, int minpollers,  int maxpollers) {
    std::string server_address(ip+ ":"+port);
    ExchangeServiceImp service(msg_size);
    ServerBuilder builder;

    // set resource quota
//    grpc::ResourceQuota quota;
//    quota.SetMaxThreads(2);
//    builder.SetResourceQuota(quota);

    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    builder.SetMaxReceiveMessageSize(-1);
    builder.SetMaxSendMessageSize(-1);
    builder.SetSyncServerOption(grpc::ServerBuilder::SyncServerOption::NUM_CQS, numcqs);
    builder.SetSyncServerOption(grpc::ServerBuilder::SyncServerOption::MIN_POLLERS, minpollers);
    builder.SetSyncServerOption(grpc::ServerBuilder::SyncServerOption::MAX_POLLERS, maxpollers);
    std::unique_ptr<Server> server(builder.BuildAndStart());

    std::cout << "Server listening on " << server_address << std::endl;
    server->Wait();
}

int main(int argc, char** argv) {
//    assert(argc == 4);
    int server_cnt = atoi(argv[1]);
    for(int i=0;i<server_cnt;i++) {
        int port = i+ atoi(argv[2]);
        if (argc == 4) RunServer("0.0.0.0", std::to_string(port), atoi(argv[3]), 1, 1, 2);
        if (argc == 7) RunServer("0.0.0.0", std::to_string(port), atoi(argv[3]), atoi(argv[4]), atoi(argv[5]), atoi(argv[6]));
    }
    return 0;
}
