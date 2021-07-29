//
// Created by fangzhuhe on 2020/10/16.
//
#pragma once
#ifndef EXCHANGE_EXCHANGE_H
#define EXCHANGE_EXCHANGE_H
#define LIMIT 1000000000
#define MOD_LIMIT 1000
#define CHUNK_NUM 10240
#define THREAD_NUM 4
#define MSG_SIZE 4*1024*1024
#define PER_MSG_SIZE 1*1024*1024

#include <string>
#include <grpcpp/grpcpp.h>
#include <thread>
#include <stdlib.h>
#include "exchange.grpc.pb.h"
using exchange::ReqChunk;
struct ServerAddr{
    std::string ip;
    std::string port;
}addr[100];

ReqChunk* GenChunk(uint64_t size) {
    ReqChunk* chk = new ReqChunk;
    char* dataChunk = new char[PER_MSG_SIZE];
    chk->set_data(dataChunk,PER_MSG_SIZE);
    chk->set_size(PER_MSG_SIZE);
    return chk;
}

ReqChunk** GenChunkList(uint64_t size) {
  ReqChunk **chk_list = new ReqChunk*[size];
  uint64_t msg_size = 0;
  for (auto i = 0; i < size; ++i) {
    ReqChunk *chk = new ReqChunk;
    msg_size = MSG_SIZE + PER_MSG_SIZE + std::abs(rand()) % MSG_SIZE;
    char *dataChunk = (char*)malloc(msg_size+1);
    memset(dataChunk,0,msg_size+1);
    memset(dataChunk,1,msg_size-i);
    chk->set_data(dataChunk, msg_size);
    chk->set_size(msg_size);
    chk_list[i] = chk;
  }
  cout<<"generate chunk size= "<< size << std::endl;
  return chk_list;
}
#endif //EXCHANGE_EXCHANGE_H
