/*
 * Copyright (C) 2020 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "DumpstateServer.grpc.pb.h"
#include "DumpstateServer.h"
#include "DumpstateServer.pb.h"

class DumpstateGrpcServer : public dumpstate_proto::DumpstateServer::Service,
                            private DumpstateServer {
  public:
    DumpstateGrpcServer(const std::string& addr, const ServiceSupplier& services);

    grpc::Status GetSystemLogs(
            ::grpc::ServerContext* context, const ::google::protobuf::Empty* request,
            ::grpc::ServerWriter<dumpstate_proto::DumpstateBuffer>* stream) override;

    grpc::Status GetAvailableServices(::grpc::ServerContext* context,
                                      const ::google::protobuf::Empty* request,
                                      dumpstate_proto::ServiceNameList* serviceList) override;

    grpc::Status GetServiceLogs(
            ::grpc::ServerContext* context, const dumpstate_proto::ServiceLogRequest* request,
            ::grpc::ServerWriter<dumpstate_proto::DumpstateBuffer>* stream) override;

    void Start();

  private:
    std::string mServiceAddr;
};
