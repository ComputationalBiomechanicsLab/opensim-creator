# ChangeLog

All notable changes to this project will be documented here. The format is based
on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).


## [Upcoming Release] - YYYY/MM/DD

- Patched an issue in OpenSim where it wasn't correctly clearing out memory
  when a model contained graph cycles (slave bodies, etc.).
- Patched an issue in OpenSim where it could segfault with use-after-free
  issues when re-finalizing a model containing graph cycles over-and-over.

