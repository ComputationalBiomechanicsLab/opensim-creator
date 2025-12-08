Rajagopal et al. (2016) full-body musculoskeletal model
=======================================================

These files are modified from the original distribution, which can be found on SimTK.org:
- https://simtk.org/projects/full_body

Models
------

The model 'Rajagopal2016.osim' is the full-body musculoskeletal model described in
Rajagopal et al. (2016), with afew minor modifications. First, the model file format
has been updated to the latest XML format as of OpenSim 4.5. Second, the 'fiber_damping'
property of the Millard2012EquilibriumMuscle has been set to 0.1 for all muscles to
match the recommendation in Millard et al. (2013).

The model 'RajagopalLaiUhlrich2023.osim' is a modified version of the original
Rajagopal et al. (2016) model. The model includes modifications from Lai et al. (2017) to
enable simulations of high knee and hip flexion. The modifications include increased range
of knee flexion, updated knee muscle paths, and modified force-generating properties of
eleven muscles. The model also includes modifications from Uhlrich et al. (2022). These
modifications included calibrated passive muscle force curves to better match experimental
data published by Silder et al. (2007), and updated hip abductor muscle paths that more closely
align with MRI and experimental data.

The SimTK project pages for the Lai et al. (2017) and Uhlrich et al. (2022)
publications can be found here:
- https://simtk.org/projects/model-high-flex
- https://simtk.org/projects/fbmodpassivecal

Scripts
-------

The MATLAB scripts under the '/scripts' can be used to run a sample simulation pipeline
including model scaling, inverse kinematics, inverse dynamics, residual reduction algorithm, and
computed muscle control. These scripts have been slightly modified from the original distribution
to be compatible with OpenSim 4.5 and later versions.

The script 'runAll.m' can be used to run the entire pipeline for a walking and running trial. The
setup XML files for the pipeline look for a model file named 'generic_unscaled.osim' in the main
directory. By default, this model is a copy of 'Rajagopal2016.osim', but it can be replaced to run
the pipeline with the model defined in 'RajagopalLaiUhlrich2023.osim' if desired.

References
----------

Rajagopal, A., Dembia, C.L., DeMers, M.S., Delp, D.D., Hicks, J.L., Delp, S.L. (2016)
    Full-body musculoskeletal model for muscle-driven simulation of human gait.
    IEEE Transactions on Biomedical Engineering.
    doi: https://doi.org/10.1109/TBME.2016.2586891

Lai, A.K.M., Arnold, A.S., Wakeling, J.M. (2017)
    Why are antagonist muscles co-activated in my simulation? A musculoskeletal model for
    analysing human locomotor tasks. Annals of Biomedical Engineering.
    doi: https://doi.org/10.1007/s10439-017-1920-7

Uhlrich, S.D., Jackson, R.W., Seth, A., Kolesar, J.A., Delp, S.L. (2022)
    Muscle coordination retraining inspired by musculoskeletal simulations reduces knee
    contact force. Scientific Reports.
    doi: https://doi.org/10.1038/s41598-022-13386-9

Millard, M., Uchida, T., Seth, A., Delp, S.L. (2013)
    Flexing computational muscle: modeling and simulation of musculotendon dynamics.
    Journal of Biomechanical Engineering.
    doi: https://doi.org/10.1115/1.4023390

Silder, A., Whittington, B., Heiderscheit, B., Thelen, D.G. (2007)
    Identification of passive elastic joint moment-angle relationships in the lower extremity.
    Journal of Biomechanics.
    doi: https://doi.org/10.1016/j.jbiomech.2006.12.017