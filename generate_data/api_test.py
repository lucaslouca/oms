import grpc
import orchestrator_pb2
import orchestrator_pb2_grpc


def main():
    '''
    $ python3 -m grpc_tools.protoc -I../protos/ --python_out=. --pyi_out=. --grpc_python_out=. ../protos/orchestrator.proto

    https://grpc.io/docs/languages/python/basics/
    '''
    channel = grpc.insecure_channel('localhost:50050')
    stub = orchestrator_pb2_grpc.OrchestratorStub(channel)

    args = orchestrator_pb2.ApiSearchArgs(query_key="a", level=1)
    res = stub.Search(args)
    print(f"Result: {res}")


if __name__ == "__main__":
    main()
