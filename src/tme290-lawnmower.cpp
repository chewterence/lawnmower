/*
 * Copyright (C) 2019 Ola Benderius
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>

#include "cluon-complete.hpp"
#include "tme290-sim-grass-msg.hpp"

// Definitions and enumerations
enum State { RETURN_TO_CHARGE, STAY_AND_CHARGE, RETURN_TO_LASTCUT, TRANSIENT_CUT, STATIONARY_CUT, STORE_LASTCUT, ERROR };
static const int MOVE_STAY = 0;
static const int MOVE_TOP_LEFT = 1;
static const int MOVE_TOP_CENTRE = 2;
static const int MOVE_TOP_RIGHT = 3;
static const int MOVE_RIGHT = 4;
static const int MOVE_BOTTOM_RIGHT = 5;
static const int MOVE_BOTTOM_CENTRE = 6;
static const int MOVE_BOTTOM_LEFT = 7;
static const int MOVE_LEFT = 8;

State updateState(State currentState, float battery, float batteryDrainThreshold, int32_t i, int32_t j, int32_t lastcut_i, int32_t lastcut_j, float grassCentre, float transientCutGrassThreshold, float rain, float rainAvoidanceThreshold) {
  switch (currentState) {
    case TRANSIENT_CUT:
      // Only stay and cut if too long
      if (battery < batteryDrainThreshold) {
        return STORE_LASTCUT;
      }
      if (grassCentre > transientCutGrassThreshold && rain < rainAvoidanceThreshold) {
        return STATIONARY_CUT;
      }
      else {
        return TRANSIENT_CUT;
      }
    case STORE_LASTCUT:
      return RETURN_TO_CHARGE;
    case RETURN_TO_CHARGE:
      if (i == 0 && j == 0) {
        return STAY_AND_CHARGE;
      }
      else {
        return RETURN_TO_CHARGE;
      }
    case STATIONARY_CUT:
      if (battery < batteryDrainThreshold) {
        return STORE_LASTCUT;
      }
      return TRANSIENT_CUT;
    case STAY_AND_CHARGE:
      if (battery < 0.98) {
        return STAY_AND_CHARGE;
      }
      else {
        return RETURN_TO_LASTCUT;
      }
    case RETURN_TO_LASTCUT:
      // If already reached the point, then change state
      if (i == lastcut_i && j == lastcut_j) {
        return STATIONARY_CUT;
      }
      else if (grassCentre > transientCutGrassThreshold) {
        return STATIONARY_CUT;
      }
      else {
        return RETURN_TO_LASTCUT;
      }
    default:
      grassCentre = grassCentre;
      return ERROR;
  }
}

int transientCut(int32_t i, int32_t j, float grassCentre, float transientCutGrassThreshold) {
  // Stay and cut if too long
  if (grassCentre > transientCutGrassThreshold) {
    return MOVE_STAY;
  }
  // Otherwise move in the default general direction
  if (j == 16 && i < 25) {
    return MOVE_RIGHT;
  }
  if (j < 17) {
    // Random chance to deviate from the general direction
    if (std::rand() % 4 <= 1) {
      return MOVE_RIGHT;
    }
    else if (std::rand() % 4 == 2) {
      return MOVE_BOTTOM_CENTRE;
    }
    else {
      return MOVE_BOTTOM_RIGHT; // Ideal general direction
    }
  }

    // Random chance to deviate from the general direction
    if (std::rand() % 3 == 0) {
      return MOVE_BOTTOM_CENTRE;
    }
    else if (std::rand() % 3 == 1) {
      return MOVE_LEFT;
    }
    else {
      return MOVE_BOTTOM_LEFT; // Ideal general direction
    }
}

int stationaryCut(float grassTopLeft, float grassTopCentre, float grassTopRight, float grassRight, float grassBottomRight, float grassBottomCentre, float grassBottomLeft, float grassLeft) {

  float maxGrassHeight = 0.0;
  int maxGrassDir = MOVE_STAY;

  // Find the maximum grass and go to that direction
  if (grassTopLeft > maxGrassHeight) {
    maxGrassHeight = grassTopLeft;
    maxGrassDir = MOVE_TOP_LEFT;
  }
  if (grassTopCentre > maxGrassHeight) {
    maxGrassHeight = grassTopCentre;
    maxGrassDir = MOVE_TOP_CENTRE;
  }
  if (grassTopRight > maxGrassHeight) {
    maxGrassHeight = grassTopRight;
    maxGrassDir = MOVE_TOP_RIGHT;
  }
  if (grassRight > maxGrassHeight) {
    maxGrassHeight = grassRight;
    maxGrassDir = MOVE_RIGHT;
  }
  if (grassBottomRight > maxGrassHeight) {
    maxGrassHeight = grassBottomRight;
    maxGrassDir = MOVE_BOTTOM_RIGHT;
  }
  if (grassBottomCentre > maxGrassHeight) {
    maxGrassHeight = grassBottomCentre;
    maxGrassDir = MOVE_BOTTOM_CENTRE;
  }
  if (grassBottomLeft > maxGrassHeight) {
    maxGrassHeight = grassBottomLeft;
    maxGrassDir = MOVE_BOTTOM_LEFT;
  }
  if (grassLeft > maxGrassHeight) {
    maxGrassHeight = grassLeft;
    maxGrassDir = MOVE_LEFT;
  }
  return maxGrassDir;
}

int returnToLastCut(int32_t i, int32_t j, int32_t lastcut_i, int32_t lastcut_j) {
  int32_t delta_i = lastcut_i - i;
  int32_t delta_j = lastcut_j - j;
  bool isPositiveDelta_i = delta_i > 0;
  bool isPositiveDelta_j = delta_j > 0;
  bool isZeroDelta_i = delta_i == 0;
  bool isZeroDelta_j = delta_j == 0;

  // If stuck at the wall from top
  if (lastcut_j > 16 && j == 16 && i < 25) {
    return MOVE_RIGHT;
  }

  if (isZeroDelta_i && !isZeroDelta_j) {
    return MOVE_BOTTOM_CENTRE;
  }
  if (!isZeroDelta_i && isZeroDelta_j) {
    // Split into left and rights for the lower hemisphere
    if (isPositiveDelta_i) {
      return MOVE_RIGHT;
    }
    else {
      return MOVE_LEFT;
    }
  }
  if (isPositiveDelta_i && isPositiveDelta_j) {
    // Random chance to deviate from the general direction
    if (std::rand() % 3 == 0) {
      return MOVE_BOTTOM_CENTRE;
    }
    else if (std::rand() % 3 == 1) {
      return MOVE_RIGHT;
    }
    else {
      return MOVE_BOTTOM_RIGHT; // Ideal general direction
    }
  }
  if (isPositiveDelta_i && !isPositiveDelta_j) {
    // Random chance to deviate from the general direction
    if (std::rand() % 3 == 0) {
      return MOVE_TOP_CENTRE;
    }
    else if (std::rand() % 3 == 1) {
      return MOVE_RIGHT;
    }
    else {
      return MOVE_TOP_RIGHT; // Ideal general direction
    }
  }
  if (!isPositiveDelta_i && isPositiveDelta_j) {
    // Random chance to deviate from the general direction
    if (std::rand() % 3 == 0) {
      return MOVE_BOTTOM_CENTRE;
    }
    else if (std::rand() % 3 == 1) {
      return MOVE_LEFT;
    }
    else {
      return MOVE_BOTTOM_LEFT; // Ideal general direction
    }
  }
  if (!isPositiveDelta_i && !isPositiveDelta_j) {
    // Random chance to deviate from the general direction
    if (std::rand() % 3 == 0) {
      return MOVE_TOP_CENTRE;
    }
    else if (std::rand() % 3 == 1) {
      return MOVE_LEFT;
    }
    else {
      return MOVE_TOP_LEFT; // Ideal general direction
    }
  }
  return MOVE_RIGHT;
}

int returnToCharge(int32_t i, int32_t j) {
  if (j == 0 && i != 0) {
    return MOVE_LEFT;
  }
  if (j != 0 && i == 0 && j < 17) {
    return MOVE_TOP_CENTRE;
  }
  // If stuck at bottom wall
  if (j == 18 && i < 23) {
    return MOVE_RIGHT;
  }
  if (j < 18) {
    return MOVE_TOP_LEFT;
  }
  if (j >= 18 && i > 24) {
    return MOVE_TOP_CENTRE;
  }
  return MOVE_TOP_RIGHT;
}

int32_t main(int32_t argc, char **argv) {
  int32_t retCode{0};

  State currentState = STATIONARY_CUT;
  int32_t lastcut_i {-1};
  int32_t lastcut_j {-1};
  int lastCommand {-1};

  auto commandlineArguments = cluon::getCommandlineArguments(argc, argv);
  if (0 == commandlineArguments.count("cid")) {
    std::cerr << argv[0] 
      << " is a lawn mower control algorithm." << std::endl;
    std::cerr << "Usage:   " << argv[0] << " --cid=<OpenDLV session>" 
      << "[--verbose]" << std::endl;
    std::cerr << "Example: " << argv[0] << " --cid=111 --verbose" << std::endl;
    retCode = 1;
  } else {
    bool const verbose{commandlineArguments.count("verbose") != 0};
    uint16_t const cid = std::stoi(commandlineArguments["cid"]);
    
    cluon::OD4Session od4{cid};

    auto onSensors{[&od4, &currentState, &lastcut_i, &lastcut_j, &lastCommand](cluon::data::Envelope &&envelope)
      {
        auto msg = cluon::extractMessage<tme290::grass::Sensors>(
            std::move(envelope));

        int currentCommand {-1};
        int32_t i = msg.i();
        int32_t j = msg.j();
        float grassTopLeft = msg.grassTopLeft();
        float grassTopCentre = msg.grassTopCentre();
        float grassTopRight = msg.grassTopRight();
        float grassRight = msg.grassRight();
        float grassBottomRight = msg.grassBottomRight();
        float grassBottomCentre = msg.grassBottomCentre();
        float grassBottomLeft = msg.grassBottomLeft();
        float grassLeft = msg.grassLeft();
        float grassCentre = msg.grassCentre();
        float rain = msg.rain();
        float battery = msg.battery();
        float rainCloudDirX = msg.rainCloudDirX();
        float rainCloudDirY = msg.rainCloudDirY();

        // Parameters to be tuned
        // If battery drains below this value the lawnmower will return to charge
        float batteryDrainThreshold;
        float batteryDrainThresholdQ1 = 0.2f;
        float batteryDrainThresholdQ2 = 0.3f;
        float batteryDrainThresholdQ3 = 0.4f;
        float batteryDrainThresholdQ4 = 0.5f;
        float transientCutGrassThreshold = 0.5f; // If current grass value is above this value the lawnmower will stay and cut
        float rainAvoidanceThreshold = 0.4f; // If rain level is below this value then lawnmower can switch to stationary mode

        // Determine which quadrant the battery threshold is
        if (0 <= i && i <= 23 && 0 <= j && j <= 17) {
          batteryDrainThreshold = batteryDrainThresholdQ1;
        }
        else if (i > 23 && j <= 17) {
          batteryDrainThreshold = batteryDrainThresholdQ2;
        }
        else if (j > 17 && i > 23) {
          batteryDrainThreshold = batteryDrainThresholdQ3;
        }
        else {
          batteryDrainThreshold = batteryDrainThresholdQ4;
        }

      
        std::cout << "STATE: " << currentState << std::endl;
        std::cout << "rain: " << rain << " rainDirX: " << rainCloudDirX << " rainDirY: " << rainCloudDirY << std::endl;
        std::cout << "lastcut_i: " << lastcut_i << " lastcut_j: " << lastcut_j << " bt: " << batteryDrainThreshold << std::endl;
        std::cout << grassTopLeft << " " << grassTopCentre << " " << grassTopRight << std::endl;
        std::cout << grassLeft << " " << grassCentre << " " << grassRight << std::endl;
        std::cout << grassBottomLeft << " " << grassBottomCentre << " " << grassBottomRight << std::endl << std::endl;

        tme290::grass::Control control;

        // Determine behaviour
        switch(currentState) {
          case TRANSIENT_CUT:
            currentCommand = transientCut(i, j, grassCentre, transientCutGrassThreshold);
            break;
          case STATIONARY_CUT:
            currentCommand = stationaryCut(grassTopLeft, grassTopCentre, grassTopRight, 
              grassRight, grassBottomRight, grassBottomCentre, grassBottomLeft, grassLeft);
            break;
          case STORE_LASTCUT:
            lastcut_i = i;
            lastcut_j = j;
            currentCommand = returnToCharge(i, j);
            break;
          case RETURN_TO_CHARGE:
            currentCommand = returnToCharge(i, j);
            break;
          case STAY_AND_CHARGE:
            currentCommand = MOVE_STAY;
            break;
          case RETURN_TO_LASTCUT:
            currentCommand = returnToLastCut(i, j, lastcut_i, lastcut_j);
            break;
          default:
            break;
        }

        // Update state
        currentState = updateState(currentState, battery, batteryDrainThreshold,
          i, j, lastcut_i, lastcut_j, grassCentre, transientCutGrassThreshold,
          rain, rainAvoidanceThreshold);

        lastCommand = currentCommand;
        control.command(currentCommand);
        od4.send(control);
      }};

    auto onStatus{[&verbose](cluon::data::Envelope &&envelope)
      {
        auto msg = cluon::extractMessage<tme290::grass::Status>(
            std::move(envelope));
        if (verbose) {
          std::cout << "Status at time " << msg.time() << ": " 
            << msg.grassMean() << "/" << msg.grassMax() << std::endl;
        }
      }};

    od4.dataTrigger(tme290::grass::Sensors::ID(), onSensors);
    od4.dataTrigger(tme290::grass::Status::ID(), onStatus);

    if (verbose) {
      std::cout << "All systems ready, let's cut some grass!" << std::endl;
    }

    tme290::grass::Control control;
    control.command(0);
    od4.send(control);

    while (od4.isRunning()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    retCode = 0;
  }
  return retCode;
}
