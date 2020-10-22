//
// Created by fangzhuhe on 2020/10/16.
//
#pragma once
#ifndef EXCHANGE_EXCHANGE_H
#define EXCHANGE_EXCHANGE_H
#define LIMIT 1000000000
#define MOD_LIMIT 100
#define CHUNK_NUM 10240
#define THREAD_NUM 4
#include <string>
struct ServerAddr{
    std::string ip;
    std::string port;
}addr[100];


#endif //EXCHANGE_EXCHANGE_H
