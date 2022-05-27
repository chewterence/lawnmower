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
enum State { RETURN_TO_CHARGE, STAY_AND_CHARGE, RETURN_TO_LASTCUT, SEEK_FOR_GRASS, STORE_LASTCUT, ERROR };
static const int MOVE_STAY = 0;
static const int MOVE_TOP_LEFT = 1;
static const int MOVE_TOP_CENTRE = 2;
static const int MOVE_TOP_RIGHT = 3;
static const int MOVE_RIGHT = 4;
static const int MOVE_BOTTOM_RIGHT = 5;
static const int MOVE_BOTTOM_CENTRE = 6;
static const int MOVE_BOTTOM_LEFT = 7;
static const int MOVE_LEFT = 8;

State updateState(State currentState, float battery, int32_t i, int32_t j, int32_t lastcut_i, int32_t lastcut_j) {
  switch (currentState) {
    case STORE_LASTCUT:
      return RETURN_TO_CHARGE;
    case RETURN_TO_CHARGE:
      if (i == 0 && j == 0) {
        return STAY_AND_CHARGE;
      }
      else {
        return RETURN_TO_CHARGE;
      }
    case SEEK_FOR_GRASS:
      if (battery < 0.4) {
        return STORE_LASTCUT;
      }
      else {
        return SEEK_FOR_GRASS;
      }
    case STAY_AND_CHARGE:
      if (battery < 0.98) {
        return STAY_AND_CHARGE;
      }
      else {
        return RETURN_TO_LASTCUT;
      }
    case RETURN_TO_LASTCUT:
      // If already at the last cut point, then change state
      if (i == lastcut_i && j == lastcut_j) {
        return SEEK_FOR_GRASS;
      }
      else {
        return RETURN_TO_LASTCUT;
      }
    default:
      return ERROR;
  }
}

int seekForGrass(float grassTopLeft, float grassTopCentre, float grassTopRight, float grassRight, float grassBottomRight, float grassBottomCentre, float grassBottomLeft, float grassLeft) {
  bool isTopLeftInvalid = grassTopLeft - -1 < 0.01f;
  bool isTopCentreInvalid = grassTopCentre - -1 < 0.01f;
  bool isTopRightInvalid = grassTopRight - -1 < 0.01f;
  bool isRightInvalid = grassRight - -1 < 0.01f;
  bool isBottomRightInvalid = grassBottomRight - -1 < 0.01f;
  bool isBottomCentreInvalid = grassBottomCentre - -1 < 0.01f;
  bool isBottomLeftInvalid = grassBottomLeft - -1 < 0.01f;
  bool isLeftInvalid = grassLeft - -1 < 0.01f;

  // If everywhere is invalid, stay on the spot to get a sensing first
  if (isTopLeftInvalid && isTopCentreInvalid && isTopRightInvalid && isRightInvalid && isBottomRightInvalid && isBottomCentreInvalid && isBottomLeftInvalid && isLeftInvalid) {
    return MOVE_STAY;
  }

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
  if (isZeroDelta_i && !isZeroDelta_j) {
    return MOVE_BOTTOM_CENTRE;
  }
  if (!isZeroDelta_i && isZeroDelta_j) {
    //TODO: split into left and rights for the lower hemisphere
    return MOVE_RIGHT;
  }
  if (isPositiveDelta_i && isPositiveDelta_j) {
    return MOVE_BOTTOM_RIGHT;
  }
  if (isPositiveDelta_i && !isPositiveDelta_j) {
    return MOVE_TOP_RIGHT;
  }
  if (!isPositiveDelta_i && isPositiveDelta_j) {
    return MOVE_BOTTOM_LEFT;
  }
  if (!isPositiveDelta_i && !isPositiveDelta_j) {
    return MOVE_TOP_LEFT;
  }
  return MOVE_RIGHT;
}

int returnToCharge(int32_t i, int32_t j) {
  if (j == 0) {
    return MOVE_LEFT;
  }
  if (j < 18) {
    return MOVE_TOP_LEFT;
  }
  i = i;
  return MOVE_TOP_RIGHT;
}

int32_t main(int32_t argc, char **argv) {
  int32_t retCode{0};

  State currentState = SEEK_FOR_GRASS;
  int32_t lastcut_i {-1};
  int32_t lastcut_j {-1};

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

    auto onSensors{[&od4, &currentState, &lastcut_i, &lastcut_j](cluon::data::Envelope &&envelope)
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

        // float grassHeightCutThreshold = 0.3;

        std::cout << "STATE: " << currentState << std::endl;
        std::cout << "rain: " << rain << " rainDirX: " << rainCloudDirX << " rainDirY: " << rainCloudDirY << std::endl;
        std::cout << "lastcut_i: " << lastcut_i << " lastcut_j: " << lastcut_j << std::endl;
        std::cout << grassTopLeft << " " << grassTopCentre << " " << grassTopRight << std::endl;
        std::cout << grassLeft << " " << grassCentre << " " << grassRight << std::endl;
        std::cout << grassBottomLeft << " " << grassBottomCentre << " " << grassBottomRight << std::endl << std::endl;

        tme290::grass::Control control;

        // Main control logic can go here
        // switch state
        // control = getStateBehaviour(para1, para2, para3)
        // state = getNextState()


        // Determine behaviour
        switch(currentState) {
          case SEEK_FOR_GRASS:
            lastcut_i = -1;
            lastcut_j = -1;
            currentCommand = seekForGrass(grassTopLeft, grassTopCentre, grassTopRight, grassRight, grassBottomRight, grassBottomCentre, grassBottomLeft, grassLeft);
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
        currentState = updateState(currentState, battery, i, j, lastcut_i, lastcut_j);

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
