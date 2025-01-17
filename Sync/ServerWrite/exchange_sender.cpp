#include <memory>
#include <iostream>
#include <string>
#include <grpcpp/grpcpp.h>
#include <grpc/support/log.h>
#include <thread>
#include <vector>
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
    ExchangeClient(std::shared_ptr<Channel> channel, int client_id)
            : stub_(ExchangeService::NewStub(channel)),client_id_(client_id) {
        chunk_num_ = 0;
    }
    void SendData0() {
      ClientContext context;
      Empty empty;
      for (auto i = 0; i <= MOD_LIMIT; ++i) {
        chunks.push_back(new ReqChunk());
      }
      uint64_t recv_bytes = 0;
      std::unique_ptr<ClientReader<ReqChunk>> reader(
          stub_->ExchangeDataRet(&context, empty));
//      while (1) {
//        bool ret = reader->Read(chunks[chunk_num_ % MOD_LIMIT]);
//        if (!ret)
//          std::cout << "read failed" << endl;
//        recv_bytes += chunks[chunk_num_ % MOD_LIMIT]->ByteSizeLong();
//        if (chunk_num_ % MOD_LIMIT == 0) {
//          cout << client_id_ << " client read chunks = " << chunk_num_
//               << " size = " << recv_bytes << endl;
//        }
//        chunk_num_++;
//      }
      int cnt = 0;
      while (1) {
        bool ret = reader->Read(&chunk_);
        if (!ret) {
          if (cnt == 0) {
            std::cout << "read failed" << endl;
          }
          else {
            std::cout << "read done:"<<cnt << endl;
          }
          break;
        }
        cnt++;
        recv_bytes += chunk_.ByteSizeLong();
        if (chunk_num_ % MOD_LIMIT == 0) {
          cout << client_id_ << " client read chunks = " << chunk_num_
               << " size = " << recv_bytes << endl;
        }
        chunk_num_++;
      }
      Status status = reader->Finish();
      if (status.ok()) {
        std::cout << "Send data rpc succeeded." << std::endl;
      } else {
        std::cout << "send Data rpc failed." << std::endl;
        }
    }

    void SendData() {
      while(true) {
        SendData0();
      }
    }

private:
    std::unique_ptr<ExchangeService::Stub> stub_;
    std::vector<ReqChunk*> chunks;
    ReqChunk chunk_;
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
        grpc::ChannelArguments  channelArgs;
        channelArgs.SetMaxReceiveMessageSize(-1);
        channelArgs.SetMaxSendMessageSize(-1);
        ExchangeClient* new_client =new ExchangeClient(grpc::CreateCustomChannel(addr[i].ip+":"+addr[i].port,
                                              grpc::InsecureChannelCredentials(),channelArgs),i);
        clients.emplace_back(new_client);
        threads.emplace_back(thread(&ExchangeClient::SendData,new_client));
    }

    for(int i=0;i< client_num;++i) {
        threads[i].join();
    }
    return 0;
}