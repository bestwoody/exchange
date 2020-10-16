 ## usage
 shuffle data among servers.
 
 ##command (in each fold)
 
`` cmake -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl ..
 
 protoc -I ../protos --grpc_out=.  --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` ../protos/exchange.proto
 
 protoc -I ../protos --cpp_out=.   ../protos/exchange.proto``