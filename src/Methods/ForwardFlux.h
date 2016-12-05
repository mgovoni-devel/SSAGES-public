/**
 * This file is part of
 * SSAGES - Suite for Advanced Generalized Ensemble Simulations
 *
 * Copyright 2016 Ben Sikora <bsikora906@gmail.com>
 *                Joshua Lequieu <lequieu@uchicago.edu>
 *                Hadi Ramezani-Dakhel <ramezani@uchicago.edu>
 *
 * SSAGES is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * SSAGES is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with SSAGES.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once 

#include "Method.h"
#include "../CVs/CollectiveVariable.h"
#include <fstream>
#include <random>
#include <queue>
#include "../FileContents.h"
#include "../Drivers/DriverException.h"

namespace mpi = boost::mpi;
namespace SSAGES
{
	//! ForwardFlux sampling method
	/*!
	 * \ingroup Methods
     * The notation used here is drawn largely from Allen, Valeriani and Rein ten Wolde. J. Phys.: Condens. Matter (2009) 21:463102. 
     * We recommend referring to this review if the reader is unfamiliar with the method, or our variable naming conventions.
	 */
	class ForwardFlux : public Method
	{
	protected:

        //-----------------------------------------------------------------
        // Private Variables
        //-----------------------------------------------------------------
        //! Internal class to store different FFS Config IDs
        class FFSConfigID
        {
           public:
            unsigned int l; //!< Interface number
            unsigned int n;      //!< Configuration Number
            unsigned int a;      //!< Attempt number
            bool success;        //!< Whether it suceeded or failed
            //FFSConfigID* previous; //!< ID of FFSConfiguration that I came from
            unsigned int l_prev;      //!< Previous Interface number (i.e. traj I came from)
            unsigned int n_prev;      //!< Previous Configuration Number
            unsigned int a_prev;      //!< Previous Attempt number
        };



		//! Number of FFS interfaces
		double _ninterfaces;

		//! FFS Interfaces
		std::vector<double> _interfaces;

        //! Current Interface
        unsigned int _currentinterface;

		//! Previous cv position, used to determine if you've crossed an interface since last time
        double _cvvalue_previous;

		//!  current cv position
        double _cvvalue;

		//! Total number of configurations to collect at lambda0 (first interface) 
		unsigned int _N0 ;

        //! Data structure that holds a Library N0 configurations at lambda0
        std::vector<FFSConfigID> Lambda0ConfigLibrary;

        //! Total Simulation Time spent in accumulating \ _N0
        double _N0TotalSimTime;

        //! Flux of trajectories out of state A. Denoted PhiA0 over h_A in Allen2009.
        double _fluxA0;

        //! Number of trials to attemt from each interface
        //! Note _M[0] sets the number of 'branches' for RBFFS and BGFFS?
        std::vector<unsigned int> _M;

        //! Number of attempts from interface i
        std::vector<unsigned int> _attempts;

        //! Flag to determine wheter fluxA0 should be calculated
        bool _computefluxA0;

        //! Probability of going from lambda_{i} to lambda_{i+1}
        std::vector<double> _P;

        //! Number of successes from lambda_{i} to lambda_{i+1}
        //!  (might need to be 2d vector if multiple branches are used (with RBFFS)
        std::vector<double> _S;

        //! Stores what 'mode' of FFS we're in. 
        /*!
         *  Options:
         *   - computefluxA0
         *   - WHAT OTHERS?
         */
        bool _initialFluxFlag;

        //! The current FFSConfigID of this MPI process
        FFSConfigID myFFSConfigID;


        /*!
         *  When a given processor reaches an interface, it pulls a config from this Queue to figure out what it should do next
         *  This object should be syncronized between all FFS walkers (is walker the correct terminology here?)
         */
        std::queue<FFSConfigID*> FFSConfigIDQueue; 


        //-----------------------------------------------------------------
        // Private Functions
        //-----------------------------------------------------------------
        
        //! Function that checks if interfaces have been crossed (different for each FFS flavor)
        void CheckForInterfaceCrossigns(Snapshot* snapshot, CVList& cvs)

        //! Function that adds new FFS configurations to the Queue
        //! Different FFS flavors can have differences in this method
        void AddNewIDsToQueue();

        //! Function checks if configuration has returned to A
        bool HasReturnedToA(Snapshot* snapshot);

        //! Function checks if configuration has crossed interface specified since the last check
        /*! Simple function, given current and previous cv position, checks if interface i has been crossed. If crossed in positive direction, return +1, if crossed in negative direction return -1, if nothing crossed return 0
         */
        int HasCrossedInterface(unsigned int interface);

        //! Function checks if FFS is Finished, returns bool with result
        //! See if interface is the last one, and the queue is empty, etc
        bool CheckIfFinishedMethod();

        //! Write a file corresponding to FFSConfigID from current snapshot
        void WriteFFSConfiguration(Snapshot *snapshot,FFSConfigID ffsconfig);

        //! Read a file corresponding to a FFSConfigID into current snapshot
        void ReadFFSConfiguration(Snapshot *,FFSConfigID);
       
        //! Compute Initial Flux
        void ComputeInitialFlux();

        //! Compute the probability of going from each lambda_i to lambda_{i+1} 
        /*!  
         *  Using number of successes and number of trials
         *  This will need to be different for each FFS flavor 
         */
        void ComputeTransitionProbabilities();

	public:
		//! Constructor
		/*!
		 * \param world MPI global communicator.
		 * \param comm MPI local communicator.
			 * \param frequency Frequency with which this method is invoked.
		 *
		 * Create instance of Forward Flux
		 */
		ForwardFlux(boost::mpi::communicator& world,
				 boost::mpi::communicator& comm,
				 unsigned int frequency) : 
		Method(frequency, world, comm)
        {
            //set variables here			
		}

		//! Pre-simulation hook.
		/*!
		 * \param snapshot Current simulation snapshot.
		 * \param cvs List of CVs.
		 */
		void PreSimulation(Snapshot* snapshot, const CVList& cvs) override;

		//! Post-integration hook.
		/*!
		 * \param snapshot Current simulation snapshot.
		 * \param cvs List of CVs.
		 */
		void PostIntegration(Snapshot* snapshot, const CVList& cvs) override;

		//! Post-simulation hook.
		/*!
		 * \param snapshot Current simulation snapshot.
		 * \param cvs List of CVs.
		 */
		void PostSimulation(Snapshot* snapshot, const CVList& cvs) override;

		//! \copydoc Serializable::Serialize()
		void Serialize(Json::Value& json) const override
		{
			//Needed to run
			json["type"] = "ForwardFlux";

        }

	};
}


/*
File Formats:
_indexfile
interface(some integer) dump_file_name(a string that contains interface and trial number)
example: 1 dump_1_10.xyz

dumpfile
atomid posx posy posz vx vy vz


*/
