/*********************************************************************
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2019, University of Stuttgart
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of the University of Stuttgart nor the names
 *     of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written
 *     permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *********************************************************************/

/* Author: Andreas Orthey, Sohaib Akbar */

#include <ompl/geometric/planners/quotientspace/algorithms/QMPImpl.h>
#include <ompl/tools/config/SelfConfig.h>
#include <boost/foreach.hpp>
#include <ompl/datastructures/NearestNeighbors.h>
#include "ompl/datastructures/PDF.h"

#define foreach BOOST_FOREACH

ompl::geometric::QMPImpl::QMPImpl(const base::SpaceInformationPtr &si, BundleSpace *parent_) : BaseT(si, parent_)
{
    setName("QMPImpl" + std::to_string(id_));

    // setMetric("geodesic");
    setMetric("shortestpath");
    setGraphSampler("randomedge");
    setImportance("exponential");

    randomWorkStates_.resize(5);
    getBundle()->allocStates(randomWorkStates_);
}

ompl::geometric::QMPImpl::~QMPImpl()
{
    getBundle()->freeStates(randomWorkStates_);
}

unsigned int ompl::geometric::QMPImpl::computeK()
{
    return k_NearestNeighbors_;
}

void ompl::geometric::QMPImpl::grow()
{
    if (firstRun_)
    {
        init();
        vGoal_ = addConfiguration(qGoal_);
        firstRun_ = false;
    }

    //(1) Get Random Sample
    sampleBundle(xRandom_->state);

    if(!getBundle()->getStateValidityChecker()->isValid(xRandom_->state)) return;

    //(2) Add Configuration if valid
    Configuration *xNew = new Configuration(getBundle(), xRandom_->state);
    addConfiguration(xNew);
    
    //(3) Connect to K nearest neighbors
    std::vector<Configuration*> nearestNeighbors;

    BaseT::nearestDatastructure_->nearestK(xNew, computeK(), nearestNeighbors);

    for(unsigned int k = 0 ; k < nearestNeighbors.size(); k++)
    {
        Configuration* xNearest = nearestNeighbors.at(k);

        xNew->total_connection_attempts++;
        xNearest->total_connection_attempts++;

        const Configuration *xNext = extendGraphTowards(xNearest, xNew);

        if(xNext)
        {
            xNew->successful_connection_attempts++;
            xNearest->successful_connection_attempts++;

            if (!hasSolution_)
            {
                if (sameComponent(vStart_, vGoal_))
                {
                    hasSolution_ = true;
                }
            }
        }

    }
}

// void ompl::geometric::QMPImpl::expand()
// {
//     PDF pdf;

//     foreach (Vertex v, boost::vertices(graph_))
//     {
//         const unsigned long int t = graph_[v]->total_connection_attempts;
//         pdf.add(graph_[v], (double)(t - graph_[v]->successful_connection_attempts) / (double)t);
//     }

//     if (pdf.empty())
//         return;
    
//     Configuration *q = pdf.sample(rng_.uniform01());

//     int s = getBundle()->randomBounceMotion(Bundle_sampler_, q->state, randomWorkStates_.size(), randomWorkStates_, false);
//     if(s > 0)
//     {
//         Configuration *prev = q;
//         Configuration *last = addMileStone(randomWorkStates_[--s]);
//         for (int i = 0; i < s; i++)
//         {
//             Configuration *tmp = new Configuration(getBundle(), randomWorkStates_[i]);
//             addConfiguration(tmp);

//             ompl::geometric::BundleSpaceGraph::addEdge(prev->index, tmp->index);
//             prev = tmp;
//         }
//         if(!sameComponent(prev->index, last->index))
//             ompl::geometric::BundleSpaceGraph::addEdge(prev->index, last->index);
//     }
// }

// ompl::geometric::BundleSpaceGraph::Configuration *ompl::geometric::QMPImpl::addMileStone(ompl::base::State *q_state)
// {
//     // add sample
//     Configuration *q_next = new Configuration(getBundle(), q_state);
//     Vertex v_next = addConfiguration(q_next);
    
//     // check for close k neibhors
//     std::vector<Configuration*> nearestNeighbors;
//     BaseT::nearestDatastructure_->nearestK(q_next, k_, nearestNeighbors);

//     for(unsigned int i=0 ; i< nearestNeighbors.size(); i++)
//     {
//         Configuration* q_neighbor = nearestNeighbors.at(i);

//         // q_next->total_connection_attempts++;
//         // q_neighbor->total_connection_attempts++;

//         if (getBundle()->checkMotion(q_neighbor->state, q_next->state)) 
//         {
//             // addEdge(q_neighbor->index, v_next);
            
//             // q_next->successful_connection_attempts++;
//             // q_neighbor->successful_connection_attempts++;

//             if (!hasSolution_)
//             {
//                 if (sameComponent(vStart_, vGoal_))
//                 {
//                     hasSolution_ = true;
//                 }
//             }
//         }

//     }
//     return q_next;
// }
