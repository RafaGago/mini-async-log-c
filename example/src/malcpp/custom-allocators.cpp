//------------------------------------------------------------------------------
// Minimal hello world.
//------------------------------------------------------------------------------
#include <cstdlib>
#include <cstddef>
#include <thread>
#include <atomic>
#include <iostream>

#include <malcpp/malcpp_lean.hpp>
//------------------------------------------------------------------------------
class myalloc {
public:
  //----------------------------------------------------------------------------
  /* just a standard allocation wrapper with toy stats. For demo purposes.
  Notice that for being able to use "offsetof" the class has to fullfil the
  "standard layout" requirements */
  //----------------------------------------------------------------------------
  bl_alloc_tbl const* get_table() const
  {
    return &m_tbl;
  }
  //----------------------------------------------------------------------------
  bool no_leaks() const
  {
    auto allocs = m_allocations.load() - m_alloc_failures.load();
    return allocs == m_deallocations.load();
  }
  //----------------------------------------------------------------------------
  bool expected_alloc_match (unsigned v) const
  {
    return m_deallocations.load() == v;
  }
  //----------------------------------------------------------------------------
  void print_stats(char const* name) const
  {
    std::cout
      << name
      << "\n  at:                    " << reinterpret_cast<void const*> (this)
      << "\n  allocations:           " << m_allocations.load()
      << "\n  reallocations:         " << m_reallocations.load()
      << "\n  deallocations:         " << m_deallocations.load()
      << "\n  alloc_failures:        " << m_alloc_failures.load()
      << "\n  realloc_failures:      " << m_realloc_failures.load()
      << "\n  avg 1st alloc (bytes): "
      << m_alloc_bytes.load() / m_allocations.load()
      << "\n";
  }
  //----------------------------------------------------------------------------
private:
  //----------------------------------------------------------------------------
  static class myalloc* get_instance (bl_alloc_tbl const* tbl_addr)
  {
    return (myalloc*) (((char*) (tbl_addr)) - offsetof (myalloc, m_tbl));
  }
  //----------------------------------------------------------------------------
  static void* alloc (std::size_t bytes, bl_alloc_tbl const* tbl)
  {
    /*  allocation */
    void* v = std::malloc (bytes);
    /*  stats */
    auto this_ = get_instance (tbl);
    this_->m_allocations.fetch_add (1, std::memory_order_relaxed);
    if (v != nullptr) {
      this_->m_alloc_bytes.fetch_add (bytes, std::memory_order_relaxed);
    }
    else {
      this_->m_alloc_failures.fetch_add (1, std::memory_order_relaxed);
    }
    return v;
  }
  //----------------------------------------------------------------------------
  static void* realloc (void* mem, std::size_t bytes, bl_alloc_tbl const* tbl)
  {
    /*  allocation */
    void* v = std::realloc (mem, bytes);
    /*  stats */
    auto this_ = get_instance (tbl);
    if (mem != nullptr) {
      this_->m_reallocations.fetch_add (1, std::memory_order_relaxed);
      if (v == nullptr) {
        this_->m_realloc_failures.fetch_add (1, std::memory_order_relaxed);
      }
    }
    else {
      this_->m_allocations.fetch_add (1, std::memory_order_relaxed);
      if (v != nullptr) {
        this_->m_alloc_bytes.fetch_add (bytes, std::memory_order_relaxed);
      }
      else {
        this_->m_alloc_failures.fetch_add (1, std::memory_order_relaxed);
      }
    }
    return v;
  }
  //----------------------------------------------------------------------------
  static void dealloc (void const* mem, bl_alloc_tbl const* tbl)
  {
    /*  allocation */
    std::free ((void*) mem);
    /*  stats */
    auto this_ = get_instance (tbl);
    if (mem != nullptr) {
      this_->m_deallocations.fetch_add (1, std::memory_order_relaxed);
    }
  }
  //----------------------------------------------------------------------------
  char padding1[128];
  std::atomic<std::size_t> m_allocations {0};
  char padding2[128];
  std::atomic<std::size_t> m_alloc_bytes {0};
  char padding3[128];
  std::atomic<std::size_t> m_alloc_failures {0};
  char padding4[128];
  std::atomic<std::size_t> m_reallocations {0};
  char padding5[128];
  std::atomic<std::size_t> m_realloc_failures {0};
  char padding6[128];
  std::atomic<std::size_t> m_deallocations {0};
  char padding7[128];
  bl_alloc_tbl             m_tbl {
    &myalloc::alloc, &myalloc::realloc, &myalloc::dealloc
  };
};
//------------------------------------------------------------------------------
int main (int argc, char const* argv[])
{
  myalloc malc_alloc;
  myalloc log_msg_alloc;
  {
    malcpp::malcpp<> logger (malc_alloc.get_table());
    /* destination register: adding stdout only */
    logger.add_destination<malcpp::stdouterr_dst>();

    auto cfg = logger.get_cfg();
    /* "msg_allocator" is used exclusively for message allocation when no
    other memory sources are configured. On this example TLS isn't used and the
    fixed allocator is disabled */
    cfg.alloc.msg_allocator = log_msg_alloc.get_table();
    cfg.alloc.fixed_allocator_bytes = 0; /* 0 was the default, just showing*/
    cfg.consumer.start_own_thread = false;
    logger.init (cfg);

    std::thread thr;
    thr = std::thread ([&logger](){
      log_error_i (logger, "message 1");
      log_error_i (logger, "message 2");
    });
    thr.join();
    logger.run_consume_task (20000);
    /* "logger" is destroyed here */
  }
  malc_alloc.print_stats ("malc_alloc");
  log_msg_alloc.print_stats ("log_msg_alloc");

  if (!malc_alloc.no_leaks() || !log_msg_alloc.no_leaks()) {
    return 1;
  }
  if (!log_msg_alloc.expected_alloc_match (2)) {
    return 2;
  }
  return 0;
}
//------------------------------------------------------------------------------
