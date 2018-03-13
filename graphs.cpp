/*
Copyright 2013-present Barefoot Networks, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include <boost/graph/graphviz.hpp>

#include "lib/log.h"
#include "lib/error.h"
#include "lib/exceptions.h"
#include "lib/gc.h"
#include "lib/crash.h"
#include "lib/nullstream.h"

#include "graphs.h"

namespace multip4 {

Graphs::vertex_t Graphs::add_vertex(const cstring &name, VertexType type) {
    auto v = boost::add_vertex(g);
    boost::put(&Vertex::name, g, v, name);
    boost::put(&Vertex::type, g, v, type);
    return g.local_to_global(v);
}

void Graphs::add_edge(const vertex_t &from, const vertex_t &to, const cstring &name) {
    auto ep = boost::add_edge(from, to, g);
    boost::put(boost::edge_name, g, ep.first, name);
}

void Graphs::writeGraphToFile(const cstring &name) {
  GraphAttributeSetter()(g);
  auto path = name + ".dot";
  auto out = openFile(path, false);
  if (out == nullptr) {
    ::error("Failed to open file %1%", path);
    return;
  }
  boost::write_graphviz(*out, g);
}


}  // namespace graphs
