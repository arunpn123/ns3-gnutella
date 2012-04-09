#ifndef QUERY_LATENCY_CONTAINER_H
#define QUERY_LATENCY_CONTAINER_H

#include "Descriptor.h"
#include "ns3/core-module.h"

// The purpose of this class is to help measure query response times.
class QueryLatencyContainer
{
  public:
    void AddQuery(DescriptorId d);
    ns3::Time FindTimeGivenQuery(DescriptorId d);
    ns3::Time RemoveQuery(DescriptorId d);
    // Remove the DescriptorID from the map
    // Update the total response time and number of responses
    void Update(DescriptorId d);
    uint32_t GetAverageResponseTime();
  private:
    // Mapping of Query DescriptorIDs to Time sent
    std::map<DescriptorId, ns3::Time> query_map_;
    // Total response time in nanoseconds
    uint64_t ns_sum_;
    // Number of responses measured
    uint32_t num_; 
};

#endif 
