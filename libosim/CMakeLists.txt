set(OSIM_INSTALL_OPENSIM_HEADERS OFF CACHE BOOL "enable/disable installing libosim.a with Simbody/OpenSim headers (used by OpenSim Creator)")

include(GNUInstallDirs)             # CMAKE_INSTALL_LIBDIR, _INCLUDEDIR, etc.
include(CMakePackageConfigHelpers)  # configure_package_config_file

find_package(LAPACK REQUIRED)
find_package(BLAS REQUIRED)
find_package(spdlog REQUIRED)

set(OSIM_OPENSIM_BASEDIR "${CMAKE_CURRENT_SOURCE_DIR}/opensim-core")
set(OSIM_SIMBODY_BASEDIR "${CMAKE_CURRENT_SOURCE_DIR}/simbody")

add_library(osim STATIC

    # Simbody sources
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Integrators/src/RungeKuttaMersonIntegrator.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Integrators/src/ExplicitEulerIntegrator.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Integrators/src/CPodes/SimTKcpodes.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Integrators/src/CPodes/nvector_SimTK.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Integrators/src/SemiExplicitEulerIntegrator.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Integrators/src/RungeKutta2Integrator.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Integrators/src/TimeStepper.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Integrators/src/VerletIntegrator.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Integrators/src/RungeKuttaFeldbergIntegrator.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Integrators/src/SemiExplicitEuler2Integrator.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Integrators/src/RungeKutta3Integrator.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Integrators/src/Integrator.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Integrators/src/AbstractIntegratorRep.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Integrators/src/CPodesIntegrator.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/src/Differentiator.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/src/MultibodyGraphMaker.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/src/About.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Geometry/src/ContactGeometry_SmoothHeightMap.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Geometry/src/ContactGeometry_Sphere.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Geometry/src/ContactGeometry.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Geometry/src/ContactGeometry_TriangleMesh.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Geometry/src/ContactGeometry_Ellipsoid.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Geometry/src/ContactGeometry_HalfSpace.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Geometry/src/CollisionDetectionAlgorithm.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Geometry/src/BicubicSurface.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Geometry/src/GCVSPLUtil.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Geometry/src/gcvspl.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Geometry/src/Geo_Box.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Geometry/src/Geo_Sphere.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Geometry/src/Geo_Triangle.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Geometry/src/ContactGeometry_Brick.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Geometry/src/Geo.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Geometry/src/OBBTree.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Geometry/src/Contact.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Geometry/src/ContactTracker.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Geometry/src/ContactGeometry_Cylinder.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Geometry/src/Geo_Point.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Geometry/src/ContactGeometry_Torus.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Geometry/src/Geodesic.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Geometry/src/OrientedBoundingBox.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/lbfgsb.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/c-cmaes/cmaes.c
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/CMAESOptimizer.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/LBFGSBOptimizer.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/InteriorPointOptimizer.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpAlgorithmRegOp.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpPDFullSpaceSolver.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpUtils.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpQualityFunctionMuOracle.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpLinearSolversRegOp.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpRestoIpoptNLP.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpRestoMinC_1Nrm.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpStdCInterface.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpCompoundVector.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpIteratesVector.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpExactHessianUpdater.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpMonotoneMuUpdate.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpGenTMatrix.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpGradientScaling.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpSymTMatrix.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpCompoundSymMatrix.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpRegOptions.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpSumMatrix.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpLoqoMuOracle.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpWarmStartIterateInitializer.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpDiagMatrix.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpSymScaledMatrix.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpDefaultIterateInitializer.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpFilterLSAcceptor.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpBlas.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpTNLPAdapter.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpObserver.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpLapackSolverInterface.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpPDPerturbationHandler.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpIpoptAlg.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpAlgBuilder.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpTSymLinearSolver.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpIpoptData.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpStdAugSystemSolver.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpTripletHelper.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpOptionsList.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpRestoIterationOutput.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpDenseGenMatrix.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpIpoptApplication.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpTripletToDenseConverter.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpIdentityMatrix.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpTaggedObject.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpOrigIpoptNLP.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpIpoptCalculatedQuantities.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpLeastSquareMults.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpRestoFilterConvCheck.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpDebug.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpSolveStatistics.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpLowRankAugSystemSolver.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpJournalist.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpSymMatrix.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpAdaptiveMuUpdate.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpRestoIterateInitializer.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpCompoundMatrix.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpOrigIterationOutput.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpFilter.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpProbingMuOracle.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpUserScaling.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpRestoRestoPhase.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpStdInterfaceTNLP.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpLimMemQuasiNewtonUpdater.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpDenseVector.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpAugRestoSystemSolver.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpInterfacesRegOp.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpMatrix.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpSumSymMatrix.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpMultiVectorMatrix.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpOptErrorConvCheck.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpBacktrackingLineSearch.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpZeroMatrix.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpLowRankUpdateSymMatrix.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpTimingStatistics.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpTripletToCSRConverter.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpNLPScaling.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpScaledMatrix.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpDenseSymMatrix.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpVector.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpExpansionMatrix.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpLapack.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/CFSQPOptimizer.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/OptimizerRep.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/lbfgs.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/Optimizer.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/LBFGSOptimizer.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/LinearAlgebra/src/LapackConvert.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/LinearAlgebra/src/Eigen.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/LinearAlgebra/src/FactorSVD.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/LinearAlgebra/src/Factor.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/LinearAlgebra/src/FactorQTZ.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKmath/LinearAlgebra/src/LapackInterface.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKcommon/src/Xml.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKcommon/src/tinyxml.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKcommon/src/NiceTypeName.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKcommon/src/PrivateInstantiations.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKcommon/src/About.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKcommon/src/Pathname.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKcommon/src/Timing.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKcommon/src/Parallel2DExecutor.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKcommon/src/tinyxmlparser.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKcommon/src/ParallelWorkQueue.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKcommon/src/ParallelExecutor.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKcommon/src/String.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKcommon/src/Plugin.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKcommon/SmallMatrix/src/SmallMatrix.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKcommon/Random/src/SFMT.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKcommon/Random/src/Random.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKcommon/Simulation/src/State.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKcommon/Simulation/src/EventReporter.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKcommon/Simulation/src/Study.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKcommon/Simulation/src/Measure.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKcommon/Simulation/src/Subsystem.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKcommon/Simulation/src/System.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKcommon/Simulation/src/EventHandler.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKcommon/Simulation/src/Event.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKcommon/Geometry/src/DecorativeGeometry.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKcommon/Geometry/src/PolygonalMesh.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKcommon/BigMatrix/src/ElementFilter.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKcommon/BigMatrix/src/MatrixCharacteristics.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKcommon/BigMatrix/src/MatrixHelper.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKcommon/Scalar/src/Scalar.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKcommon/Mechanics/src/Rotation.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKcommon/Mechanics/src/CoordinateAxis.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKcommon/Mechanics/src/Quaternion.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKcommon/Mechanics/src/UnitVec.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKcommon/Mechanics/src/MassProperties.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKcommon/Mechanics/src/Transform.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKcommon/Polynomial/src/PolynomialRootFinder.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKcommon/Polynomial/src/cpoly.cpp
    ${OSIM_SIMBODY_BASEDIR}/SimTKcommon/Polynomial/src/rpoly.cpp
    ${OSIM_SIMBODY_BASEDIR}/Simbody/src/GeneralForceSubsystem.cpp
    ${OSIM_SIMBODY_BASEDIR}/Simbody/src/RigidBodyNode.cpp
    ${OSIM_SIMBODY_BASEDIR}/Simbody/src/RigidBodyNode_LoneParticle.cpp
    ${OSIM_SIMBODY_BASEDIR}/Simbody/src/Constraint_Rod.cpp
    ${OSIM_SIMBODY_BASEDIR}/Simbody/src/CableSpring.cpp
    ${OSIM_SIMBODY_BASEDIR}/Simbody/src/Assembler.cpp
    ${OSIM_SIMBODY_BASEDIR}/Simbody/src/AssemblyCondition_OrientationSensors.cpp
    ${OSIM_SIMBODY_BASEDIR}/Simbody/src/Constraint_SphereOnPlaneContact.cpp
    ${OSIM_SIMBODY_BASEDIR}/Simbody/src/CablePath.cpp
    ${OSIM_SIMBODY_BASEDIR}/Simbody/src/SimbodyMatterSubtree.cpp
    ${OSIM_SIMBODY_BASEDIR}/Simbody/src/Constraint_PointOnPlaneContact.cpp
    ${OSIM_SIMBODY_BASEDIR}/Simbody/src/GeneralContactSubsystem.cpp
    ${OSIM_SIMBODY_BASEDIR}/Simbody/src/CableTrackerSubsystem.cpp
    ${OSIM_SIMBODY_BASEDIR}/Simbody/src/ContactTrackerSubsystem.cpp
    ${OSIM_SIMBODY_BASEDIR}/Simbody/src/Force.cpp
    ${OSIM_SIMBODY_BASEDIR}/Simbody/src/Motion.cpp
    ${OSIM_SIMBODY_BASEDIR}/Simbody/src/Force_LinearBushing.cpp
    ${OSIM_SIMBODY_BASEDIR}/Simbody/src/LocalEnergyMinimizer.cpp
    ${OSIM_SIMBODY_BASEDIR}/Simbody/src/SmoothSphereHalfSpaceForce.cpp
    ${OSIM_SIMBODY_BASEDIR}/Simbody/src/HuntCrossleyContact.cpp
    ${OSIM_SIMBODY_BASEDIR}/Simbody/src/PLUSImpulseSolver.cpp
    ${OSIM_SIMBODY_BASEDIR}/Simbody/src/Constraint_SphereOnSphereContact.cpp
    ${OSIM_SIMBODY_BASEDIR}/Simbody/src/PGSImpulseSolver.cpp
    ${OSIM_SIMBODY_BASEDIR}/Simbody/src/ObservedPointFitter.cpp
    ${OSIM_SIMBODY_BASEDIR}/Simbody/src/TextDataEventReporter.cpp
    ${OSIM_SIMBODY_BASEDIR}/Simbody/src/MobilizedBody.cpp
    ${OSIM_SIMBODY_BASEDIR}/Simbody/src/Constraint_LineOnLineContact.cpp
    ${OSIM_SIMBODY_BASEDIR}/Simbody/src/DecorationSubsystem.cpp
    ${OSIM_SIMBODY_BASEDIR}/Simbody/src/Force_Gravity.cpp
    ${OSIM_SIMBODY_BASEDIR}/Simbody/src/HuntCrossleyForce.cpp
    ${OSIM_SIMBODY_BASEDIR}/Simbody/src/About.cpp
    ${OSIM_SIMBODY_BASEDIR}/Simbody/src/RigidBodyNode_Weld.cpp
    ${OSIM_SIMBODY_BASEDIR}/Simbody/src/Constraint.cpp
    ${OSIM_SIMBODY_BASEDIR}/Simbody/src/Force_Instantiations.cpp
    ${OSIM_SIMBODY_BASEDIR}/Simbody/src/ExponentialSpringParameters.cpp
    ${OSIM_SIMBODY_BASEDIR}/Simbody/src/RigidBodyNodeSpec_Derived.cpp
    ${OSIM_SIMBODY_BASEDIR}/Simbody/src/MultibodySystem.cpp
    ${OSIM_SIMBODY_BASEDIR}/Simbody/src/ForceSubsystem.cpp
    ${OSIM_SIMBODY_BASEDIR}/Simbody/src/SimbodyMatterSubsystem.cpp
    ${OSIM_SIMBODY_BASEDIR}/Simbody/src/AssemblyCondition_Markers.cpp
    ${OSIM_SIMBODY_BASEDIR}/Simbody/src/ConditionalConstraint.cpp
    ${OSIM_SIMBODY_BASEDIR}/Simbody/src/SemiExplicitEulerTimeStepper.cpp
    ${OSIM_SIMBODY_BASEDIR}/Simbody/src/ElasticFoundationForce.cpp
    ${OSIM_SIMBODY_BASEDIR}/Simbody/src/ExponentialSpringForce.cpp
    ${OSIM_SIMBODY_BASEDIR}/Simbody/src/Body.cpp
    ${OSIM_SIMBODY_BASEDIR}/Simbody/src/OBSOLETE_LengthConstraints.cpp
    ${OSIM_SIMBODY_BASEDIR}/Simbody/src/SimbodyMatterSubsystemRep.cpp
    ${OSIM_SIMBODY_BASEDIR}/Simbody/src/ImpulseSolver.cpp
    ${OSIM_SIMBODY_BASEDIR}/Simbody/src/Force_Thermostat.cpp
    ${OSIM_SIMBODY_BASEDIR}/Simbody/src/Matter_Instantiations.cpp
    ${OSIM_SIMBODY_BASEDIR}/Simbody/src/RigidBodyNodeSpec.cpp
    ${OSIM_SIMBODY_BASEDIR}/Simbody/src/CompliantContactSubsystem.cpp
    ${OSIM_SIMBODY_BASEDIR}/Simbody/Visualizer/src/Visualizer.cpp
    ${OSIM_SIMBODY_BASEDIR}/Simbody/Visualizer/src/VisualizerGeometry.cpp
    ${OSIM_SIMBODY_BASEDIR}/Simbody/Visualizer/src/VisualizerProtocol.cpp
    ${OSIM_SIMBODY_BASEDIR}/Simbody/Visualizer/src/Visualizer_InputListener.cpp
    ${OSIM_SIMBODY_BASEDIR}/Simbody/Visualizer/src/Visualizer_Reporter.cpp

    # OpenSim sources
    ${OSIM_OPENSIM_BASEDIR}/Vendors/lepton/src/ParsedExpression.cpp
    ${OSIM_OPENSIM_BASEDIR}/Vendors/lepton/src/ExpressionProgram.cpp
    ${OSIM_OPENSIM_BASEDIR}/Vendors/lepton/src/Operation.cpp
    ${OSIM_OPENSIM_BASEDIR}/Vendors/lepton/src/Parser.cpp
    ${OSIM_OPENSIM_BASEDIR}/Vendors/lepton/src/ExpressionTreeNode.cpp
    ${OSIM_OPENSIM_BASEDIR}/Vendors/lepton/src/CompiledExpression.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Analyses/InverseDynamics.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Analyses/MuscleAnalysis.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Analyses/JointReaction.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Analyses/BodyKinematics.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Analyses/PointKinematics.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Analyses/ProbeReporter.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Analyses/About.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Analyses/StaticOptimizationTarget.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Analyses/OutputReporter.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Analyses/ForceReporter.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Analyses/StatesReporter.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Analyses/IMUDataReporter.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Analyses/InducedAccelerations.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Analyses/StaticOptimization.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Analyses/InducedAccelerationsSolver.cpp
    # ${OSIM_OPENSIM_BASEDIR}/OpenSim/Analyses/osimAnalysesDLL.cpp  # unused by OpenSim Creator
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Analyses/RegisterTypes_osimAnalyses.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Analyses/Actuation.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Analyses/Kinematics.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/ComponentOutput.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/OptimizationTarget.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/PropertyGroup.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/FileAdapter.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/ScaleSet.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/PiecewiseLinearFunction.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/XYFunctionInterface.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/MarkerData.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/ComponentList.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/PiecewiseConstantFunction.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/PropertyDblArray.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/Lmdif.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/Component.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/PropertyObj.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/DataAdapter.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/PropertyTransform.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/Units.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/IO.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/Assertion.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/Exception.cpp
    # ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/LoadOpenSimLibrary.cpp  # unused by OpenSim Creator
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/PropertyBool.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/ExpressionBasedFunction.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/ComponentSocket.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/SignalGenerator.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/osimCommonTemplates.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/ObjectGroup.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/PropertyDbl.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/XMLDocument.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/LatinHypercubeDesign.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/STOFileAdapter.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/VectorFunctionUncoupledNxN.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/VectorFunction.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/StateVector.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/Object.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/XsensDataReader.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/DebugUtilities.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/SmoothSegmentedFunctionFactory.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/PropertyStr.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/TableUtilities.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/MarkerFrame.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/SimmSpline.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/PropertyInt.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/LinearFunction.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/About.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/SimmIO.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/PropertyStrArray.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/PropertyIntArray.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/GCVSplineSet.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/APDMDataReader.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/PropertySet.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/SegmentedQuinticBezierToolkit.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/MultiplierFunction.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/Scale.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/PropertyTable.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/Property_Deprecated.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/IMUDataReader.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/AbstractDataTable.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/TRCFileAdapter.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/AbstractProperty.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/ComponentPath.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/Storage.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/PropertyBoolArray.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/RootSolver.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/FunctionSet.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/Signal.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/Property.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/GCVSpline.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/SmoothSegmentedFunction.cpp
    # ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/osimCommonDLL.cpp  # unused by OpenSim Creator
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/MultivariatePolynomialFunction.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/StepFunction.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/FunctionAdapter.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/Function.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/CSVFileAdapter.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/Constant.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/CommonUtilities.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/Logger.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/Reporter.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Common/RegisterTypes_osimCommon.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/SimulationUtilities.cpp
    # ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/osimSimulationDLL.cpp  # unused by OpenSim Creator
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/VisualizerUtilities.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Control/Control.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Control/TrackingController.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Control/Controller.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Control/ControlSet.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Control/SynergyController.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Control/PrescribedController.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Control/ControlSetController.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Control/ControlConstant.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Control/InputController.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Control/ControlLinear.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Control/ControlLinearNode.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/StatesTrajectory.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/ContactSphere.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/Analysis.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/AbstractPathPoint.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/Blankevoort1991Ligament.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/CMCActuatorSubsystem.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/JointInternalPowerProbe.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/Umberger2010MuscleMetabolicsProbe.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/Probe.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/ContactGeometry.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/GeometryPath.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/PathActuator.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/ExpressionBasedBushingForce.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/PhysicalFrame.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/ForceApplier.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/PrescribedForce.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/ExpressionBasedPointToPointForce.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/ActivationFiberLengthMuscle_Deprecated.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/ModelComponent.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/FunctionBasedPath.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/Ligament.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/Force.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/Marker.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/BodyScaleSet.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/ModelVisualizer.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/SmoothSphereHalfSpaceForce.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/Point.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/ExternalForce.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/MarkerSet.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/CoordinateSet.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/AbstractGeometryPath.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/PointToPointSpring.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/ForceProducer.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/Bhargava2004MuscleMetabolicsProbe.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/AnalysisSet.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/PathPoint.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/Actuator.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/HuntCrossleyForce.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/ControllerSet.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/ActivationFiberLengthMuscle.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/Station.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/BushingForce.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/ActuatorForceProbe.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/CoordinateLimitForce.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/Model.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/FunctionBasedBushingForce.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/Frame.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/StationDefinedFrame.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/ContactMesh.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/PhysicalOffsetFrame.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/SystemEnergyProbe.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/ForceAdapter.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/MuscleActiveFiberPowerProbe.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/Geometry.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/ExternalLoads.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/ElasticFoundationForce.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/ConditionalPathPoint.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/ActuatorPowerProbe.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/MovingPathPoint.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/ExpressionBasedCoordinateForce.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/ForceSet.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/BodyScale.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/Muscle.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/ContactHalfSpace.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/FunctionThresholdCondition.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/AbstractTool.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/PathSpring.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/Condition.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/Ground.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/Bhargava2004SmoothedMuscleMetabolics.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/MarkersReference.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/AssemblySolver.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Manager/Manager.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/About.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/RegisterTypes_osimSimulation.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Wrap/WrapEllipsoid.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Wrap/WrapObject.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Wrap/WrapDoubleCylinderObst.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Wrap/WrapResult.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Wrap/WrapSphereObst.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Wrap/PathWrap.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Wrap/PathWrapSet.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Wrap/PathWrapPoint.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Wrap/WrapMath.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Wrap/WrapCylinderObst.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Wrap/WrapTorus.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Wrap/WrapCylinder.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/Wrap/WrapSphere.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/InverseDynamicsSolver.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/MomentArmSolver.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/InverseKinematicsSolver.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/OpenSense/OpenSenseUtilities.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/OpenSense/IMUPlacer.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/BufferedOrientationsReference.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/SimbodyEngine/PlanarJoint.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/SimbodyEngine/PointOnLineConstraint.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/SimbodyEngine/SimbodyEngine.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/SimbodyEngine/PointConstraint.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/SimbodyEngine/ConstantCurvatureJoint.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/SimbodyEngine/WeldJoint.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/SimbodyEngine/UniversalJoint.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/SimbodyEngine/SpatialTransform.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/SimbodyEngine/UnilateralConstraint.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/SimbodyEngine/BallJoint.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/SimbodyEngine/PinJoint.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/SimbodyEngine/WeldConstraint.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/SimbodyEngine/CoordinateCouplerConstraint.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/SimbodyEngine/Joint.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/SimbodyEngine/Constraint.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/SimbodyEngine/RollingOnSurfaceConstraint.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/SimbodyEngine/ScapulothoracicJoint.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/SimbodyEngine/CustomJoint.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/SimbodyEngine/ConstantDistanceConstraint.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/SimbodyEngine/EllipsoidJoint.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/SimbodyEngine/Body.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/SimbodyEngine/Coordinate.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/SimbodyEngine/TransformAxis.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/SimbodyEngine/SliderJoint.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/SimbodyEngine/FreeJoint.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/SimbodyEngine/GimbalJoint.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/PositionMotion.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/StatesTrajectoryReporter.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/CoordinateReference.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Simulation/OrientationsReference.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Tools/MarkerPairSet.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Tools/CMC_Orientation.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Tools/GenericModelMaker.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Tools/CorrectionController.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Tools/InverseDynamicsTool.cpp
    # ${OSIM_OPENSIM_BASEDIR}/OpenSim/Tools/osimToolsDLL.cpp  # unused by OpenSim Creator
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Tools/Measurement.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Tools/ActuatorForceTargetFast.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Tools/ForwardTool.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Tools/DynamicsTool.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Tools/VectorFunctionForActuators.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Tools/ActuatorForceTarget.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Tools/CMC_Point.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Tools/RRATool.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Tools/About.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Tools/IKCoordinateTask.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Tools/MarkerPlacer.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Tools/AnalyzeTool.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Tools/CMCTool.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Tools/ModelScaler.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Tools/IMUInverseKinematicsTool.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Tools/MeasurementSet.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Tools/TrackingTask.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Tools/CMC.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Tools/ScaleTool.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Tools/CMC_Task.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Tools/SMC_Joint.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Tools/CMC_Joint.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Tools/RegisterTypes_osimTools.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Tools/InverseKinematicsTool.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Tools/CMC_TaskSet.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Tools/MuscleStateTrackingTask.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Tools/MarkerPair.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Actuators/ModelFactory.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Actuators/Thelen2003Muscle.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Actuators/MuscleSecondOrderActivationDynamicModel.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Actuators/RigidTendonMuscle.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Actuators/TendonForceLengthCurve.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Actuators/MuscleFirstOrderActivationDynamicModel.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Actuators/DeGrooteFregly2016Muscle.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Actuators/FiberCompressiveForceLengthCurve.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Actuators/ClutchedPathSpring.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Actuators/FirstOrderMuscleActivationDynamics.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Actuators/MuscleActivationDynamics.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Actuators/Thelen2003Muscle_Deprecated.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Actuators/Millard2012EquilibriumMuscle.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Actuators/MuscleFixedWidthPennationModel.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Actuators/PointActuator.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Actuators/ForceVelocityInverseCurve.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Actuators/PointToPointActuator.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Actuators/About.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Actuators/PolynomialPathFitter.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Actuators/ZerothOrderMuscleActivationDynamics.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Actuators/Delp1990Muscle_Deprecated.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Actuators/FiberForceLengthCurve.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Actuators/CoordinateActuator.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Actuators/ConstantMuscleActivation.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Actuators/Millard2012AccelerationMuscle.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Actuators/TorqueActuator.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Actuators/ActiveForceLengthCurve.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Actuators/BodyActuator.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Actuators/ForceVelocityCurve.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Actuators/McKibbenActuator.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Actuators/Schutte1993Muscle_Deprecated.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Actuators/SpringGeneralizedForce.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Actuators/FiberCompressiveForceCosPennationCurve.cpp
    # ${OSIM_OPENSIM_BASEDIR}/OpenSim/Actuators/osimActuatorsDLL.cpp  # unused by OpenSim Creator
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/Actuators/RegisterTypes_osimActuators.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/ExampleComponents/RegisterTypes_osimExampleComponents.cpp
    ${OSIM_OPENSIM_BASEDIR}/OpenSim/ExampleComponents/ToyReflexController.cpp

    # additional third-party plugins
    ThirdPartyPlugins/opensim-jam-org/jam-plugin/Smith2018ArticularContactForce.cpp
    ThirdPartyPlugins/opensim-jam-org/jam-plugin/Smith2018ArticularContactForce.h
    ThirdPartyPlugins/opensim-jam-org/jam-plugin/Smith2018ContactMesh.cpp
    ThirdPartyPlugins/opensim-jam-org/jam-plugin/Smith2018ContactMesh.h
    ThirdPartyPlugins/RegisterTypes_osimPlugin.cpp
    ThirdPartyPlugins/RegisterTypes_osimPlugin.h

    # additional code (in the `osim::` namespace)
    osim.cpp
    osim.h
)
target_include_directories(osim
PUBLIC
    # Simbody includes
    $<BUILD_INTERFACE:${OSIM_SIMBODY_BASEDIR}/SimTKmath/Integrators/src/CPodes/sundials/include>
    $<BUILD_INTERFACE:${OSIM_SIMBODY_BASEDIR}/SimTKmath/Integrators/include>
    $<BUILD_INTERFACE:${OSIM_SIMBODY_BASEDIR}/SimTKmath/include>
    $<BUILD_INTERFACE:${OSIM_SIMBODY_BASEDIR}/SimTKmath/Geometry/include>
    $<BUILD_INTERFACE:${OSIM_SIMBODY_BASEDIR}/SimTKcommon/SmallMatrix/include>
    $<BUILD_INTERFACE:${OSIM_SIMBODY_BASEDIR}/SimTKcommon/include>
    $<BUILD_INTERFACE:${OSIM_SIMBODY_BASEDIR}/SimTKcommon/Random/include>
    $<BUILD_INTERFACE:${OSIM_SIMBODY_BASEDIR}/SimTKcommon/Simulation/include>
    $<BUILD_INTERFACE:${OSIM_SIMBODY_BASEDIR}/SimTKcommon/Geometry/include>
    $<BUILD_INTERFACE:${OSIM_SIMBODY_BASEDIR}/SimTKcommon/BigMatrix/include>
    $<BUILD_INTERFACE:${OSIM_SIMBODY_BASEDIR}/SimTKcommon/Scalar/include>
    $<BUILD_INTERFACE:${OSIM_SIMBODY_BASEDIR}/SimTKcommon/Mechanics/include>
    $<BUILD_INTERFACE:${OSIM_SIMBODY_BASEDIR}/SimTKcommon/Polynomial/include>
    $<BUILD_INTERFACE:${OSIM_SIMBODY_BASEDIR}/Simbody/include>
    $<BUILD_INTERFACE:${OSIM_SIMBODY_BASEDIR}/Simbody/Visualizer/include>
    $<INSTALL_INTERFACE:include/simbody>

    # OpenSim includes
    $<BUILD_INTERFACE:${OSIM_OPENSIM_BASEDIR}>
    $<BUILD_INTERFACE:${OSIM_OPENSIM_BASEDIR}/Vendors/lepton/include>
    $<INSTALL_INTERFACE:include>

    # osim includes (uniformly uses `osim/DIR/HEADER` form)
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<INSTALL_INTERFACE:include>
)
target_compile_definitions(osim
PRIVATE
    # Simbody defines
    SimTK_SIMMATH_MAJOR_VERSION=${osc_VERSION_MAJOR}
    SimTK_SIMMATH_MINOR_VERSION=${osc_VERSION_MINOR}
    SimTK_SIMMATH_PATCH_VERSION=${osc_VERSION_PATCH}
    SimTK_SIMMATH_BUILD_VERSION=${osc_VERSION_PATCH}
    SimTK_SimTKCOMMON_MAJOR_VERSION=${osc_VERSION_MAJOR}
    SimTK_SimTKCOMMON_MINOR_VERSION=${osc_VERSION_MINOR}
    SimTK_SimTKCOMMON_PATCH_VERSION=${osc_VERSION_PATCH}
    SimTK_SIMBODY_MAJOR_VERSION=${osc_VERSION_MAJOR}
    SimTK_SIMBODY_MINOR_VERSION=${osc_VERSION_MINOR}
    SimTK_SIMBODY_PATCH_VERSION=${osc_VERSION_PATCH}
    SIMBODY_VISUALIZER_INSTALL_DIR="${CMAKE_INSTALL_FULL_BINDIR}"     # shuts the compiler up
    SIMBODY_VISUALIZER_REL_INSTALL_DIR="${CMAKE_INSTALL_BINDIR}"      # shuts the compiler up
    SIMBODY_PATH_FROM_LIBDIR_TO_VIZ_DIR="../${CMAKE_INSTALL_BINDIR}"  # shuts the compiler up

    # OpenSim defines
    OPENSIM_ACTUATORS_MAJOR_VERSION=${osc_VERSION_MAJOR}
    OPENSIM_ACTUATORS_MINOR_VERSION=${osc_VERSION_MINOR}
    OPENSIM_ACTUATORS_BUILD_VERSION=${osc_VERSION_PATCH}
    OPENSIM_ANALYSES_MAJOR_VERSION=${osc_VERSION_MAJOR}
    OPENSIM_ANALYSES_MINOR_VERSION=${osc_VERSION_MINOR}
    OPENSIM_ANALYSES_BUILD_VERSION=${osc_VERSION_PATCH}
    OPENSIM_COMMON_MAJOR_VERSION=${osc_VERSION_MAJOR}
    OPENSIM_COMMON_MINOR_VERSION=${osc_VERSION_MINOR}
    OPENSIM_COMMON_BUILD_VERSION=${osc_VERSION_PATCH}
    OPENSIM_SIMULATION_MAJOR_VERSION=${osc_VERSION_MAJOR}
    OPENSIM_SIMULATION_MINOR_VERSION=${osc_VERSION_MINOR}
    OPENSIM_SIMULATION_BUILD_VERSION=${osc_VERSION_PATCH}
    OPENSIM_TOOLS_MAJOR_VERSION=${osc_VERSION_MAJOR}
    OPENSIM_TOOLS_MINOR_VERSION=${osc_VERSION_MINOR}
    OPENSIM_TOOLS_BUILD_VERSION=${osc_VERSION_PATCH}
    OPENSIM_DISABLE_LOG_FILE=1  # disable unconditionally writing `opensim.log` in the process's working dir

PUBLIC
    # these ensure that `declspec(import)/declspec(export)` isn't used
    SimTK_USE_STATIC_LIBRARIES=1
    LEPTON_BUILDING_STATIC_LIBRARY=1
    OPENSIM_USE_STATIC_LIBRARIES=1

    # OpenSim's `spdlog` transitively uses a deprecated `stdext::checked_array_iterator`
    $<$<CXX_COMPILER_ID:MSVC>:_SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING>
)
target_compile_features(osim PUBLIC cxx_std_17)
set_target_properties(osim PROPERTIES

    # Treat it as a system library, which means that any downstream compiler warnings
    # won't apply to it.
    #
    # This is necessary because we're compiling OpenSim/Simbody headers, which might not
    # be compile-able with stricter compiler flags.
    SYSTEM TRUE
)

target_link_libraries(osim
PUBLIC
    ${CMAKE_DL_LIBS}            # used by simbody
    BLAS::BLAS                  # used by simbody
    LAPACK::LAPACK              # used by simbody
    spdlog::spdlog_header_only  # used by OpenSim
)

# make an installation directory that's mostly OpenSim-compatible (primarily, for OpenSim Creator)
if(${OSIM_INSTALL_OPENSIM_HEADERS})
    # install osim headers to `include/osim`
    install(FILES osim.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/osim/)

    # install `osimTargets.cmake` to `lib/cmake/osim`
    install(TARGETS osim EXPORT osimTargets)
    install(EXPORT osimTargets DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/osim)

    # install `osimConfig.cmake` to `lib/cmake/osim`
    configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/osimConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/osimConfig.cmake"
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/osim
    )
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/osimConfig.cmake" DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/osim")

    # install simbody headers to `include/simbody` (same as `simbody`)
    install(
    DIRECTORY
        ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Integrators/include/
        ${OSIM_SIMBODY_BASEDIR}/SimTKmath/include/
        ${OSIM_SIMBODY_BASEDIR}/SimTKmath/Geometry/include/
        ${OSIM_SIMBODY_BASEDIR}/SimTKcommon/SmallMatrix/include/
        ${OSIM_SIMBODY_BASEDIR}/SimTKcommon/include/
        ${OSIM_SIMBODY_BASEDIR}/SimTKcommon/Random/include/
        ${OSIM_SIMBODY_BASEDIR}/SimTKcommon/Simulation/include/
        ${OSIM_SIMBODY_BASEDIR}/SimTKcommon/Geometry/include/
        ${OSIM_SIMBODY_BASEDIR}/SimTKcommon/BigMatrix/include/
        ${OSIM_SIMBODY_BASEDIR}/SimTKcommon/Scalar/include/
        ${OSIM_SIMBODY_BASEDIR}/SimTKcommon/Mechanics/include/
        ${OSIM_SIMBODY_BASEDIR}/SimTKcommon/Polynomial/include/
        ${OSIM_SIMBODY_BASEDIR}/Simbody/include/
        ${OSIM_SIMBODY_BASEDIR}/Simbody/Visualizer/include/
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/simbody"
    )

    # install lepton headers to `include/lepton` (same as `opensim-core`)
    file(GLOB_RECURSE OSIM_GLOBBED_LEPTON_HEADERS "${OSIM_OPENSIM_BASEDIR}/Vendors/lepton/include/lepton/*.h")
    install(FILES ${OSIM_GLOBBED_LEPTON_HEADERS} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/lepton")

    # install OpenSim headers to `include/OpenSim` (same as `opensim-core`)
    set(OSIM_EXCLUDED_PATTERNS "/Test/" "/Tests/" "/Examples/" "/Moco/" "/Sandbox/" "C3DFileAdapter")
    file(GLOB_RECURSE OSIM_GLOBBED_HEADERS "${OSIM_OPENSIM_BASEDIR}/OpenSim/*.h")
    foreach(var ${OSIM_GLOBBED_HEADERS})
        set(OSIM_EXCLUDE FALSE)
        foreach(pattern ${OSIM_EXCLUDED_PATTERNS})
            if("${var}" MATCHES "${pattern}")
                set(OSIM_EXCLUDE TRUE)
            endif()
        endforeach()
        if(NOT ${OSIM_EXCLUDE})
            list(APPEND OSIM_HEADER_FILES "${var}")
        endif()
    endforeach()
    foreach(header ${OSIM_HEADER_FILES})
        file(RELATIVE_PATH header_relpath ${OSIM_OPENSIM_BASEDIR} ${header})
        get_filename_component(DIR ${header_relpath} DIRECTORY)
        install(FILES ${header} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${DIR})
    endforeach()
endif()

