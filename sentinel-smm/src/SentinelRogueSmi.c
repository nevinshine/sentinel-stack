#include <Library/DebugLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <PiDxe.h>
#include <Protocol/SmmSwDispatch2.h>

// Trigger value for the software SMI (port 0xB2)
#define ROGUE_SMI_SW_VALUE 0xCC

EFI_STATUS
EFIAPI
RogueSmiHandler(IN EFI_HANDLE DispatchHandle, IN CONST VOID *Context OPTIONAL,
                IN OUT VOID *CommBuffer OPTIONAL,
                IN OUT UINTN *CommBufferSize OPTIONAL) {
  DEBUG((DEBUG_INFO,
         "[RogueSmi] Triggered! Attempting illegal WRMSR to IA32_EFER...\n"));

  // Attempt a forbidden WRMSR from CPL3 (if Supervisor enforces the ring
  // boundary). IA32_EFER is MSR 0xC0000080.
  __asm__ __volatile__("mov $0xC0000080, %%rcx \n"
                       "mov $0x0, %%rax        \n"
                       "mov $0x0, %%rdx        \n"
                       "wrmsr                  \n"
                       :
                       :
                       : "rcx", "rax", "rdx");

  DEBUG((DEBUG_INFO,
         "[RogueSmi] FATAL: WRMSR succeeded. Supervisor trap failed!\n"));

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
RogueSmiEntry(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable) {
  EFI_STATUS Status;
  EFI_SMM_SW_DISPATCH2_PROTOCOL *SwDispatch;
  EFI_SMM_SW_REGISTER_CONTEXT SwContext;
  EFI_HANDLE DispatchHandle;

  // Locate the Software SMI Dispatch Protocol
  Status = gSmst->SmmLocateProtocol(&gEfiSmmSwDispatch2ProtocolGuid, NULL,
                                    (VOID **)&SwDispatch);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  // Register the Rogue SMI Handler to trigger on port 0xB2, value 0xCC
  SwContext.SwSmiInputValue = ROGUE_SMI_SW_VALUE;
  Status = SwDispatch->Register(SwDispatch, RogueSmiHandler, &SwContext,
                                &DispatchHandle);

  if (!EFI_ERROR(Status)) {
    DEBUG((DEBUG_INFO,
           "[RogueSmi] Registered adversarial SMI handler (SwSmi: 0x%X).\n",
           ROGUE_SMI_SW_VALUE));
  }

  return Status;
}
