#ifndef __PointForceDirection_h__
#define __PointForceDirection_h__
/* -------------------------------------------------------------------------- *
 *                      OpenSim:  PointForceDirection.h                       *
 * -------------------------------------------------------------------------- *
 * The OpenSim API is a toolkit for musculoskeletal modeling and simulation.  *
 * See http://opensim.stanford.edu and the NOTICE file for more information.  *
 * OpenSim is developed at Stanford University and supported by the US        *
 * National Institutes of Health (U54 GM072970, R24 HD065690) and by DARPA    *
 * through the Warrior Web program.                                           *
 *                                                                            *
 * Copyright (c) 2005-2024 Stanford University and the Authors                *
 * Author(s): Ajay Seth, Adam Kewley                                          *
 *                                                                            *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may    *
 * not use this file except in compliance with the License. You may obtain a  *
 * copy of the License at http://www.apache.org/licenses/LICENSE-2.0.         *
 *                                                                            *
 * Unless required by applicable law or agreed to in writing, software        *
 * distributed under the License is distributed on an "AS IS" BASIS,          *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   *
 * See the License for the specific language governing permissions and        *
 * limitations under the License.                                             *
 * -------------------------------------------------------------------------- */

#include <OpenSim/Simulation/Model/PhysicalFrame.h>
#include <OpenSim/Simulation/osimSimulationDLL.h>

namespace OpenSim {

class Body;
class PhysicalFrame;

/**
 * Convenience class for a generic representation of geometry of a complex
 * Force (or any other object) with multiple points of contact through
 * which forces are applied to bodies. This represents one such point and an
 * array of these objects defines a complete Force distribution (i.e., path).
 *
 * @author Ajay Seth
 * @version 1.0
 */
class OSIMSIMULATION_API PointForceDirection final {
public:
    PointForceDirection(
        SimTK::Vec3 point,
        const PhysicalFrame& frame,
        SimTK::Vec3 direction) :

        _point(point), _frame(&frame), _direction(direction)
    {}

    [[deprecated("the 'scale' functionality should not be used in new code: OpenSim already assumes 'direction' is non-unit-length")]]
    PointForceDirection(
        SimTK::Vec3 point,
        const PhysicalFrame& frame,
        SimTK::Vec3 direction,
        double scale) :

        _point(point), _frame(&frame), _direction(direction), _scale(scale)
    {}

    /** Returns the point of "contact", defined in `frame()` */
    SimTK::Vec3 point() { return _point; }

    /** Returns the frame in which `point()` is defined */
    const PhysicalFrame& frame() { return *_frame; }

    /** Returns the (potentially, non-unit-length) direction, defined in ground, of the force at `point()` */
    SimTK::Vec3 direction() { return _direction; }

    /** Returns the scale factor of the force */
    [[deprecated("this functionality should not be used in new code: OpenSim already assumes 'direction' is non-unit-length")]]
    double scale() { return _scale; }

    /** Replaces the current direction with `direction + newDirection` */
    void addToDirection(SimTK::Vec3 newDirection) { _direction += newDirection; }

private:
    /** Point of "contact" with a body, defined in the body frame */
    SimTK::Vec3 _point;

    /** The frame in which the point is defined */
    const PhysicalFrame* _frame;

    /** Direction of the force at the point, defined in ground */
    SimTK::Vec3 _direction;

    /** Deprecated parameter to scale the force that results from a scalar
    (tension) multiplies the direction */
    double _scale = 1.0;
};

} // namespace

#endif // __PointForceDirection_h__
