/*PGR-GNU*****************************************************************
File: pgr_ksp.hpp

Copyright (c) 2015 Celia Virginia Vergara Castillo
Mail: vicky_vergara@hotmail.com

------

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

********************************************************************PGR-GNU*/

#ifndef INCLUDE_YEN_PGR_KSP_HPP_
#define INCLUDE_YEN_PGR_KSP_HPP_
#pragma once

#include "dijkstra/pgr_dijkstra.hpp"

#include <sstream>
#include <deque>
#include <vector>
#include <algorithm>
#include <set>
#include <limits>

#include "cpp_common/pgr_assert.h"
#include "cpp_common/compPaths.h"
#include "cpp_common/pgr_messages.h"
#include "cpp_common/basePath_SSEC.hpp"

namespace pgrouting {
namespace yen {

template < class G >
class Pgr_ksp :  public Pgr_messages {
 public:
     std::deque<Path> Yen(
             G &graph,
             int64_t  start_vertex,
             int64_t end_vertex,
             int K,
             bool heap_paths) {
         /*
          * No path: already in destination
          */
         if ((start_vertex == end_vertex) || (K == 0)) {
             return std::deque<Path>();
         }
         /*
          * no path: disconnected vertices
          */
         if (!graph.has_vertex(start_vertex)
                 || !graph.has_vertex(end_vertex)) {
             return std::deque<Path>();
         }
         m_ResultSet.clear();
         m_Heap.clear();

         v_source = graph.get_V(start_vertex);
         v_target = graph.get_V(end_vertex);
         m_start = start_vertex;
         m_end = end_vertex;
         executeYen(graph, K);

         while (!m_ResultSet.empty()) {
             m_Heap.insert(*m_ResultSet.begin());
             m_ResultSet.erase(m_ResultSet.begin());
         }
         std::deque<Path> l_ResultList(m_Heap.begin(), m_Heap.end());

         std::stable_sort(l_ResultList.begin(), l_ResultList.end(),
                 [](const Path &left, const Path &right) -> bool {
                 for (size_t i = 0;
                         i < (std::min)(left.size(), right.size());
                         ++i) {
                 if (left[i].node < right[i].node) return true;
                 if (left[i].node > right[i].node) return false;
                 }
                 return false;
                 });

         std::stable_sort(l_ResultList.begin(), l_ResultList.end(),
                 [](const Path &left, const Path &right) {
                 return left.size() < right.size();});

         if (!heap_paths && l_ResultList.size() > (size_t) K)
             l_ResultList.resize(K);

         return l_ResultList;
     }

     void clear() {
         m_Heap.clear();
         m_ResultSet.clear();
     }


 private:

     //! the actual algorithm
     void executeYen(G &graph, int K) {
         clear();
         curr_result_path = getFirstSolution(graph);

         if (m_ResultSet.size() == 0) return;  // no path found

         while (m_ResultSet.size() < (unsigned int) K) {
             doNextCycle(graph);
             if (m_Heap.empty()) break;
             curr_result_path = *m_Heap.begin();
             m_ResultSet.insert(curr_result_path);
             m_Heap.erase(m_Heap.begin());
             /*
              * without the next line withpointsKSP hungs with:
              *  c++ 4.6
              *  Debug mode
              */
#ifndef NDEBUG
             log << "end of while heap size" << m_Heap.size();
#endif
         }
     }

     /** @name Auxiliary function for yen's algorithm */
     ///@{

     //! Performs the first Dijkstra of the algorithm
 protected:
     Path getFirstSolution(G &graph) {
         Path path;

         Pgr_dijkstra< G > fn_dijkstra;
         path = fn_dijkstra.dijkstra(graph, m_start, m_end);

         if (path.empty()) return path;
         m_ResultSet.insert(path);
         return path;
     }

     virtual void on_insert_to_heap(const Path) {
     };

     //! Performs the next cycle of the algorithm
     void doNextCycle(G &graph) {
         int64_t spurNodeId;


         for (unsigned int i = 0; i < curr_result_path.size(); ++i) {
             spurNodeId = curr_result_path[i].node;

             auto rootPath = curr_result_path.getSubpath(i);

             for (const auto &path : m_ResultSet) {
                 if (path.isEqual(rootPath)) {
                     if (path.size() > i + 1) {
                         graph.disconnect_edge(path[i].node,     // from
                                 path[i + 1].node);  // to
                     }
                 }
             }

             removeVertices(graph, rootPath);

             Pgr_dijkstra< G > fn_dijkstra;
             auto spurPath = fn_dijkstra.dijkstra(graph, spurNodeId, m_end);

             if (spurPath.size() > 0) {
                 rootPath.appendPath(spurPath);
                 m_Heap.insert(rootPath);
                 on_insert_to_heap(rootPath);
             }

             graph.restore_graph();
         }
     }

 protected:
     //! stores in subPath the first i elements of path
     void removeVertices(G &graph, const Path &subpath) {
         for (const auto &e : subpath)
             graph.disconnect_vertex(e.node);
     }

     ///@}

 protected:
     /** @name members */
     ///@{
     typedef typename G::V V;
     V v_source;  //!< source descriptor
     V v_target;  //!< target descriptor
     int64_t m_start;  //!< source id
     int64_t m_end;   //!< target id

     Path curr_result_path;  //!< storage for the current result

     typedef std::set<Path, compPathsLess> pSet;
     pSet m_ResultSet;  //!< ordered set of shortest paths
     pSet m_Heap;  //!< the heap

     //std::ostringstream log;
};


}  // namespace yen
}  // namespace pgrouting

#endif  // INCLUDE_YEN_PGR_KSP_HPP_
