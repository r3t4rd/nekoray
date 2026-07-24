module grpc_server

go 1.23.1

require (
	github.com/grpc-ecosystem/go-grpc-middleware v1.4.0
	github.com/matsuridayo/libneko v1.0.0 // replaced
	google.golang.org/grpc v1.73.0
	google.golang.org/protobuf v1.36.6
)

require (
	golang.org/x/net v0.38.0 // indirect
	golang.org/x/sys v0.31.0 // indirect
	golang.org/x/text v0.23.0 // indirect
	google.golang.org/genproto/googleapis/rpc v0.0.0-20250324211829-b45e905df463 // indirect
)

exclude google.golang.org/genproto v0.0.0-20200423170343-7949de9c1215

exclude cloud.google.com/go v0.26.0

replace github.com/matsuridayo/libneko v1.0.0 => ../../../libneko
