//
// Created by fangzhuhe on 2020/10/16.
//
#pragma once
#ifndef EXCHANGE_EXCHANGE_H
#define EXCHANGE_EXCHANGE_H
#define LIMIT 1000000000
#define MOD_LIMIT 100000
#define CHUNK_NUM 10240
#define THREAD_NUM 4
#define MSG_SIZE 40*1024*1024
#define PER_MSG_SIZE 10*1024*1024

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
  for (auto i = 0; i < size; ++i) {
    ReqChunk *chk = new ReqChunk;
    char *dataChunk = new char[PER_MSG_SIZE];
    chk->set_data(dataChunk, PER_MSG_SIZE);
    chk->set_size(PER_MSG_SIZE);
    chk_list[i] = chk;
  }
  return chk_list;
}
#endif //EXCHANGE_EXCHANGE_H
