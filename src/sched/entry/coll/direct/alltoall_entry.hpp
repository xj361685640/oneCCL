/*
 Copyright 2016-2020 Intel Corporation
 
 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at
 
     http://www.apache.org/licenses/LICENSE-2.0
 
 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
*/
#pragma once

#include "sched/entry/coll/direct/base_coll_entry.hpp"

class alltoall_entry : public base_coll_entry
{
public:
    static constexpr const char* class_name() noexcept
    {
        return "ALLTOALL";
    }

    alltoall_entry() = delete;
    alltoall_entry(ccl_sched* sched,
                   const ccl_buffer send_buf,
                   ccl_buffer recv_buf,
                   size_t cnt,
                   ccl_datatype_internal_t dtype,
                   ccl_comm* comm) :
        base_coll_entry(sched), send_buf(send_buf),
        recv_buf(recv_buf), cnt(cnt), dtype(dtype),
        comm(comm)
    {
    }

    void start() override
    {
        size_t dt_size = ccl_datatype_get_size(dtype);
        bytes = cnt * dt_size;

        LOG_DEBUG("ALLTOALL entry req ", &req, ", bytes ", bytes);
        atl_status_t atl_status = atl_comm_alltoall(sched->bin->get_comm_ctx(),
                                                    send_buf.get_ptr(bytes),
                                                    recv_buf.get_ptr(bytes),
                                                    bytes,
                                                    &req);

        if (unlikely(atl_status != atl_status_success))
        {
            CCL_THROW("ALLTOALL entry failed. atl_status: ", atl_status_to_str(atl_status));
        }
        else
            status = ccl_sched_entry_status_started;
    }

    void update() override
    {
        int req_status;
        atl_status_t atl_status = atl_comm_check(sched->bin->get_comm_ctx(), &req_status, &req);

        if (unlikely(atl_status != atl_status_success))
        {
            CCL_THROW("ALLTOALL entry failed. atl_status: ", atl_status_to_str(atl_status));
        }

        if (req_status)
        {
            status = ccl_sched_entry_status_complete;
        }
    }

    const char* name() const override
    {
        return class_name();
    }

protected:
    void dump_detail(std::stringstream& str) const override
    {
        ccl_logger::format(str,
                           "dt ", ccl_datatype_get_name(dtype),
                           ", send_buf ", send_buf,
                           ", recv_buf ", recv_buf,
                           ", cnt ", cnt,
                           ", recv_bytes ", bytes,
                           ", comm_id ", sched->get_comm_id(),
                           ", req ",&req,
                           "\n");
    }

private:
    ccl_buffer send_buf;
    ccl_buffer recv_buf;
    size_t cnt;
    int bytes;
    ccl_datatype_internal_t dtype;
    ccl_comm* comm;
    atl_req_t req{};
};
