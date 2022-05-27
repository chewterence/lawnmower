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

// Variables to receive
int32_t i;
int32_t j;
float grassTopLeft;
float grassTopCentre;
float grassTopRight;
float grassLeft;
float grassCentre;
float grassRight;
float grassBottomLeft;
float grassBottomCentre;
float grassBottomRight;
float rain;
float battery;
float rainCloudDirX;
float rainCloudDirY;

enum State { RETURN_TO_CHARGE, STAY_AND_CHARGE, STOP_AND_CUT, SEEK_FOR_GRASS };

// int seekForGrass(float grassTopLeft, float grassTopCentre, float grassTopRight, float grassRight, float grassBottomRight, float grassBottomCentre, float grassBottomLeft, float grassLeft, float grassCentre) {
int seekForGrass() {
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
    return 0;
  }

  float maxGrassHeight = 0.0;
  int maxGrassDir = 0;

  // Find the maximum grass
  if (grassTopLeft > maxGrassHeight) {
    maxGrassHeight = grassTopLeft;
    maxGrassDir = 1;
  }
  if (grassTopCentre > maxGrassHeight) {
    maxGrassHeight = grassTopCentre;
    maxGrassDir = 2;
  }
  if (grassTopRight > maxGrassHeight) {
    maxGrassHeight = grassTopRight;
    maxGrassDir = 3;
  }
  if (grassRight > maxGrassHeight) {
    maxGrassHeight = grassRight;
    maxGrassDir = 4;
  }
  if (grassBottomRight > maxGrassHeight) {
    maxGrassHeight = grassBottomRight;
    maxGrassDir = 5;
  }
  if (grassBottomCentre > maxGrassHeight) {
    maxGrassHeight = grassBottomCentre;
    maxGrassDir = 6;
  }
  if (grassBottomLeft > maxGrassHeight) {
    maxGrassHeight = grassBottomLeft;
    maxGrassDir = 7;
  }
  if (grassLeft > maxGrassHeight) {
    maxGrassHeight = grassLeft;
    maxGrassDir = 8;
  }
  return maxGrassDir;



}

int32_t main(int32_t argc, char **argv) {
  int32_t retCode{0};
  State currentState = SEEK_FOR_GRASS;
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

    auto onSensors{[&od4, &currentState](cluon::data::Envelope &&envelope)
      {
        auto msg = cluon::extractMessage<tme290::grass::Sensors>(
            std::move(envelope));

        int currentCommand {-1};
        i = msg.i();
        j = msg.j();
        grassTopLeft = msg.grassTopLeft();
        grassTopCentre = msg.grassTopCentre();
        grassTopRight = msg.grassTopRight();
        grassRight = msg.grassRight();
        grassBottomRight = msg.grassBottomRight();
        grassBottomCentre = msg.grassBottomCentre();
        grassBottomLeft = msg.grassBottomLeft();
        grassLeft = msg.grassLeft();
        grassCentre = msg.grassCentre();
        rain = msg.rain();
        battery = msg.battery();
        rainCloudDirX = msg.rainCloudDirX();
        rainCloudDirY = msg.rainCloudDirY();

        std::cout << "battery: " << battery << " i: " << i << " j: " << j << std::endl;
        std::cout << "rain: " << rain << " rainDirX: " << rainCloudDirX << " rainDirY: " << rainCloudDirY << std::endl;
        std::cout << grassTopLeft << " " << grassTopCentre << " " << grassTopRight << std::endl;
        std::cout << grassLeft << " " << grassCentre << " " << grassRight << std::endl;
        std::cout << grassBottomLeft << " " << grassBottomCentre << " " << grassBottomRight << std::endl << std::endl;

        tme290::grass::Control control;

        // Main control logic can go here
        // switch state
        // control = getStateBehaviour(para1, para2, para3)
        // state = getNextState()



        switch(currentState) {
          case SEEK_FOR_GRASS:
            // currentCommand = seekForGrass(grassTopLeft, grassTopCentre, grassTopRight, grassRight, grassBottomRight, grassBottomCentre, grassBottomLeft, grassLeft, grassCentre);
            currentCommand = seekForGrass();
            break;
          default:
          break;
        }

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
