#include <ata.h>
#include <timer.h>

#define ATA_PORT_DATA 					0x1F0
#define ATA_PORT_SECTOR_COUNT 			0x1F2
#define ATA_PORT_LBA_LOW 				0x1F3
#define ATA_PORT_LBA_MID 				0x1F4
#define ATA_PORT_LBA_HIGH 				0x1F5
#define ATA_PORT_DRIVE_HEAD_LBA_24_28 	0x1F6
#define ATA_PORT_COMMAND_STATUS 		0x1F7
#define ATA_PORT_CONTROL_REGISTER 		0x3F6

bool ata_wait_until_not_busy(uint32_t timeout_ms)
{
	uint8_t status = inportb(ATA_PORT_COMMAND_STATUS);

	uint32_t startTick = get_tick_count();

	while(status & 0x88) //BSY and DRQ must be clear
	{
		if(get_tick_count() > startTick + timeout_ms)
			return FALSE;

		status = inportb(ATA_PORT_COMMAND_STATUS);
	}

	return TRUE;
}

inline void ata_send_command(byte command)
{
	outportb(ATA_PORT_COMMAND_STATUS, command);
}

void ata_select_drive(uint8_t drive)
{
	uint8_t buf = inportb(ATA_PORT_DRIVE_HEAD_LBA_24_28);

	if(drive)
		buf |= 0xB0; //slave
	
	buf &= 0xEF;
	
	outportb(ATA_PORT_DRIVE_HEAD_LBA_24_28, buf);
}

inline void ata_set_mode_lba(void)
{
	uint8_t buf = inportb(ATA_PORT_DRIVE_HEAD_LBA_24_28);
	
	buf |= 0x40; //set bit 6
	
	outportb(ATA_PORT_DRIVE_HEAD_LBA_24_28, buf);
}

void ata_interrupt_enable(bool enable)
{
	outportb(ATA_PORT_CONTROL_REGISTER, enable ? (0 | 2) : 0);
}

bool ata_read_lba28(uint32_t LBA, uint8_t sectorCount, void* dest, uint8_t drive)
{
	if(!ata_wait_until_not_busy(1000))
		return FALSE;
	
	outportb(ATA_PORT_SECTOR_COUNT, sectorCount); //sector count
	outportb(ATA_PORT_LBA_LOW, LBA & 0xFF); //LBA low byte
	outportb(ATA_PORT_LBA_MID, (LBA >> 8) & 0xFF); //LBA mid byte
	outportb(ATA_PORT_LBA_HIGH, (LBA >> 16) & 0xFF); //LBA high byte
	outportb(ATA_PORT_DRIVE_HEAD_LBA_24_28, (LBA >> 24) & 0x0F); //bits 24-28 of LBA
	ata_select_drive(drive);
	ata_set_mode_lba();
	ata_interrupt_enable(FALSE);
	ata_send_command(0x20); //read sectors with retry
	
	uint8_t status = 0;
	
	for(uint32_t i = 0; i < 4; i++)
	{
		status = inportb(ATA_PORT_COMMAND_STATUS);

		if((!(status & 0x80)) && (status & 0x8)) //BSY bit clear and DRQ set
		{
			goto data_ready;
		}
	}
	
	while(1)
	{
		status = inportb(ATA_PORT_COMMAND_STATUS);

		if(status & 0x80) //BSY set
			continue;
		else
			break;
		
		if(status & 0x1) //ERR set
			return FALSE;
		
		if(status & 0x20) //DF set
			return FALSE;
	}
	
	
data_ready: ;
	
	uint16_t* memoryBuffer = (uint16_t*)dest;
	
	for(uint32_t i = 0; i < sectorCount; i++)
	{
		for(uint32_t i = 0; i < 256; i++)
		{
			*memoryBuffer = inportw(ATA_PORT_DATA);
			memoryBuffer++;
		}
		
		inportb(ATA_PORT_COMMAND_STATUS);
		inportb(ATA_PORT_COMMAND_STATUS);
		inportb(ATA_PORT_COMMAND_STATUS);
		inportb(ATA_PORT_COMMAND_STATUS);//400ns delay
		
		while(TRUE)
		{
			status = inportb(ATA_PORT_COMMAND_STATUS);
			
			if(status & 0x80) //BSY set
				continue;
			else
				break;
			
			if(status & 0x1) //ERR set
				return FALSE;
			
			if(status & 0x20) //DF set
				return FALSE;
		}
	}
	
	return TRUE;
}
