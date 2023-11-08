// SPDX-License-Identifier: Apache-2.0
// Copyright 2014 Network Geographics

/** @file

    Example tool to parse /proc/diskstats into JSON.
    Developed in response to a specific request.

    Input is information about the local disks.
    Output is the same information in JSON format.
*/

#include <iostream>
#include <fstream>
#include "swoc/TextView.h"
#include "swoc/swoc_file.h"
#include "swoc/bwf_std.h"

using namespace std::literals;
using namespace swoc::literals;

using swoc::TextView;
using swoc::svtou;
swoc::LocalBufferWriter<1024> W;

struct DiskInfo {
  unsigned id;
  unsigned idx;
  TextView name;
  std::vector<unsigned> data;
};

int
main(int, char *[]) {
  std::vector<DiskInfo> info;
  char buffer[1024];

  // Unfortunately because this isn't a regular file, @c swoc::file doesn't work.
  // The input must be streamed.
  FILE *f = fopen("/proc/diskstats", "r");

  while (fgets(buffer, sizeof(buffer), f)) {
    DiskInfo item;
    TextView txt{buffer, strlen(buffer)};
    item.id   = svtou(txt.ltrim(' ').take_prefix_at(' '));
    item.idx  = svtou(txt.ltrim(' ').take_prefix_at(' '));
    item.name = txt.ltrim(' ').take_prefix_at(' ');
    while (txt.ltrim(' ')) {
      item.data.push_back(svtou(txt.take_prefix_at(' ')));
    }
    info.emplace_back(item);
  }

  std::cout << '[';
  bool comma = false;
  for (auto const &item : info) {
    if (comma) {
      std::cout << ",";
    }
    std::cout << std::endl
              << "  "
              << "{" << std::endl;
    std::cout << R"(  "id": )" << item.id << ',' << std::endl;
    std::cout << R"(  "index": )" << item.idx << ',' << std::endl;
    std::cout << W.clear().print(R"(  "name": "{}",)", item.name).write('\n');
    bool first = true;
    if (!item.data.empty()) {
      std::cout << R"(  "values": [)";
      for (auto n : item.data) {
        if (!first) {
          std::cout << ",";
        }
        std::cout << std::endl << "    " << n;
        first = false;
      }
      std::cout << std::endl << "    ]" << std::endl;
    }
    std::cout << "  "
              << "}";
    comma = true;
  }
  std::cout << std::endl << ']' << std::endl;

  return 0;
}
