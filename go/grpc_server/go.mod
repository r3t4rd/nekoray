module grpc_server

go 1.23.1

require (
	github.com/grpc-ecosystem/go-grpc-middleware v1.4.0
	github.com/matsuridayo/libneko v1.0.0 // replaced
	google.golang.org/grpc v1.73.0
	google.golang.org/protobuf v1.36.6
)

replace github.com/matsuridayo/libneko v1.0.0 => ../../../libneko
