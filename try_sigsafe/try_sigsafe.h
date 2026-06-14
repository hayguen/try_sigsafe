#ifndef TRY_SIGSAFE_H
#define TRY_SIGSAFE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum SafeFaultHandler
{
  SFH_ABRT     = 0x01,
  SFH_SEGV     = 0x02,
  SFH_ILL      = 0x04,
  SFH_INT      = 0x08,  /* produced with Ctrl+C on Linux */
  SFH_TERM     = 0x10,
  SFH_DEFAULT  = SFH_ABRT | SFH_ILL | SFH_INT | SFH_TERM
} SafeFaultHandler;

typedef enum SafeFaultTraps
{
  SFT_FP_TRAPS  = 0x01,  /* setup FP traps */
  SFT_DIVBYZERO = 0x02,
  SFT_INEXACT   = 0x04,
  SFT_INVALID   = 0x08,
  SFT_OVERFLOW  = 0x10,
  SFT_UNDERFLOW = 0x20,
  SFT_DEFAULT   = SFT_FP_TRAPS | SFT_DIVBYZERO | SFT_INVALID | SFT_OVERFLOW
} SafeFaultTraps;


/* try execution of callback(userData) - return 0 when no errors occured */
int try_sigsafe( void (*callback)(void *userdata), void* userData, int fault_handlers_mask, int fp_traps_mask, int *catched_signal );
/* as try sigsafe, but allow explicit list of signals numbers - max 31 - to catch */
int try_sigsafe_ex( void (*callback)(void *userdata), void* userData, int fp_traps_mask, int *catched_signal, const int *handle_sig_numbers, int num_signos );

/* Windows MSVC needs help with SIGINT (Ctrl+C), see
* https://stackoverflow.com/questions/63415756/do-windows-apps-support-sigint-for-user-defined-signal-handlers
*/
int sigsafe_clear_interrupt();
void sigsafe_check_interrupt(int clear_interrupt);

void show_fp_status( void );

#ifdef __cplusplus
}
#endif

#endif
