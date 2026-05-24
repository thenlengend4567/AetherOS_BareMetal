#ifndef ATA_H
#define ATA_H

#include <stdint.h>

// ATA IO Ports (Primary Bus)
#define ATA_REG_DATA       0x1F0
#define ATA_REG_ERROR      0x1F1
#define ATA_REG_FEATURES   0x1F1
#define ATA_REG_SECCOUNT   0x1F2
#define ATA_REG_LBA0       0x1F3
#define ATA_REG_LBA1       0x1F4
#define ATA_REG_LBA2       0x1F5
#define ATA_REG_HDDEV      0x1F6
#define ATA_REG_STATUS     0x1F7
#define ATA_REG_COMMAND    0x1F7

// Status Register Bits
#define ATA_SR_ERR         0x01    // Error
#define ATA_SR_DRQ         0x08    // Data Request
#define ATA_SR_DF          0x20    // Drive Fault
#define ATA_SR_BSY         0x80    // Busy

// Commands
#define ATA_CMD_READ_PIO   0x20

#define ATA_SECTOR_SIZE    512

/**
 * @brief Read sectors using LBA28 PIO polling.
 * 
 * @param lba Starting Logical Block Address.
 * @param sector_count Number of sectors to read.
 * @param target_address Memory address to load the data into.
 */
void ata_read_sectors(uint32_t lba, uint32_t sector_count, void *target_address);

/**
 * @brief Load doom1.wad from raw sector 1000 to RAM at 0x00200000.
 * Reads exactly 8196 sectors.
 */
void ata_load_doom_wad(void);

#endif // ATA_H
