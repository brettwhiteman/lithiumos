#ifndef ATA_H
#define ATA_H

#include "../stdinc.h"

bool ata_wait_until_not_busy(uint32_t timeout_ms);
void ata_send_command(byte command);
void ata_select_drive(uint8_t drive);
void ata_set_mode_lba(void);
void ata_interrupt_enable(bool enable);
bool ata_read_lba28(uint32_t LBA, uint8_t sectorCount, void* dest, uint8_t drive);

#endif