.origin 0
.entrypoint TOP

#define DDR r29
#define DDR_SIZE r28
#define SHARED_RAM r27

#define SHARED_RAM_ADDRESS 0x10000

TOP:
  // Enable OCP master ports in SYSCFG register
  LBCO r0, C4, 4, 4
  CLR r0, r0, 4
  SBCO r0, C4, 4, 4

  MOV SHARED_RAM, SHARED_RAM_ADDRESS

  // From shared RAM, grab the address of the shared DDR segment
  LBBO DDR, SHARED_RAM, 0, 4
  // And the size of the segment from SHARED_RAM + 4
  LBBO DDR_SIZE, SHARED_RAM, 4, 4

  // BIGLOOP is one pass overwriting the shared DDR memory segment
  mov r12, 0
  mov r14, 10000
BIGLOOP:

  // Start at the beginning of the segment
  MOV r10, DDR
  ADD r11, DDR, DDR_SIZE

  // Tight loop writing the physical address of each word into that word
LOOP0:
  SBBO r10, r10, 0, 4
  ADD r10, r10, 4
  // XXX: This means r10 < r11, opposite what I expected!
  QBLT LOOP0, r11, r10

  ADD r12, r12, 1
  SBBO r12, SHARED_RAM, 0, 4
  QBGT BIGLOOP, r12, r14

  // Interrupt the host so it knows we're done
  MOV r31.b0, 19 + 16

// Don't forget to halt!
HALT
