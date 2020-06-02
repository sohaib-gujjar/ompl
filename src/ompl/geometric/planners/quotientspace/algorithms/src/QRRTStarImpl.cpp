/*********************************************************************
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2020, University of Stuttgart
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

#include <ompl/geometric/planners/quotientspace/algorithms/QRRTStarImpl.h>
#include <ompl/geometric/planners/quotientspace/datastructures/PlannerDataVertexAnnotated.h>
#include <ompl/tools/config/SelfConfig.h>
#include <ompl/base/objectives/PathLengthOptimizationObjective.h>
#include <ompl/base/goals/GoalSampleableRegion.h>
#include <boost/foreach.hpp>
#include <boost/math/constants/constants.hpp>
#include "ompl/util/GeometricEquations.h"

#define foreach BOOST_FOREACH

ompl::geometric::QRRTStarImpl::QRRTStarImpl(const base::SpaceInformationPtr &si, BundleSpace *parent_) : BaseT(si, parent_)
{
    setName("QRRTStarImpl" + std::to_string(id_));
    Planner::declareParam<double>(
        "useKNearest_", 
        this, 
        &ompl::geometric::QRRTStarImpl::setKNearest, 
        &ompl::geometric::QRRTStarImpl::getKNearest, 
        "0,1");
    d_ = (double)Bundle->getStateDimension();
    double e = boost::math::constants::e<double>();
    // k > 2^(d + 1) * e * (1 + 1 / d).
    k_rrt_Constant_ = std::pow(2, d_ + 1) * e * (1.0 + 1.0 / d_);
    // γRRG > γRRG ∗ = 2*( 1 + 1/d)^1/d * ( μ( Xfree) / ζd)^1/d
    r_rrt_Constant_ = std::pow(2 * (1.0 + 1.0 / d_) * (getBundle()->getSpaceMeasure() / unitNBallMeasure(d_)), 1.0 / d_);
    symmetric_ = Bundle->getStateSpace()->hasSymmetricInterpolate();
}

ompl::geometric::QRRTStarImpl::~QRRTStarImpl()
{
    deleteConfiguration(xRandom_);
}

void ompl::geometric::QRRTStarImpl::grow()
{
    if (firstRun_)
    {
        goal_ = pdef_->getGoal().get();
        // TODO: sampling depends on boost graph e.g. sampleFromDatastructure
        init();
        firstRun_ = false;
    }

    //(1) Get Random Sample
    sampleBundleGoalBias(xRandom_->state, goalBias_);

    //(2) Get Nearest in Tree
    Configuration *q_nearest = nearestDatastructure_->nearest(xRandom_);

    //(3) Connect Nearest to Random
    double d = distance(q_nearest, xRandom_);
    if (d > maxDistance_)
    {
        Bundle->getStateSpace()->interpolate(q_nearest->state, xRandom_->state, maxDistance_ / d, xRandom_->state);
    }

    //(4) Check if Motion is correct
    if (Bundle->checkMotion(q_nearest->state, xRandom_->state))
    {

        // (5) Add sample
        Configuration *q_new = new Configuration(Bundle, xRandom_->state);

        // Find nearby neighbors of the new motion
        std::vector<Configuration *> nearestNbh;
        
        if (useKNearest_)
        {
            // calculate k
            unsigned int k = std::ceil(k_rrt_Constant_ * log((double) nearestDatastructure_->size()));
            nearestDatastructure_->nearestK(q_new, k, nearestNbh);
        }
        else {
            double r = std::min(maxDistance_, 
                r_rrt_Constant_ * 
                std::pow(log((double) nearestDatastructure_->size()) / (double) nearestDatastructure_->size(), 1 / d_ ));
            nearestDatastructure_->nearestR(q_new, r, nearestNbh);
        }

        // nearest neighbor cost
        ompl::base::Cost nn_line_cost = opt_->motionCost(q_nearest->state, q_new->state);
        ompl::base::Cost nn_cost = opt_->combineCosts(q_nearest->cost, nn_line_cost);
        
        // Find neighbor with minimum Cost
        Configuration *q_min = q_nearest;
        ompl::base::Cost min_line_cost = nn_line_cost;
        ompl::base::Cost min_cost = nn_cost;
        
        // if valid neighbors in first step than no need to check motion in rewiring step for ith neighbor. valid values are {-1, 0, 1}
        int validNeighbor[nearestNbh.size()];

        // store the connection cost for later use, if space is symmetric
        std::vector<ompl::base::Cost> lineCosts;

        if (symmetric_)
        {
            lineCosts.resize(nearestNbh.size());
        }

        for(unsigned int i=0; i< nearestNbh.size(); i++)
        {
            Configuration* q_near = nearestNbh.at(i);
            if (q_near->isGoal || q_near->isStart) continue;

            // nearest neighbor
            if (q_nearest == q_near)
            {
                validNeighbor[i] = 1;
                if (symmetric_) {
                    lineCosts[i] = nn_line_cost;
                }
                continue;
            }
            validNeighbor[i] = 0;

            ompl::base::Cost line_cost = opt_->motionCost(q_near->state, q_new->state);
            ompl::base::Cost new_cost = opt_->combineCosts(q_near->cost, line_cost);
            
            if (symmetric_)
            {
                lineCosts[i] = line_cost;
            }

            if (opt_->isCostBetterThan(new_cost , min_cost))
            {
                if((!useKNearest_ || distance(q_near, q_new) < maxDistance_) && Bundle->checkMotion(q_near->state, q_new->state))
                {
                    q_min = q_near;
                    min_line_cost = line_cost;
                    min_cost = new_cost;
                    validNeighbor[i] = 1;
                }
                else validNeighbor[i] = -1;
            }
        }


        // (6) add edge assign cost

        q_new->lineCost = min_line_cost;
        q_new->cost = min_cost;
        q_new->parent = q_min;
        q_min->children.push_back(q_new);

        // TODO: sampling depends on graph e.g. sampleFromDatastructure
        addConfiguration(q_new);addEdge(q_min->index, q_new->index);//nearestDatastructure_->add(q_new);
        
        // (7) Rewire the tree
        bool rewired = false;
        for (unsigned int i=0 ; i< nearestNbh.size(); i++)
        {
            Configuration* q_near = nearestNbh.at(i);
            
            if (q_near != q_new->parent && !q_near->isGoal && !q_near->isStart)
            {
                base::Cost line_cost;
                if(symmetric_) {
                    line_cost = lineCosts[i];
                }
                else {
                    line_cost = opt_->motionCost(q_new->state, q_near->state);
                }
                base::Cost new_cost = opt_->combineCosts(q_new->cost, line_cost);
                
                if (opt_->isCostBetterThan(new_cost, q_near->cost))
                {
                    bool valid = (validNeighbor[i] == 1);
                    // check neighbor validity if it wasn´t checked before
                    if (validNeighbor[i] == 0)
                    {
                        valid = ((!useKNearest_ || distance(q_near, q_new) < maxDistance_) && Bundle->checkMotion(q_near->state, q_new->state));
                    }
                    if (valid)
                    {
                        // remove node from children of its parent node
                        for (auto it = q_near->parent->children.begin(); it != q_near->parent->children.end(); ++it)
                        {
                            if (*it == q_near)
                            {
                                q_near->parent->children.erase(it);
                                break;
                            }
                        }

                        // TODO: sampling depends on graph e.g. sampleFromDatastructure
                        addEdge(q_new->index, q_near->index);
                        
                        // update node parent
                        q_near->parent = q_new;
                        q_near->lineCost = line_cost;
                        q_near->cost = new_cost;
                        q_near->parent->children.push_back(q_near);
                        
                        // update node's children costs
                        updateChildCosts(q_near);
                        rewired = true;
                    }
                }
            }
        }
        
        // (7) check if this sample satisfies the goal

        double dist = 0.0;
        bool satisfied = goal_->isSatisfied(q_new->state, &dist);
        if (satisfied)
        {
            goalConfigurations_.push_back(q_new);

            // check if new state cost is better then previously satisfied cost
            if (opt_->isCostBetterThan(q_new->cost, bestCost_))
            {
                qGoal_->parent = q_new;
                bestGoalConfiguration_ = q_new;
                bestCost_ = bestGoalConfiguration_->cost;
            }
            hasSolution_ = true;
        }

        // update cost if tree is rewired
        if (rewired && !goalConfigurations_.empty())
        {
            for (unsigned int i = 0; i < goalConfigurations_.size(); i++)
            {
                if (opt_->isCostBetterThan(goalConfigurations_.at(i)->cost, bestCost_))
                {
                    bestGoalConfiguration_ = goalConfigurations_.at(i);
                    bestCost_ = bestGoalConfiguration_->cost;
                }
            }
        }
    }
}

void ompl::geometric::QRRTStarImpl::updateChildCosts(Configuration *q)
{
    for (std::size_t i = 0; i < q->children.size(); ++i)
    {
        q->children[i]->cost = opt_->combineCosts(q->cost, q->children[i]->lineCost);
        updateChildCosts(q->children[i]);
    }
}

bool ompl::geometric::QRRTStarImpl::getSolution(base::PathPtr &solution)
{
    if (hasSolution_)
    {
        auto path(std::make_shared<PathGeometric>(getBundle()));
        path->append(qGoal_->state);
        
        Configuration *intermediate_node = bestGoalConfiguration_;
        
        while (intermediate_node->parent != nullptr)
        {
            path->append(intermediate_node->state);
            intermediate_node = intermediate_node->parent;
        }
        path->reverse();
        solution = path;
        return true;
    }
    else
    {
      return false;
    }
}

void ompl::geometric::QRRTStarImpl::getPlannerData(base::PlannerData &data) const
{
    OMPL_DEBUG("Roadmap has %d vertices", nearestDatastructure_->size());
    
    std::vector<int> idxPathI;
    BundleSpace *pparent = getParent(); 
    while (pparent != nullptr)
    {
        idxPathI.push_back(0);
        pparent = pparent->getParent();
    }
    idxPathI.push_back(0);


    std::vector<Configuration *> motions;
    if (nearestDatastructure_)
        nearestDatastructure_->list(motions);

    if (bestGoalConfiguration_){
        base::PlannerDataVertexAnnotated pgoal(bestGoalConfiguration_->state, 0);
        pgoal.setPath(idxPathI);
        data.addGoalVertex(pgoal);
    }
    
    base::PlannerDataVertexAnnotated pstart(qStart_->state, 0);
    pstart.setPath(idxPathI);
    data.addStartVertex(pstart);
    
    for (unsigned int i = 0; i < motions.size(); i++)
    {
        if (motions.at(i)->parent != nullptr)
        {
            base::PlannerDataVertexAnnotated p1(motions.at(i)->parent->state);
            base::PlannerDataVertexAnnotated p2(motions.at(i)->state);
            p1.setPath(idxPathI);
            p2.setPath(idxPathI);

            data.addVertex(p1);
            data.addVertex(p2);
            data.addEdge(p1, p2);

            p1.setComponent(2);
            p2.setComponent(2);
        }
    }
}