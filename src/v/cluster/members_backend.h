#pragma once

#include "cluster/fwd.h"
#include "cluster/members_manager.h"
#include "cluster/members_table.h"
#include "cluster/scheduling/partition_allocator.h"
#include "cluster/types.h"
#include "model/fundamental.h"
#include "model/metadata.h"
#include "raft/consensus.h"

#include <seastar/core/condition-variable.hh>

#include <absl/container/node_hash_set.h>

#include <chrono>
namespace cluster {

class members_backend {
public:
    enum class reallocation_state { initial, reassigned, requested, finished };
    struct partition_reallocation {
        explicit partition_reallocation(model::ntp ntp)
          : ntp(std::move(ntp)) {}

        model::ntp ntp;
        std::optional<allocation_units> new_assignment;
        reallocation_state state = reallocation_state::initial;
    };
    /**
     * struct describing partition reallocation
     */
    struct update_meta {
        explicit update_meta(members_manager::node_update update)
          : update(update) {}

        members_manager::node_update update;
        std::vector<partition_reallocation> partition_reallocations;
        bool finished = false;
    };

    members_backend(
      ss::sharded<cluster::topics_frontend>&,
      ss::sharded<cluster::topic_table>&,
      ss::sharded<partition_allocator>&,
      ss::sharded<members_table>&,
      ss::sharded<controller_api>&,
      ss::sharded<members_manager>&,
      consensus_ptr,
      ss::sharded<ss::abort_source>&);

    void start();
    ss::future<> stop();

private:
    void start_reconciliation_loop();
    ss::future<> reconcile();
    ss::future<> reallocate_replica_set(partition_reallocation&);
    void try_to_finish_update(update_meta&);
    void calculate_reallocations(update_meta&);

    ss::future<> handle_updates();
    ss::future<> handle_single_update(members_manager::node_update);
    void handle_recommissioned(const members_manager::node_update&);

    ss::sharded<topics_frontend>& _topics_frontend;
    ss::sharded<topic_table>& _topics;
    ss::sharded<partition_allocator>& _allocator;
    ss::sharded<members_table>& _members;
    ss::sharded<controller_api>& _api;
    ss::sharded<members_manager>& _members_manager;
    consensus_ptr _raft0;
    ss::sharded<ss::abort_source>& _as;
    ss::gate _bg;
    mutex _lock;

    // replicas reallocations in progress
    std::vector<update_meta> _updates;
    std::chrono::milliseconds _retry_timeout;
    ss::timer<> _retry_timer;
    ss::condition_variable _new_updates;
};

} // namespace cluster
