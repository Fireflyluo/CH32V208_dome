

#include "OLED.h"
#include "IQmath_RV32.h"
#include "board.h"
#include "drv_i2c.h"
#include "i2c_bus_arbiter.h"
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/*
 * OLED 驱动说明：
 * 1. 显存为 OLED_DisplayBuf[8][128]，每个字节对应纵向 8 个像素。
 * 2. 业务层先改显存，再调用 OLED_Update/OLED_UpdateArea 刷新到屏幕。
 * 3. 水平寻址全屏刷新走一次性 DMA 发送，局部刷新走分段发送。
 */

uint8_t OLED_DisplayBuf[8][128];

/* I2C 设备与传输参数 */
#define OLED_I2C_DEV_ADDR 0x3C
#define OLED_I2C_OWNER_ID 3U
#define OLED_DMA_BULK_THRESHOLD 16U
#define OLED_I2C_REQ_TIMEOUT_MS 50U
#define OLED_I2C_FRAME_TIMEOUT_MS 1500U
#define OLED_WIDTH 128U
#define OLED_HEIGHT 64U
#define OLED_PAGE_CNT (OLED_HEIGHT / 8U)
#define OLED_FRAME_BYTES (OLED_WIDTH * OLED_PAGE_CNT)

void OLED_SetCursor(uint8_t Page, uint8_t X);
void OLED_WriteData(uint8_t *Data, uint16_t Count);

/* 发送 OLED 命令（控制字节 0x00） */
static int oled_submit_commands(const uint8_t *cmds, uint16_t len)
{

    i2c_bus_request_t req;

    if ((cmds == NULL) || (len == 0U))
    {
        return -1;
    }

    req.bus = I2C_NUM_1;
    req.type = I2C_BUS_REQ_WRITE_REG;
    req.owner_id = OLED_I2C_OWNER_ID;
    req.dev_addr = OLED_I2C_DEV_ADDR;
    req.reg = 0x00;
    req.wbuf = cmds;
    req.rbuf = NULL;
    req.len = len;
    req.mode_hint = I2C_MODE_IT;
    req.prio = I2C_BUS_PRIO_LOW;
    req.timeout_ms = OLED_I2C_REQ_TIMEOUT_MS;
    return i2c_bus_submit_sync(&req);
}

/* 重新下发关键显示状态，避免地址模式/滚动状态被外部污染 */
static int oled_reassert_horizontal_state(void)
{

    static const uint8_t cmds[] = {
        0x20, 0x00,
        0x2E,
        0xD3, 0x00,
        0x40};
    return oled_submit_commands(cmds, (uint16_t)sizeof(cmds));
}

/* 设置页模式光标 */
static int oled_set_cursor_checked(uint8_t Page, uint8_t X)
{

    uint8_t cmds[3];

    cmds[0] = (uint8_t)(0xB0U | Page);
    cmds[1] = (uint8_t)(0x10U | ((X & 0xF0U) >> 4));
    cmds[2] = (uint8_t)(0x00U | (X & 0x0FU));
    return oled_submit_commands(cmds, 3U);
}

/* 发送 OLED 数据（控制字节 0x40） */
static int oled_write_data_checked(uint8_t *Data, uint16_t Count)
{
    static uint8_t s_oled_frame_buf[1U + OLED_FRAME_BYTES];
    i2c_bus_request_t req;
    int ret;

    if ((Data == NULL) || (Count == 0U) || (Count > OLED_FRAME_BYTES))
    {
        return -1;
    }

    req.bus = I2C_NUM_1;
    req.type = I2C_BUS_REQ_WRITE;
    req.owner_id = OLED_I2C_OWNER_ID;
    req.dev_addr = OLED_I2C_DEV_ADDR;
    req.reg = 0U;
    req.rbuf = NULL;
    req.prio = I2C_BUS_PRIO_HIGH;
    req.timeout_ms = OLED_I2C_FRAME_TIMEOUT_MS;

    s_oled_frame_buf[0] = 0x40U;
    memcpy(&s_oled_frame_buf[1], Data, Count);

    req.wbuf = s_oled_frame_buf;
    req.len = (uint16_t)(Count + 1U);
    req.mode_hint = (Count >= OLED_DMA_BULK_THRESHOLD) ? I2C_MODE_DMA : I2C_MODE_IT;
    ret = i2c_bus_submit_sync(&req);
    if ((ret != 0) && (req.mode_hint == I2C_MODE_DMA))
    {
        req.mode_hint = I2C_MODE_IT;
        ret = i2c_bus_submit_sync(&req);
    }
    return ret;
}

/* 水平寻址全屏刷新：一次性发送 0x40 + 1024B 显存 */
#if (OLED_ADDR_MODE == OLED_ADDR_MODE_HORIZONTAL)
static void oled_refresh_fullscreen_once(void)
{
    uint8_t cmd_window[6];

    if (oled_reassert_horizontal_state() != 0)
    {
        return;
    }

    cmd_window[0] = 0x21;
    cmd_window[1] = 0U;
    cmd_window[2] = (uint8_t)(OLED_WIDTH - 1U);
    cmd_window[3] = 0x22;
    cmd_window[4] = 0U;
    cmd_window[5] = (uint8_t)(OLED_PAGE_CNT - 1U);
    if (oled_submit_commands(cmd_window, 6U) != 0)
    {
        return;
    }

    (void)oled_write_data_checked(&OLED_DisplayBuf[0][0], OLED_FRAME_BYTES);
}
#endif

/*
 * 刷新指定页区域：
 * - 水平寻址：先设置窗口；局部刷新走分段发送。
 * - 页寻址：逐页设置光标并发送。
 */
static void oled_refresh_area_pages(uint8_t x, uint8_t page_start, uint8_t page_end, uint8_t width)
{

#if (OLED_ADDR_MODE == OLED_ADDR_MODE_HORIZONTAL)

    uint8_t cmd_window[6];
    uint8_t page;
    uint16_t bytes_total;
    uint16_t offset;
    static uint8_t s_oled_area_buf[OLED_FRAME_BYTES];

    if (oled_reassert_horizontal_state() != 0)
    {
        return;
    }

    cmd_window[0] = 0x21;
    cmd_window[1] = x;
    cmd_window[2] = (uint8_t)(x + width - 1U);
    cmd_window[3] = 0x22;
    cmd_window[4] = page_start;
    cmd_window[5] = page_end;
    if (oled_submit_commands(cmd_window, 6U) != 0)
    {
        return;
    }

    bytes_total = (uint16_t)((uint16_t)(page_end - page_start + 1U) * width);

    for (page = page_start; page <= page_end; page++)
    {
        uint16_t dst_off = (uint16_t)((uint16_t)(page - page_start) * width);
        memcpy(&s_oled_area_buf[dst_off], &OLED_DisplayBuf[page][x], width);
    }

    offset = 0U;
    while (offset < bytes_total)
    {
        uint16_t chunk = (uint16_t)(bytes_total - offset);
        if (chunk > (uint16_t)(I2C_MAX_WRITE_LEN - 1U))
        {
            chunk = (uint16_t)(I2C_MAX_WRITE_LEN - 1U);
        }
        if (oled_write_data_checked(&s_oled_area_buf[offset], chunk) != 0)
        {
            break;
        }
        offset = (uint16_t)(offset + chunk);
    }
#else

    uint8_t page;
    for (page = page_start; page <= page_end; page++)
    {
        if (oled_set_cursor_checked(page, x) != 0)
        {
            break;
        }
        if (oled_write_data_checked(&OLED_DisplayBuf[page][x], width) != 0)
        {
            break;
        }
    }
#endif
}

void OLED_W_SCL(uint8_t BitValue)
{

    GPIO_WriteBit(GPIOB, GPIO_Pin_8, (BitAction)BitValue);
}

void OLED_W_SDA(uint8_t BitValue)
{

    GPIO_WriteBit(GPIOB, GPIO_Pin_9, (BitAction)BitValue);
}

void OLED_GPIO_Init(void)
{
    uint32_t i, j;

    for (i = 0; i < 1000; i++)
    {
        for (j = 0; j < 1000; j++)
            ;
    }

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    OLED_W_SCL(1);
    OLED_W_SDA(1);
}

void OLED_I2C_Start(void)
{
    OLED_W_SDA(1);
    OLED_W_SCL(1);
    OLED_W_SDA(0);
    OLED_W_SCL(0);
}

void OLED_I2C_Stop(void)
{
    OLED_W_SDA(0);
    OLED_W_SCL(1);
    OLED_W_SDA(1);
}

void OLED_I2C_SendByte(uint8_t Byte)
{
    uint8_t i;

    for (i = 0; i < 8; i++)
    {

        OLED_W_SDA(!!(Byte & (0x80 >> i)));
        OLED_W_SCL(1);
        OLED_W_SCL(0);
    }

    OLED_W_SCL(1);
    OLED_W_SCL(0);
}

void OLED_WriteCommand(uint8_t Command)
{

    (void)oled_submit_commands(&Command, 1U);
}

void OLED_WriteData(uint8_t *Data, uint16_t Count)
{
    (void)oled_write_data_checked(Data, Count);
}

/* OLED 上电初始化与基础显示参数配置 */
void OLED_Init(void)
{

    HAL_Delay(50);
    OLED_WriteCommand(0xAE);

    OLED_WriteCommand(0xD5);
    OLED_WriteCommand(0x80);

    OLED_WriteCommand(0xA8);
    OLED_WriteCommand(0x3F);

    OLED_WriteCommand(0xD3);
    OLED_WriteCommand(0x00);

    OLED_WriteCommand(0x20);
    OLED_WriteCommand(OLED_ADDR_MODE);

    OLED_WriteCommand(0x40);

    OLED_WriteCommand(0xA1);

    OLED_WriteCommand(0xC8);

    OLED_WriteCommand(0xDA);
    OLED_WriteCommand(0x12);

    OLED_WriteCommand(0x81);
    OLED_WriteCommand(0xCF);

    OLED_WriteCommand(0xD9);
    OLED_WriteCommand(0xF1);

    OLED_WriteCommand(0xDB);
    OLED_WriteCommand(0x30);

    OLED_WriteCommand(0xA4);

    OLED_WriteCommand(0xA6);

    OLED_WriteCommand(0x8D);
    OLED_WriteCommand(0x14);

    OLED_WriteCommand(0x2E);

    OLED_WriteCommand(0xAF);
    HAL_Delay(50);
    OLED_Clear();
    OLED_Update();
}

void OLED_SetCursor(uint8_t Page, uint8_t X)
{

    (void)oled_set_cursor_checked(Page, X);
}

uint32_t OLED_Pow(uint32_t X, uint32_t Y)
{
    uint32_t Result = 1;
    while (Y--)
    {
        Result *= X;
    }
    return Result;
}

uint8_t OLED_pnpoly(uint8_t nvert, int16_t *vertx, int16_t *verty, int16_t testx, int16_t testy)
{
    int16_t i, j, c = 0;

    for (i = 0, j = nvert - 1; i < nvert; j = i++)
    {
        if (((verty[i] > testy) != (verty[j] > testy)) &&
            (testx < (vertx[j] - vertx[i]) * (testy - verty[i]) / (verty[j] - verty[i]) + vertx[i]))
        {
            c = !c;
        }
    }
    return c;
}

uint8_t OLED_IsInAngle(int16_t X, int16_t Y, int16_t StartAngle, int16_t EndAngle)
{
    int16_t PointAngle;
    PointAngle = atan2(Y, X) / 3.14 * 180;
    if (StartAngle < EndAngle)
    {

        if (PointAngle >= StartAngle && PointAngle <= EndAngle)
        {
            return 1;
        }
    }
    else
    {

        if (PointAngle >= StartAngle || PointAngle <= EndAngle)
        {
            return 1;
        }
    }
    return 0;
}

void OLED_Update(void)
{
    /* 刷新整屏：水平寻址命中一次性整帧发送路径 */
#if (OLED_ADDR_MODE == OLED_ADDR_MODE_HORIZONTAL)
    oled_refresh_fullscreen_once();
#else
    oled_refresh_area_pages(0U, 0U, (uint8_t)(OLED_PAGE_CNT - 1U), OLED_WIDTH);
#endif
}

/* 刷新局部区域（自动对齐到页） */
void OLED_UpdateArea(uint8_t X, uint8_t Y, uint8_t Width, uint8_t Height)
{
    uint8_t page_start;
    uint8_t page_end;

    if (X > (OLED_WIDTH - 1U))
    {
        return;
    }
    if (Y > (OLED_HEIGHT - 1U))
    {
        return;
    }
    if ((uint16_t)X + (uint16_t)Width > OLED_WIDTH)
    {
        Width = (uint8_t)(OLED_WIDTH - X);
    }
    if ((uint16_t)Y + (uint16_t)Height > OLED_HEIGHT)
    {
        Height = (uint8_t)(OLED_HEIGHT - Y);
    }
    if ((Width == 0U) || (Height == 0U))
    {
        return;
    }

    page_start = (uint8_t)(Y / 8U);
    page_end = (uint8_t)((Y + Height - 1U) / 8U);
    oled_refresh_area_pages(X, page_start, page_end, Width);
}

void OLED_Clear(void)
{
    uint8_t i, j;
    for (j = 0; j < 8; j++)
    {
        for (i = 0; i < 128; i++)
        {
            OLED_DisplayBuf[j][i] = 0x00;
        }
    }
}

void OLED_ClearArea(uint8_t X, uint8_t Y, uint8_t Width, uint8_t Height)
{
    uint8_t i, j;

    if (X > 127)
    {
        return;
    }
    if (Y > 63)
    {
        return;
    }
    if (X + Width > 128)
    {
        Width = 128 - X;
    }
    if (Y + Height > 64)
    {
        Height = 64 - Y;
    }

    for (j = Y; j < Y + Height; j++)
    {
        for (i = X; i < X + Width; i++)
        {
            OLED_DisplayBuf[j / 8][i] &= ~(0x01 << (j % 8));
        }
    }
}

void OLED_Reverse(void)
{
    uint8_t i, j;
    for (j = 0; j < 8; j++)
    {
        for (i = 0; i < 128; i++)
        {
            OLED_DisplayBuf[j][i] ^= 0xFF;
        }
    }
}

void OLED_ReverseArea(uint8_t X, uint8_t Y, uint8_t Width, uint8_t Height)
{
    uint8_t i, j;

    if (X > 127)
    {
        return;
    }
    if (Y > 63)
    {
        return;
    }
    if (X + Width > 128)
    {
        Width = 128 - X;
    }
    if (Y + Height > 64)
    {
        Height = 64 - Y;
    }

    for (j = Y; j < Y + Height; j++)
    {
        for (i = X; i < X + Width; i++)
        {
            OLED_DisplayBuf[j / 8][i] ^= 0x01 << (j % 8);
        }
    }
}

void OLED_ShowChar(uint8_t X, uint8_t Y, char Char, uint8_t FontSize)
{
    if (FontSize == OLED_8X16)
    {

        OLED_ShowImage(X, Y, 8, 16, OLED_F8x16[Char - ' ']);
    }
    else if (FontSize == OLED_6X8)
    {

        OLED_ShowImage(X, Y, 6, 8, OLED_F6x8[Char - ' ']);
    }
}

void OLED_ShowString(uint8_t X, uint8_t Y, char *String, uint8_t FontSize)
{
    uint8_t i;
    for (i = 0; String[i] != '\0'; i++)
    {

        OLED_ShowChar(X + i * FontSize, Y, String[i], FontSize);
    }
}

void OLED_ShowNum(uint8_t X, uint8_t Y, uint32_t Number, uint8_t Length, uint8_t FontSize)
{
    uint8_t i;
    for (i = 0; i < Length; i++)
    {

        OLED_ShowChar(X + i * FontSize, Y, Number / OLED_Pow(10, Length - i - 1) % 10 + '0', FontSize);
    }
}

void OLED_ShowSignedNum(uint8_t X, uint8_t Y, int32_t Number, uint8_t Length, uint8_t FontSize)
{
    uint8_t i;
    uint32_t Number1;

    if (Number >= 0)
    {
        OLED_ShowChar(X, Y, '+', FontSize);
        Number1 = Number;
    }
    else
    {
        OLED_ShowChar(X, Y, '-', FontSize);
        Number1 = -Number;
    }

    for (i = 0; i < Length; i++)
    {

        OLED_ShowChar(X + (i + 1) * FontSize, Y, Number1 / OLED_Pow(10, Length - i - 1) % 10 + '0', FontSize);
    }
}

void OLED_ShowHexNum(uint8_t X, uint8_t Y, uint32_t Number, uint8_t Length, uint8_t FontSize)
{
    uint8_t i, SingleNumber;
    for (i = 0; i < Length; i++)
    {

        SingleNumber = Number / OLED_Pow(16, Length - i - 1) % 16;

        if (SingleNumber < 10)
        {

            OLED_ShowChar(X + i * FontSize, Y, SingleNumber + '0', FontSize);
        }
        else
        {

            OLED_ShowChar(X + i * FontSize, Y, SingleNumber - 10 + 'A', FontSize);
        }
    }
}

void OLED_ShowBinNum(uint8_t X, uint8_t Y, uint32_t Number, uint8_t Length, uint8_t FontSize)
{
    uint8_t i;
    for (i = 0; i < Length; i++)
    {

        OLED_ShowChar(X + i * FontSize, Y, Number / OLED_Pow(2, Length - i - 1) % 2 + '0', FontSize);
    }
}

void OLED_ShowFloatNum(uint8_t X, uint8_t Y, double Number, uint8_t IntLength, uint8_t FraLength, uint8_t FontSize)
{
    uint32_t PowNum, IntNum, FraNum;

    if (Number >= 0)
    {
        OLED_ShowChar(X, Y, '+', FontSize);
    }
    else
    {
        OLED_ShowChar(X, Y, '-', FontSize);
        Number = -Number;
    }

    IntNum = Number;
    Number -= IntNum;
    PowNum = OLED_Pow(10, FraLength);
    FraNum = round(Number * PowNum);
    IntNum += FraNum / PowNum;

    OLED_ShowNum(X + FontSize, Y, IntNum, IntLength, FontSize);

    OLED_ShowChar(X + (IntLength + 1) * FontSize, Y, '.', FontSize);

    OLED_ShowNum(X + (IntLength + 2) * FontSize, Y, FraNum, FraLength, FontSize);
}

void OLED_ShowChinese(uint8_t X, uint8_t Y, char *Chinese)
{
    uint8_t pChinese = 0;
    uint8_t pIndex;
    uint8_t i;
    char SingleChinese[OLED_CHN_CHAR_WIDTH + 1] = {0};

    for (i = 0; Chinese[i] != '\0'; i++)
    {
        SingleChinese[pChinese] = Chinese[i];
        pChinese++;

        if (pChinese >= OLED_CHN_CHAR_WIDTH)
        {
            pChinese = 0;

            for (pIndex = 0; strcmp(OLED_CF16x16[pIndex].Index, "") != 0; pIndex++)
            {

                if (strcmp(OLED_CF16x16[pIndex].Index, SingleChinese) == 0)
                {
                    break;
                }
            }

            OLED_ShowImage(X + ((i + 1) / OLED_CHN_CHAR_WIDTH - 1) * 16, Y, 16, 16, OLED_CF16x16[pIndex].Data);
        }
    }
}

void OLED_ShowImage(uint8_t X, uint8_t Y, uint8_t Width, uint8_t Height, const uint8_t *Image)
{
    uint8_t i, j;

    if (X > 127)
    {
        return;
    }
    if (Y > 63)
    {
        return;
    }

    OLED_ClearArea(X, Y, Width, Height);

    for (j = 0; j < (Height - 1) / 8 + 1; j++)
    {

        for (i = 0; i < Width; i++)
        {

            if (X + i > 127)
            {
                break;
            }
            if (Y / 8 + j > 7)
            {
                return;
            }

            OLED_DisplayBuf[Y / 8 + j][X + i] |= Image[j * Width + i] << (Y % 8);

            if (Y / 8 + j + 1 > 7)
            {
                continue;
            }

            OLED_DisplayBuf[Y / 8 + j + 1][X + i] |= Image[j * Width + i] >> (8 - Y % 8);
        }
    }
}

void OLED_Printf(uint8_t X, uint8_t Y, uint8_t FontSize, char *format, ...)
{
    char String[30];
    va_list arg;
    va_start(arg, format);
    vsprintf(String, format, arg);
    va_end(arg);
    OLED_ShowString(X, Y, String, FontSize);
}

void OLED_DrawPoint(uint8_t X, uint8_t Y)
{

    if (X > 127)
    {
        return;
    }
    if (Y > 63)
    {
        return;
    }

    OLED_DisplayBuf[Y / 8][X] |= 0x01 << (Y % 8);
}

uint8_t OLED_GetPoint(uint8_t X, uint8_t Y)
{

    if (X > 127)
    {
        return 0;
    }
    if (Y > 63)
    {
        return 0;
    }

    if (OLED_DisplayBuf[Y / 8][X] & 0x01 << (Y % 8))
    {
        return 1;
    }

    return 0;
}

void OLED_DrawLine(uint8_t X0, uint8_t Y0, uint8_t X1, uint8_t Y1)
{
    int16_t x, y, dx, dy, d, incrE, incrNE, temp;
    int16_t x0 = X0, y0 = Y0, x1 = X1, y1 = Y1;
    uint8_t yflag = 0, xyflag = 0;

    if (y0 == y1)
    {

        if (x0 > x1)
        {
            temp = x0;
            x0 = x1;
            x1 = temp;
        }

        for (x = x0; x <= x1; x++)
        {
            OLED_DrawPoint(x, y0);
        }
    }
    else if (x0 == x1)
    {

        if (y0 > y1)
        {
            temp = y0;
            y0 = y1;
            y1 = temp;
        }

        for (y = y0; y <= y1; y++)
        {
            OLED_DrawPoint(x0, y);
        }
    }
    else
    {

        if (x0 > x1)
        {

            temp = x0;
            x0 = x1;
            x1 = temp;
            temp = y0;
            y0 = y1;
            y1 = temp;
        }

        if (y0 > y1)
        {

            y0 = -y0;
            y1 = -y1;

            yflag = 1;
        }

        if (y1 - y0 > x1 - x0)
        {

            temp = x0;
            x0 = y0;
            y0 = temp;
            temp = x1;
            x1 = y1;
            y1 = temp;

            xyflag = 1;
        }

        dx = x1 - x0;
        dy = y1 - y0;
        incrE = 2 * dy;
        incrNE = 2 * (dy - dx);
        d = 2 * dy - dx;
        x = x0;
        y = y0;

        if (yflag && xyflag)
        {
            OLED_DrawPoint(y, -x);
        }
        else if (yflag)
        {
            OLED_DrawPoint(x, -y);
        }
        else if (xyflag)
        {
            OLED_DrawPoint(y, x);
        }
        else
        {
            OLED_DrawPoint(x, y);
        }

        while (x < x1)
        {
            x++;
            if (d < 0)
            {
                d += incrE;
            }
            else
            {
                y++;
                d += incrNE;
            }

            if (yflag && xyflag)
            {
                OLED_DrawPoint(y, -x);
            }
            else if (yflag)
            {
                OLED_DrawPoint(x, -y);
            }
            else if (xyflag)
            {
                OLED_DrawPoint(y, x);
            }
            else
            {
                OLED_DrawPoint(x, y);
            }
        }
    }
}

void OLED_DrawRectangle(uint8_t X, uint8_t Y, uint8_t Width, uint8_t Height, uint8_t IsFilled)
{
    uint8_t i, j;
    if (!IsFilled)
    {

        for (i = X; i < X + Width; i++)
        {
            OLED_DrawPoint(i, Y);
            OLED_DrawPoint(i, Y + Height - 1);
        }

        for (i = Y; i < Y + Height; i++)
        {
            OLED_DrawPoint(X, i);
            OLED_DrawPoint(X + Width - 1, i);
        }
    }
    else
    {

        for (i = X; i < X + Width; i++)
        {

            for (j = Y; j < Y + Height; j++)
            {

                OLED_DrawPoint(i, j);
            }
        }
    }
}

void OLED_DrawTriangle(uint8_t X0, uint8_t Y0, uint8_t X1, uint8_t Y1, uint8_t X2, uint8_t Y2, uint8_t IsFilled)
{
    uint8_t minx = X0, miny = Y0, maxx = X0, maxy = Y0;
    uint8_t i, j;
    int16_t vx[] = {X0, X1, X2};
    int16_t vy[] = {Y0, Y1, Y2};

    if (!IsFilled)
    {

        OLED_DrawLine(X0, Y0, X1, Y1);
        OLED_DrawLine(X0, Y0, X2, Y2);
        OLED_DrawLine(X1, Y1, X2, Y2);
    }
    else
    {

        if (X1 < minx)
        {
            minx = X1;
        }
        if (X2 < minx)
        {
            minx = X2;
        }
        if (Y1 < miny)
        {
            miny = Y1;
        }
        if (Y2 < miny)
        {
            miny = Y2;
        }

        if (X1 > maxx)
        {
            maxx = X1;
        }
        if (X2 > maxx)
        {
            maxx = X2;
        }
        if (Y1 > maxy)
        {
            maxy = Y1;
        }
        if (Y2 > maxy)
        {
            maxy = Y2;
        }

        for (i = minx; i <= maxx; i++)
        {

            for (j = miny; j <= maxy; j++)
            {

                if (OLED_pnpoly(3, vx, vy, i, j))
                {
                    OLED_DrawPoint(i, j);
                }
            }
        }
    }
}

void OLED_DrawCircle(uint8_t X, uint8_t Y, uint8_t Radius, uint8_t IsFilled)
{
    int16_t x, y, d, j;

    d = 1 - Radius;
    x = 0;
    y = Radius;

    OLED_DrawPoint(X + x, Y + y);
    OLED_DrawPoint(X - x, Y - y);
    OLED_DrawPoint(X + y, Y + x);
    OLED_DrawPoint(X - y, Y - x);

    if (IsFilled)
    {

        for (j = -y; j < y; j++)
        {

            OLED_DrawPoint(X, Y + j);
        }
    }

    while (x < y)
    {
        x++;
        if (d < 0)
        {
            d += 2 * x + 1;
        }
        else
        {
            y--;
            d += 2 * (x - y) + 1;
        }

        OLED_DrawPoint(X + x, Y + y);
        OLED_DrawPoint(X + y, Y + x);
        OLED_DrawPoint(X - x, Y - y);
        OLED_DrawPoint(X - y, Y - x);
        OLED_DrawPoint(X + x, Y - y);
        OLED_DrawPoint(X + y, Y - x);
        OLED_DrawPoint(X - x, Y + y);
        OLED_DrawPoint(X - y, Y + x);

        if (IsFilled)
        {

            for (j = -y; j < y; j++)
            {

                OLED_DrawPoint(X + x, Y + j);
                OLED_DrawPoint(X - x, Y + j);
            }

            for (j = -x; j < x; j++)
            {

                OLED_DrawPoint(X - y, Y + j);
                OLED_DrawPoint(X + y, Y + j);
            }
        }
    }
}

void OLED_DrawEllipse(uint8_t X, uint8_t Y, uint8_t A, uint8_t B, uint8_t IsFilled)
{
    int16_t x, y, j;
    int16_t a = A, b = B;
    float d1, d2;

    x = 0;
    y = b;
    d1 = b * b + a * a * (-b + 0.5);

    if (IsFilled)
    {

        for (j = -y; j < y; j++)
        {

            OLED_DrawPoint(X, Y + j);
            OLED_DrawPoint(X, Y + j);
        }
    }

    OLED_DrawPoint(X + x, Y + y);
    OLED_DrawPoint(X - x, Y - y);
    OLED_DrawPoint(X - x, Y + y);
    OLED_DrawPoint(X + x, Y - y);

    while (b * b * (x + 1) < a * a * (y - 0.5))
    {
        if (d1 <= 0)
        {
            d1 += b * b * (2 * x + 3);
        }
        else
        {
            d1 += b * b * (2 * x + 3) + a * a * (-2 * y + 2);
            y--;
        }
        x++;

        if (IsFilled)
        {

            for (j = -y; j < y; j++)
            {

                OLED_DrawPoint(X + x, Y + j);
                OLED_DrawPoint(X - x, Y + j);
            }
        }

        OLED_DrawPoint(X + x, Y + y);
        OLED_DrawPoint(X - x, Y - y);
        OLED_DrawPoint(X - x, Y + y);
        OLED_DrawPoint(X + x, Y - y);
    }

    d2 = b * b * (x + 0.5) * (x + 0.5) + a * a * (y - 1) * (y - 1) - a * a * b * b;

    while (y > 0)
    {
        if (d2 <= 0)
        {
            d2 += b * b * (2 * x + 2) + a * a * (-2 * y + 3);
            x++;
        }
        else
        {
            d2 += a * a * (-2 * y + 3);
        }
        y--;

        if (IsFilled)
        {

            for (j = -y; j < y; j++)
            {

                OLED_DrawPoint(X + x, Y + j);
                OLED_DrawPoint(X - x, Y + j);
            }
        }

        OLED_DrawPoint(X + x, Y + y);
        OLED_DrawPoint(X - x, Y - y);
        OLED_DrawPoint(X - x, Y + y);
        OLED_DrawPoint(X + x, Y - y);
    }
}

void OLED_DrawArc(uint8_t X, uint8_t Y, uint8_t Radius, int16_t StartAngle, int16_t EndAngle, uint8_t IsFilled)
{
    int16_t x, y, d, j;

    d = 1 - Radius;
    x = 0;
    y = Radius;

    if (OLED_IsInAngle(x, y, StartAngle, EndAngle))
    {
        OLED_DrawPoint(X + x, Y + y);
    }
    if (OLED_IsInAngle(-x, -y, StartAngle, EndAngle))
    {
        OLED_DrawPoint(X - x, Y - y);
    }
    if (OLED_IsInAngle(y, x, StartAngle, EndAngle))
    {
        OLED_DrawPoint(X + y, Y + x);
    }
    if (OLED_IsInAngle(-y, -x, StartAngle, EndAngle))
    {
        OLED_DrawPoint(X - y, Y - x);
    }

    if (IsFilled)
    {

        for (j = -y; j < y; j++)
        {

            if (OLED_IsInAngle(0, j, StartAngle, EndAngle))
            {
                OLED_DrawPoint(X, Y + j);
            }
        }
    }

    while (x < y)
    {
        x++;
        if (d < 0)
        {
            d += 2 * x + 1;
        }
        else
        {
            y--;
            d += 2 * (x - y) + 1;
        }

        if (OLED_IsInAngle(x, y, StartAngle, EndAngle))
        {
            OLED_DrawPoint(X + x, Y + y);
        }
        if (OLED_IsInAngle(y, x, StartAngle, EndAngle))
        {
            OLED_DrawPoint(X + y, Y + x);
        }
        if (OLED_IsInAngle(-x, -y, StartAngle, EndAngle))
        {
            OLED_DrawPoint(X - x, Y - y);
        }
        if (OLED_IsInAngle(-y, -x, StartAngle, EndAngle))
        {
            OLED_DrawPoint(X - y, Y - x);
        }
        if (OLED_IsInAngle(x, -y, StartAngle, EndAngle))
        {
            OLED_DrawPoint(X + x, Y - y);
        }
        if (OLED_IsInAngle(y, -x, StartAngle, EndAngle))
        {
            OLED_DrawPoint(X + y, Y - x);
        }
        if (OLED_IsInAngle(-x, y, StartAngle, EndAngle))
        {
            OLED_DrawPoint(X - x, Y + y);
        }
        if (OLED_IsInAngle(-y, x, StartAngle, EndAngle))
        {
            OLED_DrawPoint(X - y, Y + x);
        }

        if (IsFilled)
        {

            for (j = -y; j < y; j++)
            {

                if (OLED_IsInAngle(x, j, StartAngle, EndAngle))
                {
                    OLED_DrawPoint(X + x, Y + j);
                }
                if (OLED_IsInAngle(-x, j, StartAngle, EndAngle))
                {
                    OLED_DrawPoint(X - x, Y + j);
                }
            }

            for (j = -x; j < x; j++)
            {

                if (OLED_IsInAngle(-y, j, StartAngle, EndAngle))
                {
                    OLED_DrawPoint(X - y, Y + j);
                }
                if (OLED_IsInAngle(y, j, StartAngle, EndAngle))
                {
                    OLED_DrawPoint(X + y, Y + j);
                }
            }
        }
    }
}
