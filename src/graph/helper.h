/*
 *
 * Copyright 2015 gRPC authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifndef GRPC_COMMON_CPP_GRAPH_HELPER_H_
#define GRPC_COMMON_CPP_GRAPH_HELPER_H_

#include <string>
#include <vector>

#include "../grpc/graph.grpc.pb.h"

template <typename VERTEX_DATA, typename EDGE_DATA>
class InMemoryGraph;

namespace graph {
std::string GetDbFileContent(const std::string& db_path);
void ParseArgs(int argc, char* argv[], std::string& config_file);
void ParseDb(const std::string& db, InMemoryGraph<std::string, std::string>* graph);
bool operator<(const Edge& lhs, const Edge& rhs);
bool operator<(const Vertex& lhs, const Vertex& rhs);
}  // namespace graph

#endif
