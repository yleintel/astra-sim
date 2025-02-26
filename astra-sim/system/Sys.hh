/******************************************************************************
This source code is licensed under the MIT license found in the
LICENSE file in the root directory of this source tree.
*******************************************************************************/

#ifndef __SYSTEM_HH__
#define __SYSTEM_HH__

#include <assert.h>
#include <math.h>
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <fstream>
#include <list>
#include <map>
#include <sstream>
#include <tuple>
#include <vector>
#include "AstraMemoryAPI.hh"
#include "AstraNetworkAPI.hh"
#include "Callable.hh"
#include "CollectivePhase.hh"
#include "Common.hh"
#include "UsageTracker.hh"
#include "astra-sim/system/topology/RingTopology.hh"
#include "astra-sim/workload/Workload.hh"

namespace AstraSim {
class MemBus;
class BaseStream;
class StreamBaseline;
class DataSet;
class SimSendCaller;
class SimRecvCaller;
class QueueLevels;
class Workload;
class LogicalTopology;
class BasicLogicalTopology;
class OfflineGreedy;
class Sys : public Callable {
 public:
  class SchedulerUnit {
   public:
    Sys* sys;
    int ready_list_threshold;
    int queue_threshold;
    int max_running_streams;
    std::map<int, int> running_streams;
    std::map<int, std::list<BaseStream*>::iterator> stream_pointer;

    std::vector<Tick> latency_per_dimension;
    std::vector<double> total_chunks_per_dimension;
    std::vector<uint64_t> total_active_chunks_per_dimension;
    std::map<int, int> queue_id_to_dimension;
    std::vector<UsageTracker> usage;

    SchedulerUnit(
        Sys* sys,
        std::vector<int> queues,
        int max_running_streams,
        int ready_list_threshold,
        int queue_threshold);
    void notify_stream_removed(int vnet, Tick running_time);
    void notify_stream_added(int vnet);
    void notify_stream_added_into_ready_list();
    std::vector<double> get_average_latency_per_dimension();
  };
  SchedulerUnit* scheduler_unit;
  ~Sys();
  AstraNetworkAPI* NI;
  AstraMemoryAPI* MEM;
  int finished_workloads;
  int id;

  std::vector<CollectiveImplementation*>
      all_reduce_implementation_per_dimension;
  std::vector<CollectiveImplementation*>
      reduce_scatter_implementation_per_dimension;
  std::vector<CollectiveImplementation*>
      all_gather_implementation_per_dimension;
  std::vector<CollectiveImplementation*>
      all_to_all_implementation_per_dimension;
  CollectiveOptimization collectiveOptimization;

  std::chrono::high_resolution_clock::time_point start_sim_time;
  std::chrono::high_resolution_clock::time_point end_sim_time;

  std::list<Callable*> registered_for_finished_stream_event;

  std::vector<int> physical_dims;
  std::vector<int> queues_per_dim;
  int max_running;
  int concurrent_streams;
  int active_first_phase;
  int dim_to_break;
  std::vector<int> logical_broken_dims;

  int priority_counter;
  bool boost_mode;
  bool rendezvous_enabled;
  bool initialized;

  int processing_latency;
  int communication_delay;

  int preferred_dataset_splits;
  double compute_scale;
  double comm_scale;
  double injection_scale;
  int local_reduction_delay;
  uint64_t pending_events;
  std::string method;

  // for test
  Workload* workload;
  MemBus* memBus;
  int all_queues;
  // for supporting LIFO
  std::list<BaseStream*> ready_list;
  SchedulingPolicy scheduling_policy;
  int first_phase_streams;
  int total_running_streams;
  std::map<int, std::list<BaseStream*>> active_Streams;
  std::map<int, std::list<int>> stream_priorities;

  QueueLevels* vLevels;
  std::map<std::string, LogicalTopology*> logical_topologies;
  std::map<Tick, std::list<std::tuple<Callable*, EventType, CallData*>>>
      event_queue;
  int total_nodes;
  static Tick offset;
  static std::vector<Sys*> all_generators;
  static uint8_t* dummy_data;
  // for reports
  uint64_t streams_injected;
  uint64_t streams_finished;
  int stream_counter;
  bool enabled;

  std::string inp_scheduling_policy;
  std::string inp_all_reduce_implementation;
  std::string inp_reduce_scatter_implementation;
  std::string inp_all_gather_implementation;
  std::string inp_all_to_all_implementation;
  std::string inp_collective_optimization;
  float inp_L;
  float inp_o;
  float inp_g;
  float inp_G;
  int inp_model_shared_bus;
  int active_chunks_per_dimension;
  bool model_shared_bus;
  int inp_boost_mode;
  IntraDimensionScheduling intra_dimension_scheduling;
  InterDimensionScheduling inter_dimension_scheduling;
  int round_robin_inter_dimension_scheduler;
  OfflineGreedy* offline_greedy;
  Tick last_scheduled_collective;

  void register_for_finished_stream(Callable* callable);
  void increase_finished_streams(int amount);
  void register_event(
      Callable* callable,
      EventType event,
      CallData* callData,
      Tick cycles);
  void insert_into_ready_list(BaseStream* stream);
  void ask_for_schedule(int max);
  void schedule(int num);

  void register_phases(
      BaseStream* stream,
      std::list<CollectivePhase> phases_to_go);
  void call(EventType type, CallData* data);
  void try_register_event(
      Callable* callable,
      EventType event,
      CallData* callData,
      Tick& cycles);
  void call_events();
  void workload_finished() {
    finished_workloads++;
  };
  static Tick boostedTick();
  static void exiting();
  int nextPowerOf2(int n);
  static void sys_panic(std::string msg);
  void exitSimLoop(std::string msg);
  bool seprate_log;

  Sys(AstraNetworkAPI* NI,
      AstraMemoryAPI* MEM,
      int id,
      int num_passes,
      std::vector<int> physical_dims,
      std::vector<int> queues_per_dim,
      std::string my_sys,
      std::string my_workload,
      double comm_scale,
      double compute_scale,
      double injection_scale,
      int total_stat_rows,
      int stat_row,
      std::string path,
      std::string run_name,
      bool seprate_log,
      bool rendezvous_enabled);

  void iterate();
  bool initialize_sys(std::string name);
  std::string trim(const std::string& str, const std::string& whitespace);
  bool parse_var(std::string var, std::string value);
  bool post_process_inputs();
  std::vector<CollectiveImplementation*>
  generate_collective_implementation_from_input(std::string input);
  int break_dimension(int model_parallel_npu_group);
  int front_end_sim_send(
      Tick delay,
      void* buffer,
      uint64_t count,
      int type,
      int dst,
      int tag,
      sim_request* request,
      void (*msg_handler)(void* fun_arg),
      void* fun_arg);
  int front_end_sim_recv(
      Tick delay,
      void* buffer,
      uint64_t count,
      int type,
      int src,
      int tag,
      sim_request* request,
      void (*msg_handler)(void* fun_arg),
      void* fun_arg);
  int sim_send(
      Tick delay,
      void* buffer,
      uint64_t count,
      int type,
      int dst,
      int tag,
      sim_request* request,
      void (*msg_handler)(void* fun_arg),
      void* fun_arg);
  int sim_recv(
      Tick delay,
      void* buffer,
      uint64_t count,
      int type,
      int src,
      int tag,
      sim_request* request,
      void (*msg_handler)(void* fun_arg),
      void* fun_arg);
  int rendezvous_sim_send(
      Tick delay,
      void* buffer,
      uint64_t count,
      int type,
      int dst,
      int tag,
      sim_request* request,
      void (*msg_handler)(void* fun_arg),
      void* fun_arg);
  int rendezvous_sim_recv(
      Tick delay,
      void* buffer,
      uint64_t count,
      int type,
      int src,
      int tag,
      sim_request* request,
      void (*msg_handler)(void* fun_arg),
      void* fun_arg);
  Tick mem_read(uint64_t bytes);
  Tick mem_write(uint64_t bytes);
  static int get_layer_numbers(std::string workload_input);
  std::vector<std::string> split_string(std::string str, std::string sep);
  DataSet* generate_all_reduce(
      uint64_t size,
      std::vector<bool> involved_dimensions,
      SchedulingPolicy pref_scheduling,
      int layer);
  DataSet* generate_all_to_all(
      uint64_t size,
      std::vector<bool> involved_dimensions,
      SchedulingPolicy pref_scheduling,
      int layer);
  DataSet* generate_all_gather(
      uint64_t size,
      std::vector<bool> involved_dimensions,
      SchedulingPolicy pref_scheduling,
      int layer);
  DataSet* generate_reduce_scatter(
      uint64_t size,
      std::vector<bool> involved_dimensions,
      SchedulingPolicy pref_scheduling,
      int layer);
  DataSet* generate_collective(
      uint64_t size,
      int layer_num,
      LogicalTopology* topology,
      std::vector<CollectiveImplementation*> implementation_per_dimension,
      std::vector<bool> dimensions_involved,
      ComType collective_type,
      SchedulingPolicy pref_scheduling);
  CollectivePhase generate_collective_phase(
      ComType collective_type,
      int layer_num,
      BasicLogicalTopology* topology,
      uint64_t data_size,
      int queue_id,
      RingTopology::Direction direction,
      InjectionPolicy injection_policy,
      CollectiveImplementation* collective_implementation,
      bool boost_mode);
  void insert_stream(std::list<BaseStream*>* queue, BaseStream* baseStream);
  void proceed_to_next_vnet_baseline(StreamBaseline* stream);
  int determine_chunk_size(uint64_t size, ComType type);
  int get_priority(SchedulingPolicy pref_scheduling);
  static void handleEvent(void* arg);
  timespec_t generate_time(Tick cycles);
};
} // namespace AstraSim
#endif
