module nekobox_core

go 1.23.1

require (
	github.com/matsuridayo/libneko v1.0.0 // replaced
	github.com/sagernet/sing-box v1.0.0 // replaced
	grpc_server v1.0.0
)

replace grpc_server => ../../grpc_server

replace github.com/matsuridayo/libneko => ../../../../libneko

replace github.com/sagernet/sing-box => ../../../../sing-box
