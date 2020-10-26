 ## usage
 Shuffle data among servers in sync or async. 
 
 ## build command (in the root fold)

######  Mac 
 
 mkdir build && cd build
 
 cmake -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl .. && make -j4

######  Centos
 
 mkdir build && cd build
 
 cmake && make -j4

 ## start up
 
 ./ASRreceiver 172.16.5.85 50000 1
 
 ./ASRsender 2 172.16.5.59 50000 172.16.5.81 50000 10
 
 ## optional to generate protobuf files
`` protoc -I ../protos --grpc_out=.  --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` ../protos/exchange.proto
 
 protoc -I ../protos --cpp_out=.   ../protos/exchange.proto``