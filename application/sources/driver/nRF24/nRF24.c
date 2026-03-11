#include "nRF24.h"

// --- HÀM SPI CHUẨN ---
static uint8_t SPI_Transfer(uint8_t data) {
    while (SPI_I2S_GetFlagStatus(nRF24_SPI_PORT, SPI_I2S_FLAG_TXE) == RESET);
    SPI_I2S_SendData(nRF24_SPI_PORT, data);
    while (SPI_I2S_GetFlagStatus(nRF24_SPI_PORT, SPI_I2S_FLAG_RXNE) == RESET);
    return SPI_I2S_ReceiveData(nRF24_SPI_PORT);
}

const uint8_t RX_PW_PIPES[6] = {0x11, 0x12, 0x13, 0x14, 0x15, 0x16};
const uint8_t RX_ADDR_PIPES[6] = {0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};

void nRF24_Init() {
    GPIO_InitTypeDef PORT;
    
    // Cấu hình CSN
    PORT.GPIO_Pin = nRF24_CSN_PIN;
    PORT.GPIO_Mode = GPIO_Mode_OUT;
    PORT.GPIO_Speed = GPIO_Speed_40MHz;
    PORT.GPIO_OType = GPIO_OType_PP;
    PORT.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(nRF24_CSN_PORT, &PORT);

    // Cấu hình CE
    PORT.GPIO_Pin = nRF24_CE_PIN;
    GPIO_Init(nRF24_CE_PORT, &PORT);

    // Cấu hình IRQ
    PORT.GPIO_Pin = nRF24_IRQ_PIN;
    PORT.GPIO_Mode = GPIO_Mode_IN;
    // Kéo lên mức CAO để chống nhiễu gây lag
    PORT.GPIO_PuPd = GPIO_PuPd_UP; 
    GPIO_Init(nRF24_IRQ_PORT, &PORT);

    nRF24_CSN_H();
    nRF24_CE_L();
}

void nRF24_WriteReg(uint8_t reg, uint8_t value) {
    nRF24_CSN_L();
    SPI_Transfer(reg);
    SPI_Transfer(value);
    nRF24_CSN_H();
}

uint8_t nRF24_ReadReg(uint8_t reg) {
    uint8_t value;
    nRF24_CSN_L();
    SPI_Transfer(reg & 0x1f);
    value = SPI_Transfer(nRF24_CMD_NOP);
    nRF24_CSN_H();
    return value;
}

void nRF24_ReadBuf(uint8_t reg, uint8_t *pBuf, uint8_t count) {
    nRF24_CSN_L();
    SPI_Transfer(reg);
    while (count--) *pBuf++ = SPI_Transfer(nRF24_CMD_NOP);
    nRF24_CSN_H();
}

void nRF24_WriteBuf(uint8_t reg, uint8_t *pBuf, uint8_t count) {
    nRF24_CSN_L();
    SPI_Transfer(reg);
    while (count--) SPI_Transfer(*pBuf++);
    nRF24_CSN_H();
}

void nRF24_SetRFChannel(uint8_t RFChannel) {
    nRF24_WriteReg(nRF24_CMD_WREG | nRF24_REG_RF_CH, RFChannel);
}

void nRF24_TXMode(uint8_t RetrCnt, uint8_t RetrDelay, uint8_t RFChan, nRF24_DataRate_TypeDef DataRate, nRF24_TXPower_TypeDef TXPower, nRF24_CRC_TypeDef CRCS, nRF24_PWR_TypeDef Power, uint8_t *TX_Addr, uint8_t TX_Addr_Width) {
    nRF24_CE_L();
    nRF24_WriteReg(nRF24_CMD_WREG | nRF24_REG_SETUP_RETR, ((RetrDelay << 4) & 0xf0) | (RetrCnt & 0x0f));
    nRF24_WriteReg(nRF24_CMD_WREG | nRF24_REG_RF_SETUP, (uint8_t)DataRate | (uint8_t)TXPower);
    nRF24_WriteReg(nRF24_CMD_WREG | nRF24_REG_CONFIG, (uint8_t)CRCS | (uint8_t)Power | nRF24_PRIM_TX);
    nRF24_SetRFChannel(RFChan);
    nRF24_WriteReg(nRF24_CMD_WREG | nRF24_REG_EN_AA, 0x01);
    nRF24_WriteReg(nRF24_CMD_WREG | nRF24_REG_SETUP_AW, TX_Addr_Width - 2);
    nRF24_WriteBuf(nRF24_CMD_WREG | nRF24_REG_TX_ADDR, TX_Addr, TX_Addr_Width);
    nRF24_WriteBuf(nRF24_CMD_WREG | nRF24_REG_RX_ADDR_P0, TX_Addr, TX_Addr_Width);
}

void nRF24_RXMode(nRF24_RX_PIPE_TypeDef PIPE, nRF24_ENAA_TypeDef PIPE_AA, uint8_t RFChan, nRF24_DataRate_TypeDef DataRate, nRF24_CRC_TypeDef CRCS, uint8_t *RX_Addr, uint8_t RX_Addr_Width, uint8_t RX_PAYLOAD, nRF24_TXPower_TypeDef TXPower) {
    uint8_t rreg;
    nRF24_CE_L();
    nRF24_WriteReg(nRF24_CMD_WREG | nRF24_REG_EN_AA, (PIPE_AA != nRF24_ENAA_OFF) ? (1 << PIPE) : 0);
    rreg = nRF24_ReadReg(nRF24_REG_EN_RXADDR);
    nRF24_WriteReg(nRF24_CMD_WREG | nRF24_REG_EN_RXADDR, rreg | (1 << PIPE));
    nRF24_WriteReg(nRF24_CMD_WREG | RX_PW_PIPES[PIPE], RX_PAYLOAD);
    nRF24_WriteReg(nRF24_CMD_WREG | nRF24_REG_RF_SETUP, (uint8_t)DataRate | (uint8_t)TXPower);
    nRF24_WriteReg(nRF24_CMD_WREG | nRF24_REG_CONFIG, (uint8_t)CRCS | nRF24_PWR_Up | nRF24_PRIM_RX);
    nRF24_SetRFChannel(RFChan);
    nRF24_WriteReg(nRF24_CMD_WREG | nRF24_REG_SETUP_AW, RX_Addr_Width - 2);
    nRF24_WriteBuf(nRF24_CMD_WREG | RX_ADDR_PIPES[PIPE], RX_Addr, RX_Addr_Width);
    nRF24_WriteReg(nRF24_CMD_WREG | nRF24_REG_STATUS, 0x70); 
    nRF24_WriteReg(nRF24_CMD_FLUSH_RX, 0xFF);
    nRF24_CE_H();
}

nRF24_TX_PCKT_TypeDef nRF24_TXPacket(uint8_t * pBuf, uint8_t TX_PAYLOAD) {
    uint8_t status;
    uint32_t wait = 0xFFFF; // Đã giảm timeout để tránh treo

    nRF24_CE_L();
    nRF24_WriteBuf(nRF24_CMD_W_TX_PAYLOAD, pBuf, TX_PAYLOAD);
    nRF24_CE_H();
    
    while ((GPIO_ReadInputDataBit(nRF24_IRQ_PORT, nRF24_IRQ_PIN) != Bit_RESET) && --wait);
    if (!wait) return nRF24_TX_TIMEOUT;

    nRF24_CE_L();
    status = nRF24_ReadReg(nRF24_REG_STATUS);
    nRF24_WriteReg(nRF24_CMD_WREG | nRF24_REG_STATUS, status | 0x70);

    if (status & nRF24_MASK_MAX_RT) {
        nRF24_WriteReg(nRF24_CMD_FLUSH_TX, 0xFF);
        return nRF24_TX_MAXRT;
    }
    if (status & nRF24_MASK_TX_DS) return nRF24_TX_SUCCESS;

    return nRF24_TX_ERROR;
}

// Hàm nhận tối ưu: Kiểm tra chân IRQ trước khi dùng SPI
nRF24_RX_PCKT_TypeDef nRF24_RXPacket(uint8_t * pBuf, uint8_t RX_PAYLOAD) {
    // Nếu chân IRQ mức cao -> Không có tin nhắn -> Thoát ngay lập tức
    if (GPIO_ReadInputDataBit(nRF24_IRQ_PORT, nRF24_IRQ_PIN) != Bit_RESET) {
        return nRF24_RX_PCKT_EMPTY;
    }

    // Nếu chân IRQ mức thấp -> Có tin nhắn -> Đọc qua SPI
    uint8_t status = nRF24_ReadReg(nRF24_REG_STATUS);
    if (status & nRF24_MASK_RX_DR) {
        nRF24_ReadBuf(nRF24_CMD_R_RX_PAYLOAD, pBuf, RX_PAYLOAD);
        nRF24_WriteReg(nRF24_CMD_WREG | nRF24_REG_STATUS, status | 0x70);
        return nRF24_RX_PCKT_PIPE0;
    }
    
    // Xóa cờ lỗi nếu có rác
    nRF24_WriteReg(nRF24_CMD_FLUSH_RX, 0xFF);
    nRF24_WriteReg(nRF24_CMD_WREG | nRF24_REG_STATUS, status | 0x70);
    return nRF24_RX_PCKT_EMPTY;
}

void nRF24_ClearIRQFlags(void) {
    uint8_t status = nRF24_ReadReg(nRF24_REG_STATUS);
    nRF24_WriteReg(nRF24_CMD_WREG | nRF24_REG_STATUS, status | 0x70);
}

void nRF24_PowerDown(void) {
    uint8_t conf = nRF24_ReadReg(nRF24_REG_CONFIG);
    nRF24_CE_L();
    nRF24_WriteReg(nRF24_CMD_WREG | nRF24_REG_CONFIG, conf & ~(1<<1));
}

void nRF24_Wake(void) {
    uint8_t conf = nRF24_ReadReg(nRF24_REG_CONFIG);
    nRF24_WriteReg(nRF24_CMD_WREG | nRF24_REG_CONFIG, conf | (1<<1));
}

void nRF24_SetTXPower(nRF24_TXPower_TypeDef TXPower) {
    uint8_t rf_setup = nRF24_ReadReg(nRF24_REG_RF_SETUP);
    nRF24_WriteReg(nRF24_CMD_WREG | nRF24_REG_RF_SETUP, (rf_setup & 0xf9) | (uint8_t)TXPower);
}