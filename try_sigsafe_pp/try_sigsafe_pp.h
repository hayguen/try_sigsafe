#ifndef TRY_SIGSAFE_PP_H
#define TRY_SIGSAFE_PP_H

#include <setjmp.h>
#include <fenv.h>
#include <errno.h>
#include <utility>

class TrySigsafe
{
public:

  enum FaultHandler
  {
    SFH_ABRT     = 0x01,
    SFH_SEGV     = 0x02,
    SFH_ILL      = 0x04,
    SFH_INT      = 0x08,  /* produced with Ctrl+C on Linux */
    SFH_TERM     = 0x10,
    SFH_DEFAULT  = SFH_ABRT | SFH_ILL | SFH_INT | SFH_TERM
  };

  enum FaultTraps
  {
    SFT_FP_TRAPS  = 0x01,  /* setup FP traps */
    SFT_DIVBYZERO = 0x02,
    SFT_INEXACT   = 0x04,
    SFT_INVALID   = 0x08,
    SFT_OVERFLOW  = 0x10,
    SFT_UNDERFLOW = 0x20,
    SFT_DEFAULT   = SFT_FP_TRAPS | SFT_DIVBYZERO | SFT_INVALID | SFT_OVERFLOW
  };

  TrySigsafe( int fault_handlers_mask, int fp_traps_mask );
  TrySigsafe( const int * handle_sig_numbers, int num_signos, int fp_traps_mask );
  ~TrySigsafe();

  // check/test callback(userdata) - return 0 when no errors occured
  int testrun_callback( void (*callback)(void *userdata), void* userData );

  // check/test callback(userdata) - return 0 when no errors occured
  template <typename Func, typename... Args>
  int testrun(Func func, Args&&... args)
  {
    prepare();

    handler_info.valid_jumpBuffer = 1;
#if defined(_MSC_VER)
    restore_status = setjmp(jumpBuffer);
#else
    restore_status = sigsetjmp(jumpBuffer, 1);
#endif
    if (!restore_status)
    {
      errno = 0;
      // std::forward preserves the value category (lvalue/rvalue) of each arg
      // return is thrown away!
      func(std::forward<Args>(args)...);

#ifdef _MSC_VER
      // check once at end
      sigsafe_check_interrupt(0);
#endif
    }
    handler_info.valid_jumpBuffer = 0;

    return restore_status;
  }

  int last_catched_signal() const
  {
    return handler_info.catched_signal;
  }


  /* Windows MSVC needs help with SIGINT (Ctrl+C), see
  * https://stackoverflow.com/questions/63415756/do-windows-apps-support-sigint-for-user-defined-signal-handlers
  */
  static int sigsafe_clear_interrupt();
  static void sigsafe_check_interrupt(int clear_interrupt);

  static void show_fp_status();

public:
  static const int MAX_SIG_HANDLERS = 31;
  typedef void (*SignalHandlerPointer)(int);

  struct handler_info_t
  {
#if defined(_MSC_VER)
    jmp_buf    *jumpBuffer;
#else
    sigjmp_buf *jumpBuffer;
#endif
      void* faultAddr;
      int catched_signal;
      int catched_fp_traps;
      int call_prev_handler;
      int num_prev;
      int valid_jumpBuffer;
      const int *sig_numbers;
      SignalHandlerPointer old_handlers[MAX_SIG_HANDLERS + 1];
  };

private:
  void init( const int * handle_sig_numbers, int num_signos );
  void prepare();

  const int _fp_traps_mask;

#if defined(_MSC_VER)
  jmp_buf    jumpBuffer;
  fenv_t     save_fe_env;
  unsigned   prev_fe_traps;
#else
  sigjmp_buf jumpBuffer;
  fenv_t     save_fe_env;
  int        prev_fe_traps;
#endif
  handler_info_t handler_info;
  handler_info_t* prev_handler_info;

  int fe_get_ret;
  int restore_status;
  int have_fpe_in_list;

  int _sig_numbers[MAX_SIG_HANDLERS + 1];
  int _n_signos;
};

#endif
