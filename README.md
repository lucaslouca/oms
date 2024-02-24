## Development Environment Setup

### Kafka

```bash
docker compose up
```

### Install yaml-cpp (for config file)

```bash
curl https://github.com/jbeder/yaml-cpp/archive/refs/tags/yaml-cpp-0.7.0.tar.gz -o yaml-cpp.tar.gz  && tar -xvf yaml-cpp.tar.gz && cd yaml-cpp && mkdir build && cd build && cmake .. && make && sudo make install
```

### Install spdlog (for logging)

```bash
git clone https://github.com/gabime/spdlog.git && cd spdlog && mkdir build && cd build && cmake .. && make -j && sudo make install
```

### Install librdkafka (for kafka)

```bash
git clone https://github.com/edenhill/librdkafka && cd librdkafka && ./configure --install-deps && make && sudo make install
```

### gRPC Stubs

```bash
$ ./make-proto
# Stubs will be located under src/build/
```

## How to run

### Start workers

```bash
./src/build/graph_worker -c configs/worker_A.yaml
./src/build/graph_worker -c configs/worker_B.yaml
```

### Run orchestrator

```bash
./src/build/main -c configs/orchestrator.yaml
```
