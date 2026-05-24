#include "ata.h"
#include "io.h"

/**
 * @brief Read sectors using LBA28 PIO polling.
 */
void ata_read_sectors(uint32_t lba, uint32_t sector_count, void *target_address) {
    uint16_t *buffer = (uint16_t *)target_address;
    uint32_t sectors_read = 0;

    while (sectors_read < sector_count) {
        uint32_t chunk = sector_count - sectors_read;
        if (chunk > 256) {
            chunk = 256;
        }

        // Wait for BSY to clear
        while (inb(ATA_REG_STATUS) & ATA_SR_BSY);

        // Select drive and send 4 highest bits of LBA
        uint32_t current_lba = lba + sectors_read;
        outb(ATA_REG_HDDEV, 0xE0 | ((current_lba >> 24) & 0x0F));
        io_wait();

        // Send sectors count and remaining LBA bits
        outb(ATA_REG_FEATURES, 0x00);
        outb(ATA_REG_SECCOUNT, (chunk == 256) ? 0 : (uint8_t)chunk);
        outb(ATA_REG_LBA0, (uint8_t)(current_lba & 0xFF));
        outb(ATA_REG_LBA1, (uint8_t)((current_lba >> 8) & 0xFF));
        outb(ATA_REG_LBA2, (uint8_t)((current_lba >> 16) & 0xFF));

        // Send the READ PIO command
        outb(ATA_REG_COMMAND, ATA_CMD_READ_PIO);

        // Poll and read data sector by sector
        for (uint32_t s = 0; s < chunk; ++s) {
            // Wait for BSY to clear
            while (inb(ATA_REG_STATUS) & ATA_SR_BSY);

            // Wait for DRQ to set
            while (!(inb(ATA_REG_STATUS) & ATA_SR_DRQ));

            // Read one sector (256 words / 512 bytes)
            insw(ATA_REG_DATA, buffer, 256);
            buffer += 256;
        }

        sectors_read += chunk;
    }
}

/**
 * @brief Load doom1.wad from raw sector 1000 directly to RAM at 0x00200000.
 */
void ata_load_doom_wad(void) {
    // 8196 sectors * 512 bytes = 4,196,352 bytes (~4.2MB) loaded to 0x00200000
    ata_read_sectors(1000, 8196, (void *)0x00200000);
}
