/**
  Secondary interrupt controller (SIRC) client interface.

  Copyright (c) 2026.

**/

#ifndef __HTCLEO_SIRC_H__
#define __HTCLEO_SIRC_H__

#define HTCLEO_SIRC_GPIO_GROUP1     9
#define HTCLEO_SIRC_GPIO_GROUP2     10

#define HTCLEO_SIRC_NUM_LINES       23

#define HTCLEO_SIRC_EDGE            1
#define HTCLEO_SIRC_LEVEL           0
#define HTCLEO_SIRC_NEGATIVE        1
#define HTCLEO_SIRC_POSITIVE        0

typedef struct _HTCLEO_SIRC_PROTOCOL HTCLEO_SIRC_PROTOCOL;

typedef
EFI_STATUS
(EFIAPI *HTCLEO_SIRC_REGISTER_LINE)(
  IN HTCLEO_SIRC_PROTOCOL  *This,
  IN UINT32                 Line,
  IN UINT32                 Flags,
  IN VOID                  *Handler,
  IN VOID                  *Context
  );

struct _HTCLEO_SIRC_PROTOCOL {
  HTCLEO_SIRC_REGISTER_LINE  RegisterLine;
  UINT64                     IrqCount[HTCLEO_SIRC_NUM_LINES];
};

extern EFI_GUID  gHtcLeoSircProtocolGuid;

#endif
