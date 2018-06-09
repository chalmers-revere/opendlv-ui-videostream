/*
 * Copyright (C) 2018 Ola Benderius
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

#include <cstdint>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <thread>

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "cluon-complete.hpp"
#include "opendlv-standard-message-set.hpp"

int32_t main(int32_t argc, char **argv) {
  int32_t retCode{0};
  auto commandlineArguments = cluon::getCommandlineArguments(argc, argv);
  if ((0 == commandlineArguments.count("name")) || (0 == commandlineArguments.count("freq")) || (0 == commandlineArguments.count("cid")) || (0 == commandlineArguments.count("width")) || (0 == commandlineArguments.count("height")) || (0 == commandlineArguments.count("bpp"))) {
    std::cerr << argv[0] << " accesses video data using shared memory and sends it as a stream over an OD4 session." << std::endl;
    std::cerr << "         --freq: maximum frame rate of video stream" << std::endl;
    std::cerr << "         --name: name of the shared memory to use" << std::endl;
    std::cerr << "         --width: the width of the image inside the shared memory" << std::endl;
    std::cerr << "         --height: the height of the image inside the shared memory" << std::endl;
    std::cerr << "         --bpp: the bits per pixel of the image inside the shared memory" << std::endl;
    std::cerr << "         --scaled-width: the width of the image in the resulting video stream (default: keep same width)" << std::endl;
    std::cerr << "         --scaled-height: the height of the image in the resulting video stream (default: keep same height)" << std::endl;
    std::cerr << "         --verbose: when set, a thumbnail of the image contained in the shared memory is sent" << std::endl;
    std::cerr << "Example: " << argv[0] << " --cid=111 --freq=2 --name=cam0 --width=1280 --height=960 --bpp=24" << std::endl;
    retCode = 1;
  } else {
    bool const VERBOSE{commandlineArguments.count("verbose") != 0};
    uint32_t const FREQ{static_cast<uint32_t>(std::stoi(commandlineArguments["freq"]))};
    uint32_t const WIDTH{static_cast<uint32_t>(std::stoi(commandlineArguments["width"]))};
    uint32_t const HEIGHT{static_cast<uint32_t>(std::stoi(commandlineArguments["height"]))};
    uint32_t const BPP{static_cast<uint32_t>(std::stoi(commandlineArguments["bpp"]))};
    uint32_t const ID{(commandlineArguments["id"].size() != 0) ? static_cast<uint32_t>(std::stoi(commandlineArguments["id"])) : 0};

    uint32_t const SCALED_WIDTH{(commandlineArguments["scaled-width"].size() != 0) ? static_cast<uint32_t>(std::stoi(commandlineArguments["scaled-width"])) : WIDTH};
    uint32_t const SCALED_HEIGHT{(commandlineArguments["scaled-height"].size() != 0) ? static_cast<uint32_t>(std::stoi(commandlineArguments["scaled-height"])) : HEIGHT};

    std::string const NAME{(commandlineArguments["name"].size() != 0) ? commandlineArguments["name"] : "/cam0"};

    std::unique_ptr<cluon::SharedMemory> sharedMemory(new cluon::SharedMemory{NAME});
    if (sharedMemory && sharedMemory->valid()) {
      std::clog << argv[0] << ": Found shared memory '" << sharedMemory->name() << "' (" << sharedMemory->size() << " bytes)." << std::endl;

      CvSize size;
      size.width = WIDTH;
      size.height = HEIGHT;

      IplImage *image = cvCreateImageHeader(size, IPL_DEPTH_8U, BPP/8);
      sharedMemory->lock();
      image->imageData = sharedMemory->data();
      image->imageDataOrigin = image->imageData;
      sharedMemory->unlock();
    
      cluon::OD4Session od4{static_cast<uint16_t>(std::stoi(commandlineArguments["cid"]))};

      auto atFrequency{[&sharedMemory, &image, &WIDTH, &HEIGHT, &SCALED_WIDTH, &SCALED_HEIGHT, &ID, &VERBOSE, &od4]() -> bool
        {
          if (VERBOSE) {
            std::cout << "Waiting for image.." << std::endl;
          }
          sharedMemory->wait();

          cv::Mat scaledImage;
          {
            sharedMemory->lock();
            cv::Mat sourceImage = cv::cvarrToMat(image, false);
            cv::resize(sourceImage, scaledImage, cv::Size(SCALED_WIDTH, SCALED_HEIGHT), 0, 0, cv::INTER_NEAREST);
            sharedMemory->unlock();
          }

          std::vector<unsigned char> buf;
          cv::imencode(".jpg", scaledImage, buf);

          std::string data(buf.begin(), buf.end());

          opendlv::proxy::ImageReading imageReading;
          imageReading.format("jpeg");
          imageReading.width(SCALED_WIDTH);
          imageReading.height(SCALED_HEIGHT);
          imageReading.data(data);
          
          od4.send(imageReading, cluon::time::now(), ID);

          if (VERBOSE) {
            std::cout << "Sending image, scaled from " << WIDTH << "x" << HEIGHT << " to " << SCALED_WIDTH << "x" << SCALED_HEIGHT << "." << std::endl;
          }
          return true;
        }};

      od4.timeTrigger(FREQ, atFrequency);

      cvReleaseImageHeader(&image);
    } else {
      std::cerr << argv[0] << ": Failed to access shared memory '" << NAME << "'." << std::endl;
    }
  }
  return retCode;
}

