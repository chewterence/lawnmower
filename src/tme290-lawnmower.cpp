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
// time_t time;
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

enum State { returnToCharge, stayAndCharge };

int32_t main(int32_t argc, char **argv) {
  int32_t retCode{0};
  // State currentState = ;
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

    int timestep = 0;

    auto onSensors{[&od4, &timestep](cluon::data::Envelope &&envelope)
      {
        auto msg = cluon::extractMessage<tme290::grass::Sensors>(
            std::move(envelope));

        i = msg.i();
        j = msg.j();
        // time = msg.time();
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

        std::cout << "battery: " << battery << " i: " << i << " j: " << j << " t: " << timestep << std::endl;
        std::cout << "rain: " << rain << " rainDirX: " << rainCloudDirX << " rainDirY: " << rainCloudDirY << std::endl;
        std::cout << grassTopLeft << " " << grassTopCentre << " " << grassTopRight << std::endl;
        std::cout << grassLeft << " " << grassCentre << " " << grassRight << std::endl;
        std::cout << grassBottomLeft << " " << grassBottomCentre << " " << grassBottomRight << std::endl << std::endl;

        tme290::grass::Control control;

        // Main control logic can go here
        // switch state
        // control = getStateBehaviour(para1, para2, para3)
        // state = getNextState()



        // switch(currentState) {
        //   case returnToCharge: 
        //   break;

        //   default:
        //   break;
        // }

        timestep++;

        if (timestep % 8 == 0 || timestep % 8 == 1) {
          if (timestep % 8 == 0) {
            control.command(4);
          }
          else {
            control.command(0);
          }
        }
        else if (timestep % 8 == 2 || timestep % 8 == 3) {
          if (timestep % 8 == 2) {
            control.command(6);
          }
          else {

          }
        }
        else if (timestep % 8 == 4 || timestep % 8 == 4) {
          if (timestep % 8 == 4 ) {
            control.command(8);
          }
          else {
            control.command(0);
          }
        }
        else if (timestep % 8 == 6 || timestep % 8 == 7) {
          if (timestep % 8 == 6) {
            control.command(2);
          }
          else {
            control.command(0);
          }
        }
        else {
          control.command(0);
        }

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
