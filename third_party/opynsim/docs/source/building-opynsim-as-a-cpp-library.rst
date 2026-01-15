Building OPynSim As a C++ Library
=================================


.. warning::

    **The OPynSim C++ API is unstable.** This guide is aimed at C++ developers that
    accept the risks.
    
    Apart from a few patches, the OpenSim and Simbody API are pulled as-is from their
    central repositories, so their API stability is the same as those upstreams
    (colloquially, both OpenSim and Simbody have a very stable C++ API). This means
    that, in principle, you can write extensions to OpenSim/Simbody against OPynSim,
    which might be easier to initially setup, and then port and upstream that C++ code
    to the central OpenSim and Simbody repositories (which we encourage you to do!)

    The OPynSim C++ API (i.e. anything in ``libopynsim/`` or namespaced with ``opyn::``)
    is currently an **internal** API that's designed and managed by the OPynSim development
    team. It's specifically to service the OPynSim python API (public) and OpenSim Creator
    (internal). Therefore, if we think it makes sense to refactor/break an OPynSim C++
    API to better-service those needs, we will.
    
    Over time, as the OPynSim project matures, we anticipate that the C++ will also stabilize
    and, when we have sufficient resources to provide support for it, we may make the C++
    API public and stable.

    All of this is to say, it's possible to use the OPynSim C++ API in your downstream
    projects, but you should probably freeze the OPynSim version your downstream project
    uses; otherwise, upgrades to OPynSim might break the C++ you have written
    downstream. Any issues/emails/requests with content like 'your change broke my
    C++ code' will be responsed with something along the lines of 'LOL RTFM'.

    Otherwise, have fun 😉


Overview
--------

OPynSim has the following architectural layout.