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

/* Author: Andreas Orthey */

#ifndef OMPL_GEOMETRIC_PLANNERS_BUNDLESPACE_BUNDLESPACESEQUENCE_
#define OMPL_GEOMETRIC_PLANNERS_BUNDLESPACE_BUNDLESPACESEQUENCE_
#include <ompl/geometric/planners/multilevel/datastructures/BundleSpace.h>
#include <type_traits>
#include <queue>

namespace ompl
{
    namespace geometric
    {
        /** \brief A planner for a sequence of BundleSpaces

             Example usage with QRRT
             ompl::base::PlannerPtr planner =
                 std::make_shared<BundleSpaceSequence<ompl::geometric::QRRT> >(si_vec); 

             whereby si_vec is of type std::vector<ompl::base::SpaceInformationPtr>
         */

        template <class T>
        class BundleSpaceSequence : public ompl::base::Planner
        {
            using BaseT = ompl::base::Planner;
            static_assert(std::is_base_of<BundleSpace, T>::value, 
                "Template must inherit from BundleSpace");

        public:
            /** \brief Constructor taking a sequence of ompl::base::SpaceInformationPtr
                 and computing the BundleSpaces for each pair in the sequence */
            BundleSpaceSequence(
                std::vector<ompl::base::SpaceInformationPtr> &siVec, 
                std::string type = "BundleSpacePlanner");
            BundleSpaceSequence(ompl::base::SpaceInformationPtr si) = delete;
            BundleSpaceSequence(ompl::base::SpaceInformationPtr si, std::string type) = delete;

            virtual ~BundleSpaceSequence();

            /** \brief Return annotated vertices (with information about BundleSpace level) */
            void getPlannerData(ompl::base::PlannerData &data) const override;

            ompl::base::PlannerStatus solve(const ompl::base::PlannerTerminationCondition &ptc) override;
            void setup() override;
            void clear() override;
            virtual void setProblemDefinition(const ompl::base::ProblemDefinitionPtr &pdef) override;
            const ompl::base::ProblemDefinitionPtr &getProblemDefinition(unsigned int kBundleSpace) const;

            /** \brief Number of BundleSpaces */
            int getLevels() const;

            /** \brief Get all dimensions of the BundleSpaces in the sequence */
            std::vector<int> getDimensionsPerLevel() const;

            void setStopLevel(unsigned int level_);

            // for benchmarks
            void setMetric(const std::string& sMetric);
            void setImportance(const std::string& sImportance);
            void setGraphSampler(const std::string& sGraphSampler);
            void setK(unsigned int k);
            void setFeasiblePathRestriction(bool val);

        protected:

            /** \brief Solution paths on each BundleSpace (TODO: put into pdefs)*/
            std::vector<ompl::base::PathPtr> solutions_;

            /** \brief Sequence of BundleSpaces */
            std::vector<BundleSpace *> bundleSpaces_;

            /** \brief Indicator if a solution has been found on the current BundleSpaces */
            bool foundKLevelSolution_{false};

            /** \brief Current level on which we have not yet found a path */
            unsigned int currentBundleSpaceLevel_{0};

            /** \brief \brief Sometimes we only want to plan until a certain BundleSpace
                level (for debugging for example). This variable sets the stopping
                level. */
            unsigned int stopAtLevel_;

            /** \brief Each BundleSpace has a unique ompl::base::SpaceInformationPtr */
            std::vector<ompl::base::SpaceInformationPtr> siVec_;

            /** \brief Compare function for priority queue */
            struct CmpBundleSpacePtrs
            {
                // ">" operator: smallest value is top in queue
                // "<" operator: largest value is top in queue (default)
                bool operator()(const BundleSpace *lhs, const BundleSpace *rhs) const
                {
                    return lhs->getImportance() < rhs->getImportance();
                }
            };
            /** \brief \brief Priority queue of BundleSpaces which keeps track of how often
                every graph on each space has been expanded. */
            typedef std::priority_queue<BundleSpace *, std::vector<BundleSpace *>, CmpBundleSpacePtrs>
                BundleSpacePriorityQueue;
            BundleSpacePriorityQueue priorityQueue_;
        };
    }  // namespace geometric
}  // namespace ompl
#include "BundleSpaceSequenceImpl.h"
#endif
