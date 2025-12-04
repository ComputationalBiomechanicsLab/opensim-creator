include(GNUInstallDirs)             # CMAKE_INSTALL_LIBDIR, _INCLUDEDIR, etc.
include(CMakePackageConfigHelpers)  # configure_package_config_file

# BLAS/LAPACK: prefer using OpenBLAS if it has been installed
# on the `CMAKE_PREFIX_PATH`, because the caller _probably_
# wants the build to use that
foreach(_prefix IN LISTS CMAKE_PREFIX_PATH)
    if(EXISTS "${_prefix}/${CMAKE_INSTALL_LIBDIR}/cmake/OpenBLAS/OpenBLASConfig.cmake")
        find_package(OpenBLAS REQUIRED CONFIG)

        # Alias FindBLAS/FindLAPACK targets
        add_library(BLAS::BLAS INTERFACE IMPORTED)
        add_library(LAPACK::LAPACK INTERFACE IMPORTED)
        set_target_properties(BLAS::BLAS PROPERTIES INTERFACE_LINK_LIBRARIES OpenBLAS::OpenBLAS)
        set_target_properties(LAPACK::LAPACK PROPERTIES INTERFACE_LINK_LIBRARIES OpenBLAS::OpenBLAS)

        break()
    endif()
endforeach()
if(NOT OpenBLAS_FOUND)
    # Fall back to system-provided BLAS/LAPACK
    find_package(BLAS    REQUIRED)
    find_package(LAPACK  REQUIRED)
endif()

find_package(spdlog  REQUIRED)  # used by OpenSim

set(OPYN_OPENSIM_BASEDIR "${CMAKE_CURRENT_SOURCE_DIR}/opensim-core")
set(OPYN_SIMBODY_BASEDIR "${CMAKE_CURRENT_SOURCE_DIR}/simbody")

add_library(osim STATIC

    # Simbody sources
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Integrators/src/CPodes/sundials/src/sundials/sundials_band.c
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Integrators/src/CPodes/sundials/src/sundials/sundials_dense.c
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Integrators/src/CPodes/sundials/src/sundials/sundials_direct.c
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Integrators/src/CPodes/sundials/src/sundials/sundials_iterative.c
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Integrators/src/CPodes/sundials/src/sundials/sundials_lapack.c
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Integrators/src/CPodes/sundials/src/sundials/sundials_math.c
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Integrators/src/CPodes/sundials/src/sundials/sundials_nvector.c
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Integrators/src/CPodes/sundials/src/sundials/sundials_spbcgs.c
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Integrators/src/CPodes/sundials/src/sundials/sundials_spgmr.c
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Integrators/src/CPodes/sundials/src/sundials/sundials_sptfqmr.c
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Integrators/src/CPodes/sundials/src/nvec_ser/fnvector_serial.c
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Integrators/src/CPodes/sundials/src/nvec_ser/nvector_serial.c
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Integrators/src/CPodes/sundials/src/cpodes/cpodes_band.c
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Integrators/src/CPodes/sundials/src/cpodes/cpodes_bandpre.c
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Integrators/src/CPodes/sundials/src/cpodes/cpodes_bbdpre.c
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Integrators/src/CPodes/sundials/src/cpodes/cpodes.c
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Integrators/src/CPodes/sundials/src/cpodes/cpodes_dense.c
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Integrators/src/CPodes/sundials/src/cpodes/cpodes_direct.c
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Integrators/src/CPodes/sundials/src/cpodes/cpodes_ic.c
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Integrators/src/CPodes/sundials/src/cpodes/cpodes_io.c
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Integrators/src/CPodes/sundials/src/cpodes/cpodes_lapack.c
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Integrators/src/CPodes/sundials/src/cpodes/cpodes_nls.c
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Integrators/src/CPodes/sundials/src/cpodes/cpodes_proj.c
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Integrators/src/CPodes/sundials/src/cpodes/cpodes_root.c
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Integrators/src/CPodes/sundials/src/cpodes/cpodes_spbcgs.c
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Integrators/src/CPodes/sundials/src/cpodes/cpodes_spgmr.c
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Integrators/src/CPodes/sundials/src/cpodes/cpodes_spils.c
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Integrators/src/CPodes/sundials/src/cpodes/cpodes_sptfqmr.c
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Integrators/src/RungeKuttaMersonIntegrator.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Integrators/src/ExplicitEulerIntegrator.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Integrators/src/CPodes/SimTKcpodes.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Integrators/src/CPodes/nvector_SimTK.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Integrators/src/SemiExplicitEulerIntegrator.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Integrators/src/RungeKutta2Integrator.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Integrators/src/TimeStepper.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Integrators/src/VerletIntegrator.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Integrators/src/RungeKuttaFeldbergIntegrator.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Integrators/src/SemiExplicitEuler2Integrator.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Integrators/src/RungeKutta3Integrator.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Integrators/src/Integrator.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Integrators/src/AbstractIntegratorRep.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Integrators/src/CPodesIntegrator.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/src/Differentiator.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/src/MultibodyGraphMaker.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/src/About.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Geometry/src/ContactGeometry_SmoothHeightMap.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Geometry/src/ContactGeometry_Sphere.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Geometry/src/ContactGeometry.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Geometry/src/ContactGeometry_TriangleMesh.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Geometry/src/ContactGeometry_Ellipsoid.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Geometry/src/ContactGeometry_HalfSpace.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Geometry/src/CollisionDetectionAlgorithm.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Geometry/src/BicubicSurface.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Geometry/src/GCVSPLUtil.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Geometry/src/gcvspl.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Geometry/src/Geo_Box.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Geometry/src/Geo_Sphere.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Geometry/src/Geo_Triangle.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Geometry/src/ContactGeometry_Brick.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Geometry/src/Geo.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Geometry/src/OBBTree.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Geometry/src/Contact.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Geometry/src/ContactTracker.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Geometry/src/ContactGeometry_Cylinder.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Geometry/src/Geo_Point.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Geometry/src/ContactGeometry_Torus.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Geometry/src/Geodesic.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Geometry/src/OrientedBoundingBox.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/lbfgsb.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/c-cmaes/cmaes.c
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/CMAESOptimizer.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/LBFGSBOptimizer.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/InteriorPointOptimizer.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpAlgorithmRegOp.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpPDFullSpaceSolver.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpUtils.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpQualityFunctionMuOracle.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpLinearSolversRegOp.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpRestoIpoptNLP.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpRestoMinC_1Nrm.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpStdCInterface.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpCompoundVector.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpIteratesVector.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpExactHessianUpdater.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpMonotoneMuUpdate.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpGenTMatrix.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpGradientScaling.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpSymTMatrix.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpCompoundSymMatrix.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpRegOptions.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpSumMatrix.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpLoqoMuOracle.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpWarmStartIterateInitializer.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpDiagMatrix.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpSymScaledMatrix.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpDefaultIterateInitializer.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpFilterLSAcceptor.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpBlas.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpTNLPAdapter.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpObserver.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpLapackSolverInterface.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpPDPerturbationHandler.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpIpoptAlg.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpAlgBuilder.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpTSymLinearSolver.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpIpoptData.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpStdAugSystemSolver.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpTripletHelper.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpOptionsList.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpRestoIterationOutput.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpDenseGenMatrix.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpIpoptApplication.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpTripletToDenseConverter.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpIdentityMatrix.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpTaggedObject.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpOrigIpoptNLP.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpIpoptCalculatedQuantities.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpLeastSquareMults.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpRestoFilterConvCheck.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpDebug.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpSolveStatistics.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpLowRankAugSystemSolver.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpJournalist.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpSymMatrix.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpAdaptiveMuUpdate.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpRestoIterateInitializer.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpCompoundMatrix.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpOrigIterationOutput.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpFilter.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpProbingMuOracle.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpUserScaling.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpRestoRestoPhase.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpStdInterfaceTNLP.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpLimMemQuasiNewtonUpdater.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpDenseVector.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpAugRestoSystemSolver.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpInterfacesRegOp.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpMatrix.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpSumSymMatrix.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpMultiVectorMatrix.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpOptErrorConvCheck.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpBacktrackingLineSearch.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpZeroMatrix.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpLowRankUpdateSymMatrix.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpTimingStatistics.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpTripletToCSRConverter.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpNLPScaling.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpScaledMatrix.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpDenseSymMatrix.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpVector.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpExpansionMatrix.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/IpOpt/IpLapack.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/CFSQPOptimizer.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/OptimizerRep.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/lbfgs.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/Optimizer.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Optimizers/src/LBFGSOptimizer.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/LinearAlgebra/src/LapackConvert.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/LinearAlgebra/src/Eigen.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/LinearAlgebra/src/FactorSVD.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/LinearAlgebra/src/Factor.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/LinearAlgebra/src/FactorQTZ.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKmath/LinearAlgebra/src/LapackInterface.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKcommon/src/Xml.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKcommon/src/tinyxml.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKcommon/src/NiceTypeName.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKcommon/src/PrivateInstantiations.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKcommon/src/About.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKcommon/src/Pathname.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKcommon/src/Timing.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKcommon/src/Parallel2DExecutor.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKcommon/src/tinyxmlparser.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKcommon/src/ParallelWorkQueue.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKcommon/src/ParallelExecutor.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKcommon/src/String.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKcommon/src/Plugin.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKcommon/SmallMatrix/src/SmallMatrix.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKcommon/Random/src/SFMT.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKcommon/Random/src/Random.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKcommon/Simulation/src/State.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKcommon/Simulation/src/EventReporter.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKcommon/Simulation/src/Study.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKcommon/Simulation/src/Measure.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKcommon/Simulation/src/Subsystem.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKcommon/Simulation/src/System.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKcommon/Simulation/src/EventHandler.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKcommon/Simulation/src/Event.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKcommon/Geometry/src/DecorativeGeometry.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKcommon/Geometry/src/PolygonalMesh.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKcommon/BigMatrix/src/ElementFilter.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKcommon/BigMatrix/src/MatrixCharacteristics.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKcommon/BigMatrix/src/MatrixHelper.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKcommon/Scalar/src/Scalar.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKcommon/Mechanics/src/Rotation.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKcommon/Mechanics/src/CoordinateAxis.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKcommon/Mechanics/src/Quaternion.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKcommon/Mechanics/src/UnitVec.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKcommon/Mechanics/src/MassProperties.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKcommon/Mechanics/src/Transform.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKcommon/Polynomial/src/PolynomialRootFinder.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKcommon/Polynomial/src/cpoly.cpp
    ${OPYN_SIMBODY_BASEDIR}/SimTKcommon/Polynomial/src/rpoly.cpp
    ${OPYN_SIMBODY_BASEDIR}/Simbody/src/CableSpan.cpp
    ${OPYN_SIMBODY_BASEDIR}/Simbody/src/CableSpan_SubsystemTestHelper_Impl.cpp
    ${OPYN_SIMBODY_BASEDIR}/Simbody/src/GeneralForceSubsystem.cpp
    ${OPYN_SIMBODY_BASEDIR}/Simbody/src/RigidBodyNode.cpp
    ${OPYN_SIMBODY_BASEDIR}/Simbody/src/RigidBodyNode_LoneParticle.cpp
    ${OPYN_SIMBODY_BASEDIR}/Simbody/src/Constraint_Rod.cpp
    ${OPYN_SIMBODY_BASEDIR}/Simbody/src/CableSpring.cpp
    ${OPYN_SIMBODY_BASEDIR}/Simbody/src/Assembler.cpp
    ${OPYN_SIMBODY_BASEDIR}/Simbody/src/AssemblyCondition_OrientationSensors.cpp
    ${OPYN_SIMBODY_BASEDIR}/Simbody/src/Constraint_SphereOnPlaneContact.cpp
    ${OPYN_SIMBODY_BASEDIR}/Simbody/src/CablePath.cpp
    ${OPYN_SIMBODY_BASEDIR}/Simbody/src/SimbodyMatterSubtree.cpp
    ${OPYN_SIMBODY_BASEDIR}/Simbody/src/Constraint_PointOnPlaneContact.cpp
    ${OPYN_SIMBODY_BASEDIR}/Simbody/src/GeneralContactSubsystem.cpp
    ${OPYN_SIMBODY_BASEDIR}/Simbody/src/CableTrackerSubsystem.cpp
    ${OPYN_SIMBODY_BASEDIR}/Simbody/src/ContactTrackerSubsystem.cpp
    ${OPYN_SIMBODY_BASEDIR}/Simbody/src/Force.cpp
    ${OPYN_SIMBODY_BASEDIR}/Simbody/src/Motion.cpp
    ${OPYN_SIMBODY_BASEDIR}/Simbody/src/Force_LinearBushing.cpp
    ${OPYN_SIMBODY_BASEDIR}/Simbody/src/LocalEnergyMinimizer.cpp
    ${OPYN_SIMBODY_BASEDIR}/Simbody/src/SmoothSphereHalfSpaceForce.cpp
    ${OPYN_SIMBODY_BASEDIR}/Simbody/src/HuntCrossleyContact.cpp
    ${OPYN_SIMBODY_BASEDIR}/Simbody/src/PLUSImpulseSolver.cpp
    ${OPYN_SIMBODY_BASEDIR}/Simbody/src/Constraint_SphereOnSphereContact.cpp
    ${OPYN_SIMBODY_BASEDIR}/Simbody/src/PGSImpulseSolver.cpp
    ${OPYN_SIMBODY_BASEDIR}/Simbody/src/ObservedPointFitter.cpp
    ${OPYN_SIMBODY_BASEDIR}/Simbody/src/TextDataEventReporter.cpp
    ${OPYN_SIMBODY_BASEDIR}/Simbody/src/MobilizedBody.cpp
    ${OPYN_SIMBODY_BASEDIR}/Simbody/src/Constraint_LineOnLineContact.cpp
    ${OPYN_SIMBODY_BASEDIR}/Simbody/src/DecorationSubsystem.cpp
    ${OPYN_SIMBODY_BASEDIR}/Simbody/src/Force_Gravity.cpp
    ${OPYN_SIMBODY_BASEDIR}/Simbody/src/HuntCrossleyForce.cpp
    ${OPYN_SIMBODY_BASEDIR}/Simbody/src/About.cpp
    ${OPYN_SIMBODY_BASEDIR}/Simbody/src/RigidBodyNode_Weld.cpp
    ${OPYN_SIMBODY_BASEDIR}/Simbody/src/Constraint.cpp
    ${OPYN_SIMBODY_BASEDIR}/Simbody/src/Force_Instantiations.cpp
    ${OPYN_SIMBODY_BASEDIR}/Simbody/src/ExponentialSpringParameters.cpp
    ${OPYN_SIMBODY_BASEDIR}/Simbody/src/RigidBodyNodeSpec_Derived.cpp
    ${OPYN_SIMBODY_BASEDIR}/Simbody/src/MultibodySystem.cpp
    ${OPYN_SIMBODY_BASEDIR}/Simbody/src/ForceSubsystem.cpp
    ${OPYN_SIMBODY_BASEDIR}/Simbody/src/SimbodyMatterSubsystem.cpp
    ${OPYN_SIMBODY_BASEDIR}/Simbody/src/AssemblyCondition_Markers.cpp
    ${OPYN_SIMBODY_BASEDIR}/Simbody/src/ConditionalConstraint.cpp
    ${OPYN_SIMBODY_BASEDIR}/Simbody/src/SemiExplicitEulerTimeStepper.cpp
    ${OPYN_SIMBODY_BASEDIR}/Simbody/src/ElasticFoundationForce.cpp
    ${OPYN_SIMBODY_BASEDIR}/Simbody/src/ExponentialSpringForce.cpp
    ${OPYN_SIMBODY_BASEDIR}/Simbody/src/Body.cpp
    ${OPYN_SIMBODY_BASEDIR}/Simbody/src/OBSOLETE_LengthConstraints.cpp
    ${OPYN_SIMBODY_BASEDIR}/Simbody/src/SimbodyMatterSubsystemRep.cpp
    ${OPYN_SIMBODY_BASEDIR}/Simbody/src/ImpulseSolver.cpp
    ${OPYN_SIMBODY_BASEDIR}/Simbody/src/Force_Thermostat.cpp
    ${OPYN_SIMBODY_BASEDIR}/Simbody/src/Matter_Instantiations.cpp
    ${OPYN_SIMBODY_BASEDIR}/Simbody/src/RigidBodyNodeSpec.cpp
    ${OPYN_SIMBODY_BASEDIR}/Simbody/src/CompliantContactSubsystem.cpp
    ${OPYN_SIMBODY_BASEDIR}/Simbody/Visualizer/src/Visualizer.cpp
    ${OPYN_SIMBODY_BASEDIR}/Simbody/Visualizer/src/VisualizerGeometry.cpp
    ${OPYN_SIMBODY_BASEDIR}/Simbody/Visualizer/src/VisualizerProtocol.cpp
    ${OPYN_SIMBODY_BASEDIR}/Simbody/Visualizer/src/Visualizer_InputListener.cpp
    ${OPYN_SIMBODY_BASEDIR}/Simbody/Visualizer/src/Visualizer_Reporter.cpp

    # OpenSim sources
    ${OPYN_OPENSIM_BASEDIR}/Vendors/lepton/src/ParsedExpression.cpp
    ${OPYN_OPENSIM_BASEDIR}/Vendors/lepton/src/ExpressionProgram.cpp
    ${OPYN_OPENSIM_BASEDIR}/Vendors/lepton/src/Operation.cpp
    ${OPYN_OPENSIM_BASEDIR}/Vendors/lepton/src/Parser.cpp
    ${OPYN_OPENSIM_BASEDIR}/Vendors/lepton/src/ExpressionTreeNode.cpp
    ${OPYN_OPENSIM_BASEDIR}/Vendors/lepton/src/CompiledExpression.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Analyses/InverseDynamics.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Analyses/MuscleAnalysis.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Analyses/JointReaction.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Analyses/BodyKinematics.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Analyses/PointKinematics.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Analyses/ProbeReporter.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Analyses/About.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Analyses/StaticOptimizationTarget.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Analyses/OutputReporter.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Analyses/ForceReporter.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Analyses/StatesReporter.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Analyses/IMUDataReporter.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Analyses/InducedAccelerations.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Analyses/StaticOptimization.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Analyses/InducedAccelerationsSolver.cpp
    # ${OPYN_OPENSIM_BASEDIR}/OpenSim/Analyses/osimAnalysesDLL.cpp  # unused by libopynsim (uses `opyn::init`)
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Analyses/RegisterTypes_osimAnalyses.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Analyses/Actuation.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Analyses/Kinematics.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/ComponentOutput.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/OptimizationTarget.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/PropertyGroup.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/FileAdapter.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/ScaleSet.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/PiecewiseLinearFunction.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/XYFunctionInterface.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/MarkerData.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/ComponentList.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/PiecewiseConstantFunction.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/PropertyDblArray.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/Lmdif.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/Component.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/PropertyObj.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/DataAdapter.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/PropertyTransform.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/Units.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/IO.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/Assertion.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/Exception.cpp
    # ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/LoadOpenSimLibrary.cpp  # unused libopynsim (no native plugin support)
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/PropertyBool.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/ExpressionBasedFunction.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/ComponentSocket.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/SignalGenerator.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/osimCommonTemplates.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/ObjectGroup.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/PropertyDbl.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/XMLDocument.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/LatinHypercubeDesign.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/STOFileAdapter.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/VectorFunctionUncoupledNxN.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/VectorFunction.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/StateVector.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/Object.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/XsensDataReader.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/DebugUtilities.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/SmoothSegmentedFunctionFactory.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/PropertyStr.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/TableUtilities.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/MarkerFrame.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/SimmSpline.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/PropertyInt.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/LinearFunction.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/About.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/SimmIO.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/PropertyStrArray.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/PropertyIntArray.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/GCVSplineSet.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/APDMDataReader.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/PropertySet.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/SegmentedQuinticBezierToolkit.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/MultiplierFunction.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/Scale.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/PropertyTable.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/Property_Deprecated.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/IMUDataReader.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/AbstractDataTable.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/TRCFileAdapter.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/AbstractProperty.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/ComponentPath.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/Storage.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/PropertyBoolArray.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/RootSolver.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/FunctionSet.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/Signal.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/Property.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/GCVSpline.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/SmoothSegmentedFunction.cpp
    # ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/osimCommonDLL.cpp  # unused by libopynsim (uses `opyn::init`)
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/MultivariatePolynomialFunction.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/StepFunction.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/FunctionAdapter.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/Function.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/CSVFileAdapter.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/Constant.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/CommonUtilities.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/Logger.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/Reporter.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Common/RegisterTypes_osimCommon.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/SimulationUtilities.cpp
    # ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/osimSimulationDLL.cpp  # unused by libopynsim (uses `opyn::init`)
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/VisualizerUtilities.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Control/Control.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Control/TrackingController.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Control/Controller.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Control/ControlSet.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Control/SynergyController.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Control/PrescribedController.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Control/ControlSetController.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Control/ControlConstant.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Control/InputController.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Control/ControlLinear.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Control/ControlLinearNode.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/StatesTrajectory.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/Scholz2015GeometryPath.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/Analysis.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/AbstractPathPoint.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/Blankevoort1991Ligament.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/CMCActuatorSubsystem.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/ExpressionBasedPathForce.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/JointInternalPowerProbe.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/Umberger2010MuscleMetabolicsProbe.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/Probe.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/ContactGeometry.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/GeometryPath.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/PathActuator.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/ExpressionBasedBushingForce.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/PhysicalFrame.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/ForceApplier.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/PrescribedForce.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/ExpressionBasedPointToPointForce.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/ActivationFiberLengthMuscle_Deprecated.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/ModelComponent.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/FunctionBasedPath.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/Ligament.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/Force.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/Marker.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/BodyScaleSet.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/ModelVisualizer.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/SmoothSphereHalfSpaceForce.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/Point.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/ExponentialContactForce.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/ExternalForce.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/MarkerSet.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/CoordinateSet.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/AbstractGeometryPath.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/PointToPointSpring.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/ForceProducer.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/Bhargava2004MuscleMetabolicsProbe.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/AnalysisSet.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/PathPoint.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/Actuator.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/HuntCrossleyForce.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/ControllerSet.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/ActivationFiberLengthMuscle.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/Station.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/BushingForce.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/ActuatorForceProbe.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/CoordinateLimitForce.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/Model.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/FunctionBasedBushingForce.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/Frame.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/StationDefinedFrame.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/ContactMesh.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/PhysicalOffsetFrame.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/SystemEnergyProbe.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/ForceAdapter.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/MuscleActiveFiberPowerProbe.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/Geometry.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/ExternalLoads.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/ElasticFoundationForce.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/ConditionalPathPoint.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/ActuatorPowerProbe.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/MovingPathPoint.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/ExpressionBasedCoordinateForce.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/ForceSet.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/BodyScale.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/Muscle.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/ContactHalfSpace.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/FunctionThresholdCondition.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/AbstractTool.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/PathSpring.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/Condition.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/Ground.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Model/Bhargava2004SmoothedMuscleMetabolics.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/MarkersReference.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/AssemblySolver.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Manager/Manager.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/About.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/RegisterTypes_osimSimulation.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Wrap/WrapEllipsoid.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Wrap/WrapObject.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Wrap/WrapResult.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Wrap/PathWrap.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Wrap/PathWrapSet.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Wrap/PathWrapPoint.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Wrap/WrapMath.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Wrap/WrapTorus.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Wrap/WrapCylinder.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/Wrap/WrapSphere.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/InverseDynamicsSolver.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/MomentArmSolver.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/InverseKinematicsSolver.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/OpenSense/OpenSenseUtilities.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/OpenSense/IMUPlacer.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/BufferedOrientationsReference.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/SimbodyEngine/PlanarJoint.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/SimbodyEngine/PointOnLineConstraint.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/SimbodyEngine/SimbodyEngine.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/SimbodyEngine/PointConstraint.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/SimbodyEngine/ConstantCurvatureJoint.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/SimbodyEngine/WeldJoint.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/SimbodyEngine/UniversalJoint.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/SimbodyEngine/SpatialTransform.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/SimbodyEngine/UnilateralConstraint.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/SimbodyEngine/BallJoint.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/SimbodyEngine/PinJoint.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/SimbodyEngine/WeldConstraint.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/SimbodyEngine/CoordinateCouplerConstraint.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/SimbodyEngine/Joint.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/SimbodyEngine/Constraint.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/SimbodyEngine/RollingOnSurfaceConstraint.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/SimbodyEngine/ScapulothoracicJoint.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/SimbodyEngine/CustomJoint.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/SimbodyEngine/ConstantDistanceConstraint.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/SimbodyEngine/EllipsoidJoint.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/SimbodyEngine/Body.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/SimbodyEngine/Coordinate.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/SimbodyEngine/TransformAxis.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/SimbodyEngine/SliderJoint.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/SimbodyEngine/FreeJoint.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/SimbodyEngine/GimbalJoint.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/PositionMotion.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/StatesTrajectoryReporter.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/CoordinateReference.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Simulation/OrientationsReference.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Tools/MarkerPairSet.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Tools/CMC_Orientation.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Tools/GenericModelMaker.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Tools/CorrectionController.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Tools/InverseDynamicsTool.cpp
    # ${OPYN_OPENSIM_BASEDIR}/OpenSim/Tools/osimToolsDLL.cpp  # unused by libopynsim (uses `opyn::init`)
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Tools/Measurement.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Tools/ActuatorForceTargetFast.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Tools/ForwardTool.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Tools/DynamicsTool.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Tools/VectorFunctionForActuators.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Tools/ActuatorForceTarget.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Tools/CMC_Point.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Tools/RRATool.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Tools/About.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Tools/IKCoordinateTask.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Tools/MarkerPlacer.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Tools/AnalyzeTool.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Tools/CMCTool.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Tools/ModelScaler.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Tools/IMUInverseKinematicsTool.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Tools/MeasurementSet.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Tools/TrackingTask.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Tools/CMC.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Tools/ScaleTool.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Tools/CMC_Task.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Tools/SMC_Joint.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Tools/CMC_Joint.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Tools/RegisterTypes_osimTools.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Tools/InverseKinematicsTool.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Tools/CMC_TaskSet.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Tools/MuscleStateTrackingTask.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Tools/MarkerPair.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Actuators/ModelFactory.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Actuators/Thelen2003Muscle.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Actuators/MuscleSecondOrderActivationDynamicModel.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Actuators/RigidTendonMuscle.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Actuators/TendonForceLengthCurve.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Actuators/MuscleFirstOrderActivationDynamicModel.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Actuators/DeGrooteFregly2016Muscle.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Actuators/FiberCompressiveForceLengthCurve.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Actuators/ClutchedPathSpring.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Actuators/FirstOrderMuscleActivationDynamics.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Actuators/MuscleActivationDynamics.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Actuators/Thelen2003Muscle_Deprecated.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Actuators/Millard2012EquilibriumMuscle.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Actuators/MuscleFixedWidthPennationModel.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Actuators/PointActuator.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Actuators/ForceVelocityInverseCurve.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Actuators/PointToPointActuator.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Actuators/About.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Actuators/PolynomialPathFitter.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Actuators/ZerothOrderMuscleActivationDynamics.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Actuators/Delp1990Muscle_Deprecated.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Actuators/FiberForceLengthCurve.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Actuators/CoordinateActuator.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Actuators/ConstantMuscleActivation.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Actuators/Millard2012AccelerationMuscle.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Actuators/TorqueActuator.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Actuators/ActiveForceLengthCurve.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Actuators/BodyActuator.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Actuators/ForceVelocityCurve.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Actuators/McKibbenActuator.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Actuators/Schutte1993Muscle_Deprecated.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Actuators/SpringGeneralizedForce.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Actuators/FiberCompressiveForceCosPennationCurve.cpp
    # ${OPYN_OPENSIM_BASEDIR}/OpenSim/Actuators/osimActuatorsDLL.cpp  # unused by libopynsim (uses `opyn::init`)
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/Actuators/RegisterTypes_osimActuators.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/ExampleComponents/RegisterTypes_osimExampleComponents.cpp
    ${OPYN_OPENSIM_BASEDIR}/OpenSim/ExampleComponents/ToyReflexController.cpp

    # jam-plugin sources
    jam-plugin/Smith2018ArticularContactForce.cpp
    jam-plugin/Smith2018ContactMesh.cpp
)
target_include_directories(osim
PUBLIC
    # Simbody includes
    $<BUILD_INTERFACE:${OPYN_SIMBODY_BASEDIR}/SimTKmath/Integrators/src/CPodes/sundials/include>
    $<BUILD_INTERFACE:${OPYN_SIMBODY_BASEDIR}/SimTKmath/Integrators/include>
    $<BUILD_INTERFACE:${OPYN_SIMBODY_BASEDIR}/SimTKmath/include>
    $<BUILD_INTERFACE:${OPYN_SIMBODY_BASEDIR}/SimTKmath/Geometry/include>
    $<BUILD_INTERFACE:${OPYN_SIMBODY_BASEDIR}/SimTKcommon/SmallMatrix/include>
    $<BUILD_INTERFACE:${OPYN_SIMBODY_BASEDIR}/SimTKcommon/include>
    $<BUILD_INTERFACE:${OPYN_SIMBODY_BASEDIR}/SimTKcommon/Random/include>
    $<BUILD_INTERFACE:${OPYN_SIMBODY_BASEDIR}/SimTKcommon/Simulation/include>
    $<BUILD_INTERFACE:${OPYN_SIMBODY_BASEDIR}/SimTKcommon/Geometry/include>
    $<BUILD_INTERFACE:${OPYN_SIMBODY_BASEDIR}/SimTKcommon/BigMatrix/include>
    $<BUILD_INTERFACE:${OPYN_SIMBODY_BASEDIR}/SimTKcommon/Scalar/include>
    $<BUILD_INTERFACE:${OPYN_SIMBODY_BASEDIR}/SimTKcommon/Mechanics/include>
    $<BUILD_INTERFACE:${OPYN_SIMBODY_BASEDIR}/SimTKcommon/Polynomial/include>
    $<BUILD_INTERFACE:${OPYN_SIMBODY_BASEDIR}/Simbody/include>
    $<BUILD_INTERFACE:${OPYN_SIMBODY_BASEDIR}/Simbody/Visualizer/include>
    $<INSTALL_INTERFACE:include/simbody>

    # OpenSim includes
    $<BUILD_INTERFACE:${OPYN_OPENSIM_BASEDIR}>
    $<BUILD_INTERFACE:${OPYN_OPENSIM_BASEDIR}/Vendors/lepton/include>
    $<INSTALL_INTERFACE:include>

    # jam-plugin includes
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
target_compile_definitions(osim
PRIVATE
    # Simbody defines
    SimTK_SIMMATH_MAJOR_VERSION=${PROJECT_VERSION_MAJOR}
    SimTK_SIMMATH_MINOR_VERSION=${PROJECT_VERSION_MINOR}
    SimTK_SIMMATH_PATCH_VERSION=${PROJECT_VERSION_PATCH}
    SimTK_SIMMATH_BUILD_VERSION=${PROJECT_VERSION_PATCH}
    SimTK_SimTKCOMMON_MAJOR_VERSION=${PROJECT_VERSION_MAJOR}
    SimTK_SimTKCOMMON_MINOR_VERSION=${PROJECT_VERSION_MINOR}
    SimTK_SimTKCOMMON_PATCH_VERSION=${PROJECT_VERSION_PATCH}
    SimTK_SIMBODY_MAJOR_VERSION=${PROJECT_VERSION_MAJOR}
    SimTK_SIMBODY_MINOR_VERSION=${PROJECT_VERSION_MINOR}
    SimTK_SIMBODY_PATCH_VERSION=${PROJECT_VERSION_PATCH}
    SIMBODY_VISUALIZER_INSTALL_DIR="${CMAKE_INSTALL_FULL_BINDIR}"     # shuts the compiler up
    SIMBODY_VISUALIZER_REL_INSTALL_DIR="${CMAKE_INSTALL_BINDIR}"      # shuts the compiler up
    SIMBODY_PATH_FROM_LIBDIR_TO_VIZ_DIR="../${CMAKE_INSTALL_BINDIR}"  # shuts the compiler up

    # OpenSim defines
    OPENSIM_ACTUATORS_MAJOR_VERSION=${PROJECT_VERSION_MAJOR}
    OPENSIM_ACTUATORS_MINOR_VERSION=${PROJECT_VERSION_MINOR}
    OPENSIM_ACTUATORS_BUILD_VERSION=${PROJECT_VERSION_PATCH}
    OPENSIM_ANALYSES_MAJOR_VERSION=${PROJECT_VERSION_MAJOR}
    OPENSIM_ANALYSES_MINOR_VERSION=${PROJECT_VERSION_MINOR}
    OPENSIM_ANALYSES_BUILD_VERSION=${PROJECT_VERSION_PATCH}
    OPENSIM_COMMON_MAJOR_VERSION=${PROJECT_VERSION_MAJOR}
    OPENSIM_COMMON_MINOR_VERSION=${PROJECT_VERSION_MINOR}
    OPENSIM_COMMON_BUILD_VERSION=${PROJECT_VERSION_PATCH}
    OPENSIM_SIMULATION_MAJOR_VERSION=${PROJECT_VERSION_MAJOR}
    OPENSIM_SIMULATION_MINOR_VERSION=${PROJECT_VERSION_MINOR}
    OPENSIM_SIMULATION_BUILD_VERSION=${PROJECT_VERSION_PATCH}
    OPENSIM_TOOLS_MAJOR_VERSION=${PROJECT_VERSION_MAJOR}
    OPENSIM_TOOLS_MINOR_VERSION=${PROJECT_VERSION_MINOR}
    OPENSIM_TOOLS_BUILD_VERSION=${PROJECT_VERSION_PATCH}
    OPENSIM_DISABLE_LOG_FILE=1  # disable unconditionally writing `opensim.log` in the process's working dir

PUBLIC
    # these ensure that `declspec(import)/declspec(export)` isn't used
    SimTK_USE_STATIC_LIBRARIES=1
    LEPTON_BUILDING_STATIC_LIBRARY=1
    OPENSIM_USE_STATIC_LIBRARIES=1
    OPENSIM_DISABLE_STATIC_TYPE_REGISTRATION=1  # disable objects being registered during static init time

    # This disables the operators from SimTK that newer compilers have problems with
    # see https://github.com/simbody/simbody/pull/815 for details
    SWIG_PYTHON
)
target_compile_features(osim PUBLIC cxx_std_20)

target_link_libraries(osim
PUBLIC
    ${CMAKE_DL_LIBS} # used by simbody (plugin loaders)
    BLAS::BLAS       # used by simbody (linear algebra)
    LAPACK::LAPACK   # used by simbody (linear algebra)
    spdlog::spdlog   # used by OpenSim (logging)
)
set_target_properties(osim PROPERTIES
    CXX_EXTENSIONS OFF
    CXX_CLANG_TIDY            ""  # do not statically analyze third-party code
)
if(OPYN_BUILD_PYTHON_BINDINGS)
    set_target_properties(osim PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

# Install osim such that it is header-compatible with OpenSim and
# simbody projects (unofficial legacy project support).
if(TRUE)

    # install `osimTargets.cmake` to `lib/cmake/osim`
    install(TARGETS osim EXPORT osimTargets COMPONENT osim-cpp)
    install(EXPORT osimTargets DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/osim COMPONENT osim-cpp)

    # install `osimConfig.cmake` to `lib/cmake/osim`
    configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/osimConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/generated/osimConfig.cmake"
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/osim
    )
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/generated/osimConfig.cmake" DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/osim" COMPONENT osim-cpp)

    # install simbody headers to `include/simbody` (compatible with `simbody`'s install)
    install(
        DIRECTORY
            ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Integrators/include/
            ${OPYN_SIMBODY_BASEDIR}/SimTKmath/include/
            ${OPYN_SIMBODY_BASEDIR}/SimTKmath/Geometry/include/
            ${OPYN_SIMBODY_BASEDIR}/SimTKcommon/SmallMatrix/include/
            ${OPYN_SIMBODY_BASEDIR}/SimTKcommon/include/
            ${OPYN_SIMBODY_BASEDIR}/SimTKcommon/Random/include/
            ${OPYN_SIMBODY_BASEDIR}/SimTKcommon/Simulation/include/
            ${OPYN_SIMBODY_BASEDIR}/SimTKcommon/Geometry/include/
            ${OPYN_SIMBODY_BASEDIR}/SimTKcommon/BigMatrix/include/
            ${OPYN_SIMBODY_BASEDIR}/SimTKcommon/Scalar/include/
            ${OPYN_SIMBODY_BASEDIR}/SimTKcommon/Mechanics/include/
            ${OPYN_SIMBODY_BASEDIR}/SimTKcommon/Polynomial/include/
            ${OPYN_SIMBODY_BASEDIR}/Simbody/include/
            ${OPYN_SIMBODY_BASEDIR}/Simbody/Visualizer/include/

        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/simbody"
        COMPONENT osim-cpp
    )

    # install lepton headers to `include/lepton` (compatible with `opensim-core`'s install)
    file(GLOB_RECURSE OPYN_GLOBBED_LEPTON_HEADERS "${OPYN_OPENSIM_BASEDIR}/Vendors/lepton/include/lepton/*.h")
    install(FILES ${OPYN_GLOBBED_LEPTON_HEADERS} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/lepton" COMPONENT osim-cpp)

    # install OpenSim headers to `include/OpenSim` (compatible with `opensim-core`'s install)
    set(OPYN_EXCLUDED_PATTERNS "/Test/" "/Tests/" "/Examples/" "/Moco/" "/Sandbox/" "C3DFileAdapter")
    file(GLOB_RECURSE OPYN_GLOBBED_HEADERS "${OPYN_OPENSIM_BASEDIR}/OpenSim/*.h")
    foreach(var ${OPYN_GLOBBED_HEADERS})
        set(OPYN_EXCLUDE FALSE)
        foreach(pattern ${OPYN_EXCLUDED_PATTERNS})
            if("${var}" MATCHES "${pattern}")
                set(OPYN_EXCLUDE TRUE)
            endif()
        endforeach()
        if(NOT OPYN_EXCLUDE)
            list(APPEND OPYN_HEADER_FILES "${var}")
        endif()
    endforeach()
    foreach(header ${OPYN_HEADER_FILES})
        file(RELATIVE_PATH header_relpath ${OPYN_OPENSIM_BASEDIR} ${header})
        get_filename_component(DIR ${header_relpath} DIRECTORY)
        install(FILES ${header} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${DIR} COMPONENT osim-cpp)
    endforeach()
endif()
