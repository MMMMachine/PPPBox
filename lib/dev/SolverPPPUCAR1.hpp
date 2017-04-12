#pragma ident "$Id: SolverPPPUCAR1.hpp 2645 2011-06-08 03:23:24Z shjzhang $"

/**
 * @file SolverPPPUCAR1.hpp
 * Class to compute the PPP Solution with ionospheric estimation.
 */

#ifndef GPSTK_SOLVERPPPUCAR1_HPP
#define GPSTK_SOLVERPPPUCAR1_HPP

//============================================================================
//
//  This file is a part of ROCKET software
//
//  The ROCKET software is based on GPSTK, RTKLIB and some other open source
//  GPS data processing software. The aim of ROCKET software is to compute
//  real-time GNSS orbit, clock and positions using GNSS data. 
//
//  The ROCKET software is developed at School of Geodesy and Geomatics, 
//  Wuhan University. 
//
//  Shoujian Zhang - Wuhan University, 2012
//
//============================================================================
//
//  Revision
//
//  2014/02/24  
//  create to process mixed model for PPP
//
//  2014/04/07  
//  solve the ppp solution with extended model by fixing the zero-difference 
//  ambiguities.
//
//  2016/01/12  
//  finish this class, but to be tested!
//  
//	 2016.11.09
//  Several notes:
//  1. inverseChol() can not inverse an asymmetrical matrix, in that case, 
//     inverse() should be employed.
//  2. TypeIDSet can not keep the types you added in an order of your insertion.
//     If you want to sequence the types with your order, TypeIDSet is useless.  
//  
//  2016.11.09
//	 This version attempts to solve the equation system like this:
//  ===
//  P1 = ?dx +?dy +?dz + dtr' - dts_IF + Trop + r1*Ms*vtec + e 
//  P2 = ?dx +?dy +?dz + dtr' + DCBr - dts_IF + Trop + r2*Ms*vtec + e
//  L1 = ?dx +?dy +?dz + dtr' - dts_IF + Trop - r1*Ms*vtec + 
//			lambda1*N1 + ur,l1 - us,l1 - br,p1 + bs,IF + e
//  L2 = ?dx +?dy +?dz + dtr' - dts_IF + Trop - r2*Ms*vtec + 
//			lambda1*N2 + ur,l2 - us,l2 - br,p1 + bs,IF + e
//  Iono = Ms*vtec
//  dcb  = DCBr
//  ===
//  where: 
//	 dtr' = dtr + br,p1 
//	 dts_IF = dts + bs,IF 
//	 ri   = f1^2/fi^2
//  Iono is from IONEX file
//  dcb is a prior value with appropriate variance 
//  ===
//  Now I want to test:
//  1. no given prior trop  
//  2. combine upd at receiver end with clock errors 
//============================================================================


#include "CodeKalmanSolver.hpp"
#include "GNSSconstants.hpp"
#include "Variable.hpp"
#include "AmbiDatum.hpp"
#include "ARMLambda.hpp"
#include <vector>

namespace gpstk
{

      /** @addtogroup GPSsolutions */
      /// @ingroup math

      //@{

      /**This class computes the Precise Point Positioning (PPP) solution
       * using a Kalman solver that estimate the ionospheric delays with
       * P1/P2/L1/L2 combinations.
       *
       * This class may be used either in a Vector- and Matrix-oriented way,
       * or with GNSS data structure objects from "DataStructures" class (much
       * more simple to use it this way).
       *
       * A typical way to use this class with GNSS data structures follows:
       *
       * @code
       *
       * gnssRinex gRin;
       * SolverPPPUCAR1 pppSolver;
       * pppSolver.Process(gRin);
       *
       * @endcode
       *
       * \warning "SolverPPPUCAR1" is based on a Kalman filter, and Kalman filters
       * are objets that store their internal state, so you MUST NOT use the
       * SAME object to process DIFFERENT data streams.
       *
       * @sa SolverBase.hpp, SolverLMS.hpp and CodeKalmanSolver.hpp for
       * base classes.
       *
       */
   class SolverPPPUCAR1 : public CodeKalmanSolver
   {
   public:

         // AR method
      enum ARMethod
      {
         WLLC=1,
         WLL1,
         WLL2
      };


         /** Common constructor.
          *
          * @param useNEU   If true, will compute dLat, dLon, dH coordinates;
          *                 if false (the default), will compute dx, dy, dz.
          */
      SolverPPPUCAR1(bool useNEU = false, int polyOrder = 2);


         /** Returns a reference to a gnnsSatTypeValue object after
          *  solving the previously defined equation system.
          *
          * @param gData    Data object holding the data.
          */
      virtual gnssSatTypeValue& Process(gnssSatTypeValue& gData)
         throw(ProcessingException, SVNumException);


         /** Returns a reference to a gnnsRinex object after solving
          *  the previously defined equation system.
          *
          * @param gData    Data object holding the data.
          */
      virtual gnssRinex& Process(gnssRinex& gData)
         throw(ProcessingException, SVNumException);


         /** Code to be executed before 'Compute()' method.
          *
          * @param gData    Data object holding the data.
          */
      virtual gnssRinex& preCompute( gnssRinex& gData )
         throw(ProcessingException, SVNumException);


         /** Code to be executed after 'Compute()' method.
          *
          * @param gData    Data object holding the data.
          */
      virtual gnssRinex& postCompute( gnssRinex& gData )
         throw(ProcessingException);


         /** Returns a reference to a gnssRinex object after computing .
          *
          * @param gData    Data object holding the data.
          */
      virtual gnssRinex& Compute( gnssRinex& gData )
         throw(InvalidSolver);


         /** Time Update of the kalman filter.
          *
          * @param gData    Data object holding the data.
          */
      virtual int TimeUpdate( const Matrix<double>& phiMatrix,
                              const Matrix<double>& processNoiseCovariance  )
         throw(InvalidSolver);


         /** Returns a reference to a gnnsRinex object after applying
          * ambiguity constraint.
          *
          * @param gData    Data object holding the data.
          */
      virtual gnssRinex& AmbiguityConstr(gnssRinex& gData)
         throw(ProcessingException);



         /** Measurement Update of the kalman filter.
          *
          * @param gData    Data object holding the data.
          */
      virtual int MeasUpdate( const Vector<double>& prefitResiduals,
                              const Matrix<double>& designMatrix,
                              const Matrix<double>& weightMatrix  )
         throw(InvalidSolver);



         /** Returns a reference to a gnnsRinex object after fixing 
          *  all the potential ambiguities  
          *
          * @param gData    Data object holding the data.
          */
      virtual gnssRinex& AmbiguityFixing(gnssRinex& gData)
         throw(ProcessingException);


         /** Update the free unknonws' solution and covariance after
          *  fixing the ambiguities
          *
          * @param gData    Data object holding the data.
          */
      virtual void AmbiguityUpdate(Vector<double>& state,
                                  Matrix<double>& covariance,
                                  Vector<double>& fixedFlag,
                                  int& index,
                                  double& fixedValue)
         throw(ProcessingException);


         /** Sets if a NEU system will be used.
          *
          * @param useNEU  Boolean value indicating if a NEU system will
          *                be used
          *
          */
      virtual SolverPPPUCAR1& setNEU( bool useNEU, int polyOrder );


         /** Set ambiguity resolution method
          *
          * @param method  enum value
          *
          */
      virtual SolverPPPUCAR1& setARMethod( string method)
      { ARMethod=method ;return (*this);} ;

         /** Set ambiguity resolution method
          *
          * @param method  enum value
          *
          */
      virtual SolverPPPUCAR1& setUsingC1( bool useC1)
      { usingC1=useC1; return (*this);} ;


         /** Set a single coordinates stochastic model to ALL coordinates.
          *
          * @param pModel      Pointer to StochasticModel associated with
          *                    coordinates.
          *
          * @warning Do NOT use this method to set the SAME state-aware
          * stochastic model (like RandomWalkModel, for instance) to ALL
          * coordinates, because the results will certainly be erroneous. Use
          * this method only with non-state-aware stochastic models like
          * 'StochasticModel' (constant coordinates) or 'WhiteNoiseModel'.
          */
      virtual SolverPPPUCAR1& setCoordinatesModel(StochasticModel* pModel);


         /** Set a random walk stochastic model to slant ionospheric delays.
          *
          * @param pModel      Pointer to StochasticModel associated with
          *                    ionospheric delays.
          *
          */
      virtual SolverPPPUCAR1& setIonosphericModel(StochasticModel* pModel);


         /** Set value of initial variance for apriori tropospheric values.
          *
          * @param variance      Initial variance assigned to this variable.
          */
      SolverPPPUCAR1& setInitialTropoVar(double variance)
      { aprioriTropoVar = variance; return (*this); };


         /** Set value of initial variance for apriori ionospheric values.
          *
          * @param variance      Initial variance assigned to this variable.
          */
      SolverPPPUCAR1& setInitialIonoVar(double variance)
      { aprioriIonoVar = variance; return (*this); };


         /** Set value of initial variance for apriori ionospheric values.
          *
          * @param variance      Initial variance assigned to this variable.
          */
      SolverPPPUCAR1& setInitialSpatialVar(double variance)
      { aprioriSpatialVar = variance; return (*this); };


         /** Set reinitialize interval.
          *
          * @param interval    interval that reinitialize the filter.
          */
      virtual SolverPPPUCAR1& setReInitInterv(double interval)
      { reInitialInterval = interval; return (*this); };


         /** Set reinitialize boolean .
          *
          * @param reiniitlal   boolean indicating wether re-iniitialze the filter.
          */
      virtual SolverPPPUCAR1& setReInitialize(double reini)
      { reInitialize = reini; return (*this); };


         /** Returns the solution associated to a given TypeID.
          *
          * @param type    TypeID of the solution we are looking for.
          */
      virtual double getSolution(const TypeID& type) const
         throw(InvalidRequest);


         /** Returns the solution associated to a given TypeID.
          *
          * @param type    TypeID of the solution we are looking for.
          */
      virtual double getFixedSolution(const TypeID& type) const
         throw(InvalidRequest);


         /** Returns the variance associated to a given TypeID.
          *
          * @param type    TypeID of the variance we are looking for.
          */
      virtual double getVariance(const TypeID& type) const
         throw(InvalidRequest);


         /** Returns the variance associated to a given TypeID.
          *
          * @param type    TypeID of the variance we are looking for.
          */
      virtual double getFixedVariance(const TypeID& type) const
         throw(InvalidRequest);


         /** Return the CURRENT number of satellite.
          */
      virtual int getCurrentSatNumber() const
         throw(InvalidRequest);


         /** Return the CURRENT number of satellite.
          */
      virtual int getFixedAmbNumL1() const
         throw(InvalidRequest);


         /** Return the CURRENT number of satellite.
          */
      virtual int getFixedAmbNumWL() const
         throw(InvalidRequest);

      
      virtual std::vector<double> getTTFFWL() const
      {
          return ttffWL;
      }

      virtual std::vector<double> getTTFFL1() const
      {
          return ttffL1;
      }


      virtual std::vector<double> getSTime() const
      { return startTimeVec; }


         /// Returns an index identifying this object.
      virtual int getIndex(void) const;


         /// Returns a string identifying this object.
      virtual std::string getClassName(void) const;

         /// Debug level
      static int debugLevel;

         /// Destructor.
      virtual ~SolverPPPUCAR1() {};


   private:

         /// Constraint equation system
      AmbiDatum datumL1;
      AmbiDatum datumL2;

         /// ploynominal order
      int polyOrder;

         /// Whether turn on the 'reInitialize' or not
      bool reInitialize;

         /// Using C1 or P1 ?
      bool usingC1;

         /// Interval to reinitialize the filter
      double reInitialInterval;

         /// Ambiguity resolution method
      string ARMethod;

         /// Boolean to indicate whether the solution is converged
      bool converged;

         /// Size for convergBuffer
      int bufferSize;

         /// Buffer to store the solution drou 
      std::deque<bool> convergBuffer;

         /// Variable to indicate the first epoch
      CommonTime firstEpoch;

         /// Number of variables
      int numVar;

         /// Number of unknowns
      int numUnknowns;

         /// Number of measurements
      int numMeas;

         /// Number of measurements
      int numCurrentSV;


         /// State Transition Matrix (PhiMatrix)
      Matrix<double> phiMatrix;

         /// Noise covariance matrix (QMatrix)
      Matrix<double> qMatrix;

         /// Geometry matrix
      Matrix<double> hMatrix;

         /// Weights matrix
      Matrix<double> rMatrix;

         /// Measurements vector (Prefit-residuals)
      Vector<double> measVector;

         /// Source-indexed TypeID set
//      TypeIDSet srcIndexedTypes;
		TypeIDList srcIndexedTypes;

         /// Observable-independent TypeID set
      TypeIDSet coreTypes;

         /// Satellite-indexed TypeID set
//      TypeIDSet satIndexedTypes;
		TypeIDList satIndexedTypes; 

         /// Global set of unknowns
      VariableSet varUnknowns;

         /// Ionospheric delay variables
      VariableSet ionoUnks;

         /// Map holding state information for satellite related variables
      VariableDataMap stateMap;

         /// A structure used to store Kalman filter data.
      struct filterData
      {
            // Default constructor 
         filterData() {};

         std::map<TypeID, double> srcIndexedVarCov;  ///< source indexed variables' covariance values.
         std::map<Variable, double> satIndexedVarCov;  ///< satellite indexed variables' covariance values.

      };

         /// Map holding covariance information
      std::map<Variable, filterData> covarianceMap;

         /// Boolean indicating if this filter was run at least once
      bool firstTime;

         /// The variance of the apriori ionospheric delays and wet tropo 
      double aprioriTropoVar;
      double aprioriIonoVar;
      double aprioriSpatialVar;
      double aprioriDCBVar;
      double aprioriUPDVar;

      bool resetWL;
      bool resetL1;
      bool resetSol;

      double startTime;

         /// Vectors to store the convegence statistics information
      std::vector< double > startTimeVec;
      std::vector< double > ttffWL;
      std::vector< double > ttffL1;
      std::vector< double > ttffL2;
      std::vector< double > ttsc;


         /// Number of fixed widelane ambiguities
      int numFixedBWL;
      int numFixedBL1;
      int numFixedBL2;


         // Predicted state
      Vector<double> xhatminus;


         // Predicted covariance.
      Matrix<double> Pminus;


         // A posteriori state
      Vector<double> xhat;


         // A posteriori covariance.
      Matrix<double> P;

         // The updated state 
      Vector<double> newState ;
     
         // The updated covariance
      Matrix<double> newCov;


         /// Set with all satellites being processed this epoch
      SatIDSet currSatSet;


         /// Set with all satellites being processed this epoch
      SatIDSet satSet;


         /// General Kalman filter object
      SimpleKalmanFilter kFilter;


         /// Initializing method.
      void Init(void);


         /// White noise stochastic model for position
      WhiteNoiseModel whitenoiseModelX;
      WhiteNoiseModel whitenoiseModelY;
      WhiteNoiseModel whitenoiseModelZ;


         /// Random Walk stochastic model
      TropoRandomWalkModel tropoModel;

         /// Constant stochastic model
      StochasticModel constantModel;

         /// White noise stochastic model
      WhiteNoiseModel whitenoiseModel;

         /// Random walk model for dcb
      RandomWalkModel dcbModel;

         /// Random walk model for upd on L1/L2
      RandomWalkModel updModelL1;
      RandomWalkModel updModelL2;

         /// Random walk model for upd on L1/L2
      RandomWalkModel A0Model;
      RandomWalkModel A1Model;
      RandomWalkModel A2Model;
      RandomWalkModel A3Model;
      RandomWalkModel A4Model;
      RandomWalkModel A5Model;

         /// Phase Ambiguity models
      PhaseAmbiguityModel ambiModelL2;
      PhaseAmbiguityModel ambiModelL1;

         /// Random walk model for ionospheric delays
      IonoRandomWalkModel ionoModel;

         /// Pointer to stochastic model for troposphere
      StochasticModel* pTropoStoModel;

         /// Pointer to stochastic model for dx (or dLat) coordinate
      StochasticModel* pCoordXStoModel;
      StochasticModel* pCoordYStoModel;
      StochasticModel* pCoordZStoModel;

         /// Pointer to stochastic model for receiver clock
      StochasticModel* pClockStoModel;
      StochasticModel* pDCBStoModel;
//      StochasticModel* pUPDStoModelL1;
//      StochasticModel* pUPDStoModelL2;
      StochasticModel* pClockStoModelL1;
      StochasticModel* pClockStoModelL2;

      StochasticModel* pA0StoModel;
      StochasticModel* pA1StoModel;
      StochasticModel* pA2StoModel;
      StochasticModel* pA3StoModel;
      StochasticModel* pA4StoModel;
      StochasticModel* pA5StoModel;

         /// Pointer to stochastic model for ambiguity on L1
      PhaseAmbiguityModel* pAmbiModelL1; // for L1 ambiguity
      PhaseAmbiguityModel* pAmbiModelL2;

         /// Pointer to stochastic model for ionospheric delays
      StochasticModel* pIonoModel; 


         // First, let's define a set with the typical code-based unknowns
      TypeIDSet defaultTypeSet;

         // Then, define a set with the typical for satellite-related unknowns
      TypeIDSet varTypeSet;


         /// Initial index assigned to this class.
      static int classIndex;


         /// Index belonging to this object.
      int index;


         /// Sets the index and increment classIndex.
      void setIndex(void)
      { index = classIndex++; };


         // Some methods that we want to hide
      virtual int Compute( const Vector<double>& prefitResiduals,
                           const Matrix<double>& designMatrix )
         throw(InvalidSolver)
      { return 0; };


      virtual SolverPPPUCAR1& setDefaultEqDefinition(
                             const gnssEquationDefinition& eqDef )
      { return (*this); };



   }; // End of class 'SolverPPPUCAR1'

      //@}

}  // End of namespace gpstk
#endif   // GPSTK_SOLVERPPPUCAR_HPP
