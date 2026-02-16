# ChangeLog

All notable changes to this project will be documented here. The format is based
on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).


## [Upcoming Release] - YYYY/MM/DD

- Patched an issue in OpenSim where it wasn't correctly clearing out memory
  when a model contained graph cycles (slave bodies, etc.).
- Patched an issue in OpenSim where it could segfault with use-after-free
  issues when re-finalizing a model containing graph cycles over-and-over.
- Updated OpenSim from `30430e4feecfef3f6385140c72b8e34a54d76d3e` to `286b1f60f147ae707edaa0693931d750305ae50b`, 
  which includes new `MeyerFregly2016Force` and `MeyerFregly2016Muscle` components.
- Updated simbody to from `f9ab12cbad9d0da106473259d34c50577f934f49` to `7f35622b3c5daac919fc39a2865498c03c553e53`,
  which contains some cable path fixes.
