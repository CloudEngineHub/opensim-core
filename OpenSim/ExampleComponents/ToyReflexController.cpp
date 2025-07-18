/* -------------------------------------------------------------------------- *
 *                      OpenSim:  ToyReflexController.cpp                     *
 * -------------------------------------------------------------------------- *
 * The OpenSim API is a toolkit for musculoskeletal modeling and simulation.  *
 * See http://opensim.stanford.edu and the NOTICE file for more information.  *
 * OpenSim is developed at Stanford University and supported by the US        *
 * National Institutes of Health (U54 GM072970, R24 HD065690) and by DARPA    *
 * through the Warrior Web program.                                           *
 *                                                                            *
 * Copyright (c) 2005-2017 Stanford University and the Authors                *
 * Author(s): Ajay Seth                                                       *
 *                                                                            *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may    *
 * not use this file except in compliance with the License. You may obtain a  *
 * copy of the License at http://www.apache.org/licenses/LICENSE-2.0.         *
 *                                                                            *
 * Unless required by applicable law or agreed to in writing, software        *
 * distributed under the License is distributed on an "AS IS" BASIS,          *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied    *
 * See the License for the specific language governing permissions and        *
 * limitations under the License.                                             *
 * -------------------------------------------------------------------------- */



//=============================================================================
// INCLUDES
//=============================================================================
#include "ToyReflexController.h"
#include <OpenSim/Simulation/Model/Muscle.h> 

// This allows us to use OpenSim functions, classes, etc., without having to
// prefix the names of those things with "OpenSim::".
using namespace OpenSim;
using namespace std;

//=============================================================================
// CONSTRUCTOR(S) AND DESTRUCTOR
//=============================================================================
//_____________________________________________________________________________
/* Default constructor. */
ToyReflexController::ToyReflexController()
{
    constructProperties();
}

/* Convenience constructor. */
ToyReflexController::ToyReflexController(double gain)
{
    constructProperties();
    set_gain(gain);
}


/*
 * Construct Properties
 */
void ToyReflexController::constructProperties()
{
    constructProperty_gain(1.0);
}

void ToyReflexController::extendConnectToModel(Model &model)
{
    Super::extendConnectToModel(model);

    const auto& socket = getSocket<Actuator>("actuators");
    for (int i = 0; i < (int)socket.getNumConnectees(); ++i) {
        const auto& actu = socket.getConnectee(i);
        const auto* musc = dynamic_cast<const Muscle*>(&actu);
        OPENSIM_THROW_IF_FRMOBJ(!musc, Exception,
            "Expected only muscle actuators assigned to this controller's "
            "'actuators' socket, but the non-muscle actuator '{}' was found.",
            actu.getName());
    }
}

//=============================================================================
// COMPUTATIONS
//=============================================================================
//_____________________________________________________________________________
/**
 * Compute the controls for muscles under influence of this reflex controller
 *
 * @param s         current state of the system
 * @param controls  system wide controls to which this controller can add
 */
void ToyReflexController::computeControls(
        const SimTK::State& s, SimTK::Vector& controls) const {

    // Get the Socket to the list of actuators assigned to the reflex controller.
    const auto& socket = getSocket<Actuator>("actuators");

    // muscle lengthening speed
    double speed = 0;
    // max muscle lengthening (stretch) speed
    double max_speed = 0;
    //reflex control
    double control = 0;

    for (int i = 0; i < (int)socket.getNumConnectees(); ++i) {
        const auto& actu = socket.getConnectee(i);
        const Muscle *musc = dynamic_cast<const Muscle*>(&actu);
        speed = musc->getLengtheningSpeed(s);
        // un-normalize muscle's maximum contraction velocity (fib_lengths/sec) 
        max_speed =
            musc->getOptimalFiberLength()*musc->getMaxContractionVelocity();
        control = 0.5*get_gain()*(fabs(speed)+speed)/max_speed;

        SimTK::Vector actControls(1,control);
        // add reflex controls to whatever controls are already in place.
        musc->addInControls(actControls, controls);
    }
}
