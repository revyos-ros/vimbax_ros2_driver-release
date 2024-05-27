// Copyright (c) 2024 Allied Vision Technologies GmbH. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//    * Redistributions of source code must retain the above copyright
//      notice, this list of conditions and the following disclaimer.
//
//    * Redistributions in binary form must reproduce the above copyright
//      notice, this list of conditions and the following disclaimer in the
//      documentation and/or other materials provided with the distribution.
//
//    * Neither the name of the Allied Vision Technologies GmbH nor the names of its
//      contributors may be used to endorse or promote products derived from
//      this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include <iostream>
#include <queue>

#include <rclcpp/rclcpp.hpp>

#include <image_transport/image_transport.hpp>

#include "example_helper.hpp"


int main(int argc, char * argv[])
{
  auto const args = rclcpp::init_and_remove_ros_arguments(argc, argv);

  if (args.size() < 2) {
    std::cout << "Usage asynchronous_grab_cpp <vimbax camera node namespace>" << std::endl;
    return 1;
  }

  auto node = rclcpp::Node::make_shared("_asynchronous_grab_cpp");

  int64_t frames_missing = 0;
  int64_t frame_complete = 0;
  std::chrono::nanoseconds last_timestamp{0};

  auto const topic_name = build_topic_path(args[1], "/image_raw");

  auto subscription_options = rclcpp::SubscriptionOptions();
  subscription_options.event_callbacks.message_lost_callback =
    [&frames_missing](rclcpp::QOSMessageLostInfo & info) {
      std::cout << info.total_count_change << " missing frames detected!!!" << std::endl;
      frames_missing += info.total_count_change;
    };

  auto subscription = image_transport::create_subscription(
    node.get(), topic_name,
    [&](sensor_msgs::msg::Image::ConstSharedPtr imgmsg) {
      auto const timestamp = std::chrono::nanoseconds{imgmsg->header.stamp.nanosec} +
      std::chrono::seconds{imgmsg->header.stamp.sec};

      auto const diff =
      std::chrono::duration_cast<std::chrono::microseconds>(timestamp - last_timestamp).count();

      auto const mfps = 1000000000 / diff;

      std::cout << "Got frame diff " << diff << " mfps: " << mfps << std::endl;

      last_timestamp = timestamp;
      frame_complete++;
    }, "raw", rmw_qos_profile_default, subscription_options);

  rclcpp::spin(node);

  std::cout << "Missed " << frames_missing << "/" << frame_complete + frames_missing << " frames"
            << std::endl;
}
