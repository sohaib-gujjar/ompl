#pragma once

#include "PathVisibilityChecker.h"
#include <ompl/geometric/planners/quotientspace/datastructures/QuotientSpaceGraph.h>
#include <ompl/datastructures/NearestNeighbors.h>
#include <ompl/util/RandomNumbers.h>
#include <ompl/geometric/PathGeometric.h>
#include "ompl/geometric/PathSimplifier.h"

#include <boost/graph/subgraph.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/random.hpp> 
#include <boost/random/linear_congruential.hpp>
#include <boost/random/variate_generator.hpp>

namespace ob = ompl::base;
namespace og = ompl::geometric;

namespace ompl
{
  namespace geometric
  {
    class QuotientSpaceGraphSparse: public ompl::geometric::QuotientSpaceGraph{

        using BaseT = ompl::geometric::QuotientSpaceGraph;
      public:

        QuotientSpaceGraphSparse(const ob::SpaceInformationPtr &si, QuotientSpace *parent = nullptr);
        virtual ~QuotientSpaceGraphSparse() override;

        virtual void grow() = 0;
        virtual bool getSolution(ompl::base::PathPtr &solution) override;
        
        virtual void getPlannerData(ob::PlannerData &data) const override;
        void getPlannerDataRoadmap(ob::PlannerData &data, std::vector<int> pathIdx) const;

        virtual void deleteConfiguration(Configuration *q);
        Vertex addConfigurationSparse(Configuration *q);
        void addEdgeSparse(const Vertex a, const Vertex b);

        // void AddEdge(const Configuration* q1, const Configuration* q2);
        virtual void setup() override;
        virtual void clear() override;
        void TestVisibilityChecker();

        virtual void Init();
        //Copied from SPARS
        void findGraphNeighbors(Configuration *q, std::vector<Configuration*> &graphNeighborhood,
                                std::vector<Configuration*> &visibleNeighborhood);
        bool checkAddCoverage(Configuration *q, std::vector<Configuration*> &visibleNeighborhoods);
        bool checkAddConnectivity(Configuration* q, std::vector<Configuration*> &visibleNeighborhood);
        bool checkAddInterface(Configuration *q, std::vector<Configuration*> &graphNeighborhood, std::vector<Configuration*> &visibleNeighborhood);
        bool checkAddPath(Configuration *q);

        void updateRepresentatives(Configuration *q);
        void getInterfaceNeighborRepresentatives(Configuration *q, std::set<Vertex> &interfaceRepresentatives);
        void addToRepresentatives(Vertex q, Vertex rep, const std::set<Vertex> &interfaceRepresentatives);
        void removeFromRepresentatives(Configuration *q);
        
        void getInterfaceNeighborhood(Configuration *q, std::vector<Vertex> &interfaceNeighborhood);
        void computeVPP(Vertex v, Vertex vp, std::vector<Vertex> &VPPs);
        void computeX(Vertex v, Vertex vp, Vertex vpp, std::vector<Vertex> &Xs);
        Vertex getInterfaceNeighbor(Vertex q, Vertex rep);
        void computeDensePath(const Vertex &start, const Vertex &goal, std::deque<base::State *> &path);
        bool addPathToSpanner(const std::deque<base::State *> &dense_path, Vertex vp, Vertex vpp);

        void updatePairPoints(Configuration *q);

        // void findCloseRepresentatives(base::State *workArea, Configuration *q,
        //                                                          const Configuration *qRep,
        //                                                          std::map<Vertex, base::State *> &closeRepresentatives);
        // void computeVPP(Vertex v, Vertex vp, std::vector<Vertex> &VPPs);

        void Rewire(Vertex &v);
        void Rewire();
        void printAllPathsUtil(Vertex u, Vertex d, bool visited[], int path[], int &path_index);
        virtual void enumerateAllPaths();
        void removeReducibleLoops();
        void removeEdgeIfReductionLoop(const Edge &e);
        unsigned int getNumberOfPaths() const;
        const std::vector<ob::State*> getKthPath(uint k) const;
        void getPathIndices(const std::vector<ob::State*> &states, std::vector<int> &idxPath) const;
        bool isProjectable(const std::vector<ob::State*> &pathQ1) const;
        int getProjectionIndex(const std::vector<ob::State*> &pathQ1) const;

        int selectedPath{-1}; //selected path to sample from (if children try to sample this space)
        bool sampleQuotient(ob::State *q_random_graph) override;

        PathVisibilityChecker* getPathVisibilityChecker();
        void debugInvalidState(const ob::State *s);

        void pushPathToStack(std::vector<ob::State*> &path);
        void removeLastPathFromStack();

        virtual void print(std::ostream &out) const override;
        bool hasSparseGraphChanged();

        virtual const Configuration *nearest(const Configuration *s) const;
        void freePath(std::vector<ob::State*> path, const ob::SpaceInformationPtr &si) const;
        std::vector<ob::State*> getProjectedPath(std::vector<ob::State*> pathQ1, const ob::SpaceInformationPtr &si) const;
    protected:

        double sparseDelta_{0.};
        double denseDelta_{0.}; //delta for dense graph -> move this QuotientSpaceGraph.h
        double sparseDeltaFraction_{0.15};
        double denseDeltaFraction_{0.05};
        double pathBias_{0.};
        double pathBiasFraction_{0.05};
        double kPRMStarConstant_;
        unsigned Nold_v{0};
        unsigned Nold_e{0};

        void setSparseDeltaFraction(double D)
        {
            sparseDeltaFraction_ = D;
        }
        double getSparseDeltaFraction() const
        {
            return sparseDeltaFraction_;
        }
        uint numberVertices{0};

        unsigned numberOfFailedAddingPathCalls{0};
        unsigned Nhead{7}; //head -nX (to display only X top paths)
        std::vector<og::PathGeometric> pathStack_;
        std::vector<std::vector<ob::State*>> pathStackHead_;
        void PrintPathStack();

        std::vector<int> GetSelectedPathIndex() const;

        virtual void uniteComponentsSparse(Vertex m1, Vertex m2);
        bool sameComponentSparse(Vertex m1, Vertex m2);
        // boost::disjoint_sets<boost::associative_property_map<std::map<Vertex, VertexRank> >, boost::associative_property_map<std::map<Vertex, Vertex> > > 
        //   disjointSetsSparse_{boost::make_assoc_property_map(vrank), boost::make_assoc_property_map(vparent)};
        std::map<Vertex, VertexRank> vrankSparse;
        std::map<Vertex, Vertex> vparentSparse;
        boost::disjoint_sets<boost::associative_property_map<std::map<Vertex, VertexRank> >, boost::associative_property_map<std::map<Vertex, Vertex> > > 
          disjointSetsSparse_{boost::make_assoc_property_map(vrankSparse), boost::make_assoc_property_map(vparentSparse)};


        void clearDynamic();
        ompl::base::PathPtr getPathSparse(const Vertex &start, const Vertex &goal);
        ompl::base::Cost costHeuristicSparse(Vertex u, Vertex v) const;
        Graph graphSparse_;
        RoadmapNeighborsPtr nearestSparse_;
        std::vector<Configuration*> graphNeighborhood;
        std::vector<Configuration*> visibleNeighborhood;

        Vertex v_start_sparse;
        Vertex v_goal_sparse;

        bool isDenseFoundSolution_{false};

        PathVisibilityChecker* pathVisibilityChecker_{nullptr};

        // From SPARS
        /** \brief A counter for the number of consecutive failed iterations of the algorithm */
        unsigned int consecutiveFailures_{0u};
        
        /** \brief The stretch factor in terms of graph spanners for SPARS to check against */
        double stretchFactor_{3.};

        /** \brief Geometric Path variable used for smoothing out paths. */
        PathGeometric geomPath_;

        /** \brief A path simplifier used to simplify dense paths added to S */
        PathSimplifierPtr psimp_;
    };
  };
};


